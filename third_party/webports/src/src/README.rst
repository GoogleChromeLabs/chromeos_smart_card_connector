naclports - Ports of open source software to Native Client
==========================================================

naclports is collection of open source libraries and applications that have
been ported to Native Client, along with set to tools for building and
maintaining them.

Packages can be built from source or prebuilt binaries packages can be
downloaded from the continuous build system.

The sources for the ports live in the ``ports`` directory.  Each one contains
at least the following file:

- pkg_info: a description of the package.

Most also contain the follow optional files:

- build.sh: a bash script for building it
- nacl.patch: an optional patch file.

The tools for building packages live in ``bin``.  The binary tool is simple
called ``naclports``.  To build and install a package into the toolchain run
``naclports install <package_dir>``.  This script will download, patch, build
and install the application or library.  By default it will first install any
dependencies that that the package has.


Links
-----

Project home: https://code.google.com/p/naclports/
Continuous builder: http://build.chromium.org/p/client.nacl.ports/
Continuous build artifacts: http://gsdview.appspot.com/naclports/builds/


Prerequistes
------------

The minimum requirements for using naclports are:

- python 2.7
- gclient (from depot_tools)
- Native Client SDK

For building packages from source the build scripts require that certain tools
are present in the host system:

- bash
- make
- curl
- sed
- git

To build all ports you will also need these:

- cmake
- texinfo
- gettext
- pkg-config
- autoconf, automake, libtool
- libglib2.0-dev >= 2.26.0 (if you want to build glib)
- xsltproc

On Mac OS X you can use homebrew to install these using the following command::

  brew install autoconf automake cmake gettext libtool pkg-config

The build system for some of the native Python modules relies on a 32-bit
host build of Python itself, which in turn relies on the development version
of zlib and libssl being available.  On 64-bit Ubuntu/Trusty this means
installing:

- zlib1g-dev:i386
- libssl-dev:i386

On older Debian/Ubuntu systems these packages were known as:

- lib32z1-dev
- libssl0.9.8:i386


Building
--------

Before you can build any of the package you must set the ``NACL_SDK_ROOT``
environment variable to top directory of a version of the Native Client SDK
(the directory containing toolchain/). This path should be absolute.

The top level Makefile can be used as a quick way to build one or more
packages. For example, ``make libvorbis`` will build ``libvorbis`` and
``libogg``. ``make all`` will build all the packages.

There are 4 possible architectures that NaCl modules can be compiled for: i686,
x86_64, arm, pnacl. The naclports build system will only build just one at at
time. You can control which one by setting the ``NACL_ARCH`` environment
variable. e.g.::

  $ NACL_ARCH=arm make openssl

For some architectures there is more than one toolchain available.  For example
for x86 you can choose between clang-newlib and glibc.  The toolchain defaults
to pnacl and can be specified by setting the ``TOOLCHAIN`` environment
variable::

  $ NACL_ARCH=i686 TOOLCHAIN=glibc make openssl

If you want to build a certain package for all architectures and all toolchains
you can use the top level ``make_all.sh`` script. e.g.::

  $ ./make_all.sh openssl

Headers and libraries are installed into the toolchains directly so there is
not add extra -I or -L options in order to use the libraries.

The source code and build output for each package is placed in::

  out/build/<PACKAGE_NAME>

By default all builds are in release configuration.  If you want to build
debug packages set ``NACL_DEBUG=1`` or pass ``--debug`` to the naclports
script.

**Note**: Each package has its own license. Please read and understand these
licenses before using these packages in your projects.

**Note to Windows users**: These scripts are written in bash and must be
launched from a Cygwin shell. While many of the scripts should work under
Cygwin, naclports is only tested on Linux and Mac so YMMV.


Binary Packages
---------------

By default naclports will attempt to install binary packages rather than
building them from source. The binary packages are produced by the buildbots
and stored in Google cloud storage. The index of current binary packages
is stored in ``lib/prebuilt.txt`` and this is currently manually updated
by running ``build_tools/scan_packages.py``.

If the package version does not match the package will always be built from
source.

If you want to force a package to be built from source you can pass
``--from-source`` to the naclports script.


Emscripten Support
------------------

The build system contains very early alpha support for building packages
with Emscripten.  To do requires the Emscripten SDK to be installed and
configured (with the Emscripten tools in the PATH).  To build for Emscripten
build with ``TOOLCHAIN=emscripten``.


Running the examples
--------------------

Applications/Examples that build runnable web pages are published to
``out/publish``. To run them in chrome you need to serve them with a web
server.  The easiest way to do this is to run::

  $ make run

This will start a local web server serving the content of ``out/publish``
after which you can navigate to http://localhost:5103 to view the content.


Happy porting!
