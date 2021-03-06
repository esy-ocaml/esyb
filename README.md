**DEPRECATED: this project was merged into mainline esy/esy repo**

# esyb

A package builder for [esy][].

## How it works

A single command `esyb` operates on a build description, it allows to:

- Build a package with `esyb build` command.
- Shell into the build environment with `esyb shell`.
- Execute commands with the build environment with `esyb exec -- <command>`.

### Build description

Build description is a JSON files with the information about a package's build
environment and commands needed to build the package and install its artifacts
to the store.

Example:

```json
{
  "id": "pkg-1.0.0-somehash",
  "name": "pkg",
  "version": "1.0.0",
  "sourceType": "immutable",
  "buildType": "_build",
  "build": [
    ["jbuilder", "build"]
  ],
  "install": [
    ["jbuilder", "install"]
  ],
  "sourceDir": "%sandbox%",
  "stageDir": "%localStore%/s/name",
  "installDir": "%localStore%/i/name",
  "buildDir": "%localStore%/b/name",
  "env": {
    "cur__name": "pkg",
    "cur__install": "%localStore%/s/name",
    ...
  }
}
```

Usually you get those build description from esy.

Note that some properties are allowed to use `%name%` variables:

- `%sandbox%` — the absolute path to the sandbox.
- `%store%` — the absolute path to the store.
- `%localStore%` — the absolute path to the sandbox-local store.

This is needed to allow build descriptions not to be tied to a concrete host.

Examples:

Build the project using `$PWD/build.json` description:
```
% esyb build
```

Build the project using the specified build description:
```
% esyb build -B build-merlin.json
```

Build description can also be read from stdin (useful for automatically
generated build descriptions):
```
% cat build-merlin.json | esyb build -B -
```

## Requirements

- `rsync` executable

## Development

install `esy`, install dependencies and build:

```
% npm install -g esy
% make install build-dev
```

Then you can test it:

```
% esy x esyb
```

## Publishing a release

Things to consider:

- `package.json` contains npm metadata for a dist package, you can bump version
  in there using `npm version patch|minor|major` or othewise edit it however you
  need. Note that the list of files in to be included in the dist is also
  hardcoded in `Makefile`.

- `postinstall.sh` contains a list of instructions to be performed after the
  installation.

Produce a ready to be published release package within the `dist/` directory:

```
% make dist
```

Publish it:

```
% cd dist/
% npm publish
```

[esy]: http://esy.sh
