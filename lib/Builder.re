let rsync = (origDir, destDir) => {
  let s = Fpath.to_string;
  let cmd = [
    "rsync",
    "--quiet",
    "--archive",
    "--exclude",
    s(destDir),
    "--exclude",
    "node_modules",
    "--exclude",
    "_build",
    "--exclude",
    "_release",
    "--exclude",
    "_esybuild",
    "--exclude",
    "_esyinstall",
    /* The trailing "/" is important as it makes rsync to sync the contents of
     * origDir rather than the origDir itself into destDir, see "man rsync" for
     * details.
     */
    s(origDir) ++ "/",
    s(destDir)
  ];
  let cmd = Bos.Cmd.of_list(cmd);
  Bos.OS.Cmd.run(cmd);
};

let relocateSourceDir = (spec: BuildSpec.t) =>
  rsync(spec.sourceDir, spec.buildDir);

let relocateInstallDir = (_spec: BuildSpec.t) => Run.ok;

let relocateBuildDir = (_spec: BuildSpec.t) => Run.ok;

let relocateBuildDirCleanup = (_spec: BuildSpec.t) => Run.ok;

let doNothing = (_spec: BuildSpec.t) => Run.ok;

/**
 * Execute `run` within the build environment for `spec`.
 */
let withBuildEnv = (config: Config.t, spec: BuildSpec.t, run) => {
  open Run;
  let {BuildSpec.sourceDir, installDir, buildDir, stageDir, _} = spec;
  let (rootDir, prepareRootDir, completeRootDir) =
    switch (spec.buildType, spec.sourceType) {
    | (InSource, _) => (buildDir, relocateSourceDir, doNothing)
    | (JbuilderLike, Immutable) => (buildDir, relocateSourceDir, doNothing)
    | (JbuilderLike, Transient) => (
        sourceDir,
        relocateBuildDir,
        relocateBuildDirCleanup
      )
    | (JbuilderLike, Root) => (sourceDir, doNothing, doNothing)
    | (OutOfSource, _) => (sourceDir, doNothing, doNothing)
    };
  let sandboxConfig = {
    open Sandbox;
    let allowWriteToSourceDir =
      switch spec.buildType {
      | JbuilderLike => [
          Subpath(sourceDir / "_build"),
          Regex(sourceDir / ".*" / "[^/]*\\.install"),
          Regex(sourceDir / "[^/]*\\.install"),
          Regex(sourceDir / ".*" / "[^/]*\\.opam"),
          Regex(sourceDir / "[^/]*\\.opam"),
          Regex(sourceDir / ".*" / "jbuild-ignore")
        ]
      | _ => []
      };
    allowWriteToSourceDir
    @ [
      Regex(sourceDir / ".*" / "\\.merlin"),
      Regex(sourceDir / "\\.merlin"),
      Subpath(buildDir),
      Subpath(stageDir)
    ];
  };
  let%bind commandExec = Sandbox.sandboxExec({allowWrite: sandboxConfig});
  let rec runCommands = commands =>
    switch commands {
    | [] => Ok()
    | [cmd, ...cmds] =>
      switch (commandExec(~env=spec.env, cmd)) {
      | Ok(_) => runCommands(cmds)
      | Error(err) => Error(err)
      }
    };
  /*
   * Prepare build/install.
   */
  let prepare = () => {
    let%bind () = mkdir(installDir);
    let%bind () = mkdir(stageDir);
    let%bind () = mkdir(stageDir / "bin");
    let%bind () = mkdir(stageDir / "lib");
    let%bind () = mkdir(stageDir / "etc");
    let%bind () = mkdir(stageDir / "sbin");
    let%bind () = mkdir(stageDir / "man");
    let%bind () = mkdir(stageDir / "share");
    let%bind () = mkdir(stageDir / "doc");
    let%bind () =
      if (spec.sourceType == Immutable || spec.buildType == InSource) {
        let%bind () = rmdir(buildDir);
        let%bind () = mkdir(buildDir);
        ok;
      } else {
        let%bind () = mkdir(buildDir);
        ok;
      };
    let%bind () = prepareRootDir(spec);
    let%bind () = mkdir(buildDir / "_esy");
    let%bind () = mkdir(buildDir / "_esy" / "log");
    ok;
  };
  /*
   * Finalize build/install.
   */
  let finalize = result =>
    switch result {
    | Ok () =>
      let%bind () = relocateInstallDir(spec);
      let%bind () = completeRootDir(spec);
      ok;
    | error => error
    };
  let%bind store = Store.create(config.storePath);
  let%bind localStore = Store.create(config.localStorePath);
  let%bind () = prepare();
  let result = withCwd(rootDir, run(runCommands));
  let%bind () = finalize(result);
  result;
};

let build = (config, spec) => {
  open Run;
  let runBuildAndInstall = (run, ()) => {
    let {BuildSpec.build, install} = spec;
    let%bind () = run(build);
    let%bind () = run(install);
    ok;
  };
  withBuildEnv(config, spec, runBuildAndInstall);
};
