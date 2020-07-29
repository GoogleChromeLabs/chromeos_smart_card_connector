# Updating the CCID free software driver

Note: This page is mainly intended for the Chrome OS Smart Card Connector
project maintainers.

## Background

The Chrome OS Smart Card Connector app is shipped with a specific version of the
CCID free software driver. This is a driver for smart card reader (not to be
confused with drivers for smart cards, also sometimes called middleware).

Reasons for pinning to a specific version of the driver:

* potentially, adjustments in our code or in the building scripts might be
  required after updating to new versions (note that we don't run the driver
  verbatim, but instead port it under the Native Client technology for running
  inside the JavaScript-based App);
* we can make sure that the driver that we roll out as part of the App underwent
  some QA testing.

However, this approach also implies some amount of maintenance work for updating
the driver (say, every few months). It also means that the users typically get
a somewhat outdated version of the driver.

## Update procedure

0. The currently used version of the driver is located under the
   `//third_party/ccid/` directory. Specifically, the
   `//third_party/ccid/README.google` file should contain the version number,
   and the `//third_party/ccid/src/` directory contains the original files of
   the driver plus some (minor) modifications.

1. Download a new release of the CCID free software drivers from the official
   project page: [https://ccid.apdu.fr/](https://ccid.apdu.fr/).

2. Delete all files of the previous version from `//third_party/ccid/src/`, and
   unpack the new files from the downloaded archive into this directory. (Hint:
   Pay attention to the directory structure; e.g., you should have the `AUTHORS`
   file located at `//third_party/ccid/src/AUTHORS` in the end.)

3. Apply the patches from the `//third_party/ccid/patches/` directory to the
   files unpacked at the previous step. (Hint: We only have a single tiny patch
   currently, so probably it's the simplest to just manually make the
   modification shown in that patch file.)

4. Edit the URL and the version in the `//third_party/ccid/README.google` file.

5. Edit the version in the `//third_party/ccid/naclport/include.mk` file.

6. Bump the file timestamps for the newly created files, in order to enforce the
   `make` to recompile the CCID (otherwise, `make` will skip the files that have
   old timestamps taken from the downloaded archive). To do this, for example,
   run this in the terminal: `find third_party/ccid -exec touch {} \;`

7. Compile the Smart Card Connector app and all other targets, by running the
   `make-all.sh` script.

8. In case of any errors discovered at the previous step, investigate and fix
   them. (Hint: In case a new file was created in the driver, you have to add it
   into `//third_party/ccid/naclport/build/Makefile` into one of the sections.)

9. Start the just-built Smart Card Connector app (e.g., by running `make run` in
   the `//smart_card_connector/build/` directory) and perform some manual QA
   testing using physical reader(s).

10. Check the logs from the App and double-check that they mention the correct
    version of the driver (it'll be a string like
    `init_driver() Driver version: 1.4.30`).

11. If everything is OK, prepare a commit / a Pull Request with all these
    changes.

100. Later, when rolling out a new version of the Smart Card Connector app via
     Web Store, please don't forget to mention the update of the CCID driver at
     the Release's description on GitHub. It's also nice to add a link to the
     CCID release notes (which are typically published at
     [https://ludovicrousseau.blogspot.com/](https://ludovicrousseau.blogspot.com/)).
