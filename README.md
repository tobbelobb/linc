This repo has just been created to make possible the use of build2's ci features.
It contains a build2 project, source code, and nothing more.

It is included as a submodule in https://gitlab.com/hangprinter/line-collision-detector
and is indended to be used through that repo.


## Build2 cheat sheet
### Build
```
b
```

### Build and run the tests
```
b test
```

### Clean
```
b clean
```

### Check which configs exist
```
bdep config list
```

### Create a config
```
bdep init --config-create ../linc-gcc8 cc config.cxx=g++-8

```

### The two configs already in this repo
```
bdep init --config-create ../linc-gcc @gcc cc config.cxx=g++
bdep init --config-create ../linc-clang @clang cc config.cxx=clang++-10
```

### Build with clang
```
b @../linc-clang/
# or
bdep update @clang
```

### Clean the clang build
```
b clean: @../linc-clang/
# or
bdep clean @clang
```

### Build with all configs
```
bdep update -a
```

### Build and run tests with all configs
```
bdep test -a
```

### Clean for all configs
```
bdep clean -a
```

### Run ci service
This will start builds on a lot of different machines,
and provide you a link to the results and logs:
```
bdep ci
```

### Build with -O3
```
b config.cxx.coptions=-O3  # Rebuild with -O3.
```

## More on build2 tests
The test setup deserves some more explanation.
There are two kinds of tests:

#### Functional tests (testscripts)
These are the ones called for example `somename.testscript`.
These tests are the tests written in build2's own Testscript language.
They are basically bash-scripts linked to executable build targets.
They run the associated executable (in our example: `somename`) and matches expected output based on input.
Associations between executables and testscripts are created in the buildfile as part of the test build process.

#### Unit tests
These are written in c++.
They are defined in files `somename.test.c++`, who are compiled into executables `somename.test`.
The test executables end up in the same directory where the main `linc` binary itself ends up.
They are also symlinked from the source directory like the `linc` binary.
To run a unit tests means to execute its executable with no arguments.

Note that we can associate a test executable with a testscript.
The file names will be `somename.test.testscript`.
In the testscript we can give the test executable arguments and check the output.
So we can create functional tests based on unit tests.

#### Automatic magic
Also note that the buildfile need not be changed for new `.test.c++`, `.test.h++`, or `.testscript`
files to be compiled and executed together with the other tests by the `b test` command.

New `.h++` and `.c++` files are also automatically compiled into both main executable and unit test executables.

Both build and unit testing is incremental, so only meaningfully changed source code is compiled and tested
with `b` and `b test`.

Build2 is nice, but not widely used, so many will need to skim through it's [docs](https://build2.org/build2/doc/build2-build-system-manual.xhtml)
to fully control how this project is built and managed.

## Licensing
The cppcore Gsl library is MIT licensed, and can be found here: https://github.com/microsoft/GSL
The Eigen library is MPL2.0 licensed and can be found here: https://gitlab.com/libeigen/eigen
The Spdlog library is MIT licensed, and can be found here: https://github.com/gabime/spdlog
The Doctest library is MIT licensed and can be found here: https://github.com/onqtam/doctest

