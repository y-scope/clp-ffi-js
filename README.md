clp-ffi-js is a JavaScript FFI library for [CLP]. It currently supports decoding log events from CLP
IR streams. Other CLP features are being added incrementally.

You can use GitHub issues to [request features][feature-req] or [report bugs][bug-report].

# Building

## Requirements
* CMake 3.16 or higher
* GNU Make
* Python 3
* [Task] 3.38.0 or higher

## Setup
Initialize and update submodules:
```shell
git submodule update --init --recursive
```

## Common build commands
To build the project:
```shell
task
```

To clean the build:
```shell
task clean
```

# Contributing 
Follow the steps below to develop and contribute to the project.

## Set up
Before opening the project in an IDE, you'll first need to download and install [emscripten]:
```shell
task emsdk
```

## Linting
Before submitting a pull request, ensure you’ve run the linting commands below and either fixed any
violations or suppressed the warning.

To run all linting checks:
```shell
task lint:check
```

To run all linting checks AND automatically fix any fixable issues:
```shell
task lint:fix
```

[bug-report]: https://github.com/y-scope/clp-ffi-js/issues/new?labels=bug&template=bug-report.yml
[CLP]: https://github.com/y-scope/clp
[emscripten]: https://emscripten.org
[feature-req]: https://github.com/y-scope/clp-ffi-js/issues/new?labels=enhancement&template=feature-request.yml
[Task]: https://taskfile.dev
