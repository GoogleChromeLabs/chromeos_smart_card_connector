Contibuting to naclports
========================

The naclports project welcomes contributions.  The most common forms of
contribution are new ports, or updates to existing ports.

Before we can use use your code, you must sign the `Google Individual
Contributor License Agreement
<https://developers.google.com/open-source/cla/individual?csw=1>`_ (CLA),
which you can do online. The CLA is necessary mainly because you own the
copyright to your changes, even after your contribution becomes part of our
codebase, so we need your permission to use and distribute your code. We also
need to be sure of various other things—for instance that you'll tell us if you
know that your code infringes on other people's patents. You don't have to sign
the CLA until after you've submitted your code for review and a member has
approved it, but you must do it before we can put your code into our codebase.
Before you start working on a larger contribution, you should get in touch with
us first through the issue tracker with your idea so that we can help out and
possibly guide you. Coordinating up front makes it much easier to avoid
frustration later on.

Once you have a change that you would like to submit you must upload it for
review using::

  $ git cl upload

This will upload the change to the code review tool.  From there you can send it
out for review via the web interface (you can also do this from the command line
if you prefer).  Once you have an 'lgtm' you can use the commit queue (CQ button
in the review tool) to have your change submitted.

Adding a new package
--------------------

To add a package:

1. Add a directory to the ``ports`` directory using the name your new package.
   For example: ``ports/openssl``.
2. Add the build.sh script and pkg_info to that directory.
3. Optionally include the upstream tarball and add its sha1 checksum to
   pkg_info. You can do this using ``build_tools/sha1sum.py``.  Redirect the
   script to append to the pkg_info file.  e.g.::

     $ sha1sum.py mypkg.tar.gz >> ports/openssl/pkg_info

4. Optionally include a patch file (nacl.patch). See below for the
   recommended way to generate this patch.
5. Make sure your package builds for all architectures::

     $ ./make_all.sh <PACKAGE_NAME>

Writing build scripts
---------------------

Each port has an optional build script: ``build.sh``.  Some ports, such as
those that are based on autotools+make don't need a build script at all. The
build script is run in a bash shell, it can set variables at the global scope
that override the default behaviour of various steps in the build process. The
most common steps that implement by package-specific scripts are:

- ConfigureStep()
- BuildStep()
- InstallStep()
- TestStep()

When implementing a given step the default step can be still invoked, e.g.
by calling DefaultBuildStep() from within BuildStep()

Each build is is run independently in a subshell, so variables set in one
step are not visible in others, and changing the working directory within a
step will not effect other steps.

A variety of shared variables and functions are available from with the build
scripts.  These are defined in build_tools/common.sh.

Modifying package sources / Working with patches
------------------------------------------------

When a package is first built, its source is downloaded and extracted to
``out/build/<pkg_name>``. A new git repository is then created in this
folder with the original archive contents on a branch called ``upstream``. The
optional ``nacl.patch`` file is then applied on the ``master`` branch. This
means that at any given time you can see the changes from upstream using ``git
diff upstream``.

To make changes to a package's patch file the recommended workflow is:

1. Directly modify the sources in ``out/build/<pkg_name>``.
2. Build the package and verify the changes.
3. Use ``naclports updatepatch <pkg_name>`` to (re)generate the patch file.

Whenever the upstream archive or patch file changes and you try to build the
package you will be prompted to remove the existing repository and start a new
one. This is to avoid deleting a repository that might contain unsaved changed.

Coding Style
------------

For code that is authored in the naclports repository (as opposed to patches)
we follow the Chromium style guide:
http://www.chromium.org/developers/coding-style.

C/C++ code can be automatically formatted with Chromium's clang-format:
https://code.google.com/p/chromium/wiki/ClangFormat. If you have checkout of
Chromium you can set CHROMIUM_BUILDTOOLS_PATH=<chromium>/src/buildtools
which will enable the clang-format script in depot_tools to find the binary.

Python code can be automatically formatted with the ``yapf`` tool which is
automatically downloaded during ``gclient sync``. e.g::

  bin/yapf -i <path/to/my/file.py>

Shell scripts
~~~~~~~~~~~~~

When modifying any shell scripts in naclports it is recommended that you
run ``shellcheck`` to catch common errors.  The recommended command line
for this is::

  shellcheck -e SC2044,SC2129,SC2046,SC2035,SC2034,SC2086,SC2148 \
    `git ls-files "*.sh"`

Commit Messages
~~~~~~~~~~~~~~~

Where possible try to follow the generally accepted best practices for git
commit messages.  That is, a single subject line of 50 characters or less
followed by a blank line, followed by a longer description wrapped at 72
characters.  For more information of crafted good commit messages see:
http://chris.beams.io/posts/git-commit/


Happy porting!
