# Building the code

This document describes steps needed for compiling the code of the Smart Card
Connector App and examples of how programs can communicate with the app.


## Common building prerequisites

* **OS: Linux**.

  Building under a different \*nix system, Mac OS or Windows should be possible
  too, though most probably will require more efforts.

* The following tools should be present in the system: **bash**, **make**
  (version 4.3+ is recommended), **curl**, **sed**, **mktemp**, **realpath**,
  **xxd**.

* **Python 2.7**, including the dev package and the **python-six** module.

  Python 3.x is not supported yet.

* **git** (version 2.2.1+ is recommended).

* **OpenSSL** (version 1.0+ is recommended).

* **Xvfb**.

* (for 64-bit Linux) **32-bit version of libstdc++**.

  For example, on Ubuntu it's provided by the libstdc++6:i386 package.

* **Java Runtime Environment 7**.

* **CMake** (needed only in Emscripten builds).

In order to **run** the built apps, you will need *either* of these:

* a **Chromebook** with Chrome OS >= 48.

  This will provide the closest environment to the real world's one.

  However, the disadvantage of this option is the inconvenient way of doing
  short development iterations: each time the built apps will have to be somehow
  transferred to the Chromebook and installed onto it.

* a locally installed **Chrome** browser with version >= 48.

  This option will save time during development, allowing to install and run the
  apps easily on the local machine.

  For convenience, each app's Makefile provides a special `run` target that
  creates a temporary local Chrome profile and runs the browser with having the
  app installed and run into it. This allows to test the apps locally, without
  interfering with the real Chrome profile.

  One downside of this option is that the desktop Chrome does not provide all
  the APIs that are provided under Chrome OS. The most noticeable example is the
  **chrome.certificateProvider** API: it's only available under Chrome OS, so
  its usages in the apps will have to be stubbed out when executing locally.

  Another downside is that the desktop OS may require additional setup in order
  to allow Chrome (and, consequently, the apps being executed in it) to access
  the USB devices. Some instructions are given in the
  [docs/running-on-desktop.md](running-on-desktop.md) document.


## Building

Follow these steps for performing the *initial build*:

1. Execute:

   ```shell
   env/initialize.sh
   ```

   It's enough to execute this command only once, after you have cloned the
   whole repository (unless you would like to update to the latest tools
   versions).

   This will download and install locally the following dependencies required
   for building the apps:

   * *depot_tools* (see
     [https://commondatastorage.googleapis.com/chrome-infra-docs/flat/depot_tools/docs/html/depot_tools.html](https://commondatastorage.googleapis.com/chrome-infra-docs/flat/depot_tools/docs/html/depot_tools.html))
   * *NaCl SDK* (see
     [https://developer.chrome.com/native-client/sdk/download](https://developer.chrome.com/native-client/sdk/download))
   * *webports* (see
     [https://chromium.googlesource.com/webports/](https://chromium.googlesource.com/webports/))

2. Execute:

   ```shell
   source env/activate
   ```

   This command sets the environment variables required for enabling the use of
   the tools downloaded at step 1.

3. Execute:

   ```shell
   ./make-all.sh
   ```

   This builds the Connector App, the C++ Example App and the JS Example App and
   all the libraries shared between them.

After that, you can *perform incremental building* of either all of the apps (by
running the command from step 3.) or of the single app you work on (by following
its build instructions).

You should only make sure, however, that the environment definitions are always
here - and, if not, use the command from step 2 for setting them up back.


## Debug and Release building modes

During the development process, it's useful to enable the extended levels of
logging and (depending on the actual app) the more extensive debug assertions
checks.

Switching to the **Debug** building mode can be performed by adjusting the
`CONFIG` environment variable, i.e. by executing the following shell command
before building the apps::

```shell
export CONFIG=Debug
```

This triggers a number of things, basically (for some additional details
regarding concrete apps refer to their own documentation):

* For the compiled JavaScript code - enables the creation of the source map
  allowing to view the uncompiled code when debugging.

* For the JavaScript code built using the Closure library logging subsystem -
  selects more verbose logging level by default and enables printing extended
  details in the log messages (e.g. dumps of all parameters for some functions).

* For the C/C++ code - undefines the `NDEBUG` macro, which enables some extended
  debug assertion checks, more verbose logging level and enables printing
  extended details in the log messages (e.g. dumps of all parameters for some
  functions).

However, please ensure that the publicly released apps are always using the
**Release** mode. Otherwise, the **user's privacy may be harmed** as the debug
log messages may contain sensitive data.

The Release mode is the default building mode; you can switch to it back from
the Debug build by adjusting the `CONFIG` environment variable, for example:

```shell
export CONFIG=Release
```

or simply:

```shell
unset CONFIG
```
