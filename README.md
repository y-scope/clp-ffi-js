clp-ffi-js is a JavaScript FFI library for [CLP]. It currently supports decoding log events from CLP
IR streams. Other CLP features are being added incrementally.

You can use GitHub issues to [request features][feature-req] or [report bugs][bug-report].

# Building

## Requirements
* CMake >= 3.16
* GNU Make
* Python 3
* [Task] >= 3.48.0

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
Before opening the project in an IDE, run the commands below.

Download and install [emscripten]:
```shell
task emsdk
```

Download the required source dependencies:
```shell
task deps
```

Set up the config files for our C++ linting tools:
```shell
task lint:cpp-configs
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

### Running specific linters
The commands above run all linting checks, but for performance you may want to run a subset (e.g.,
if you only changed C++ files, you don't need to run the YAML linting checks) using one of the tasks
in the table below.

| Task                    | Description                                              |
|-------------------------|----------------------------------------------------------|
| `lint:cpp-check`        | Runs the C++ linters (formatters and static analyzers).  |
| `lint:cpp-fix`          | Runs the C++ linters and fixes some violations.          |
| `lint:cpp-format-check` | Runs the C++ formatters.                                 |
| `lint:cpp-format-fix`   | Runs the C++ formatters and fixes some violations.       |
| `lint:cpp-static-check` | Runs the C++ static analyzers.                           |
| `lint:cpp-static-fix`   | Runs the C++ static analyzers and fixes some violations. |
| `lint:yml-check`        | Runs the YAML linters.                                   |
| `lint:yml-fix`          | Runs the YAML linters and fixes some violations.         |

[bug-report]: https://github.com/y-scope/clp-ffi-js/issues/new?labels=bug&template=bug-report.yml
[CLP]: https://github.com/y-scope/clp
[emscripten]: https://emscripten.org
[feature-req]: https://github.com/y-scope/clp-ffi-js/issues/new?labels=enhancement&template=feature-request.yml
[Task]: https://taskfile.dev
