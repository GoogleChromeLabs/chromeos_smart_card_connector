# Updating the CCID free software driver

Note: This page is mainly intended for the ChromeOS Smart Card Connector
project maintainers.


## Background

The ChromeOS Smart Card Connector app is shipped with a specific (pinned)
version of the CCID free software driver. This is a driver for smart card
readers (not to be confused with drivers for smart cards, also sometimes called
middleware).

Reasons for pinning to a specific version of the driver:

* Potentially, adjustments in our code or in the building scripts might be
  required after updating to new versions (note that we don't run the driver
  verbatim, but instead port it under the WebAssembly and Native Client
  technologies to run inside the JavaScript-based App);
* We can test the driver version and assure its quality on ChromeOS.

However, this approach also implies some amount of maintenance work to update
the driver (say, every few months). It also means that the users typically get a
somewhat outdated version of the driver.


## Update procedure

0. The currently used version of the driver is located under the
   `//third_party/ccid/` directory. Specifically, the
   `//third_party/ccid/README.google` file contains the version number, and the
   `//third_party/ccid/src/` directory contains the original files of
   the driver plus some (minor) modifications.

1. Download a new release of the CCID free software drivers from the official
   project page: [https://ccid.apdu.fr/](https://ccid.apdu.fr/).

2. Clean the compilation artifacts from the old version of the CCID driver by
   running this command in the `//third_party/ccid/webport/build/` directory:
   
   ```shell
   CONFIG=Debug make clean && CONFIG=Release make clean
   ```

3. Delete all files of the previous version from `//third_party/ccid/src/`, and
   unpack the new files from the downloaded archive into this directory. (Hint:
   Pay attention to the directory structure; e.g., you should have the `AUTHORS`
   file located at `//third_party/ccid/src/AUTHORS` in the end.)

4. Apply the patches from the `//third_party/ccid/patches/` directory to the
   files unpacked at the previous step. (Hint: We only have a single tiny patch
   currently, so probably it's the simplest to just manually apply the
   modification shown in that patch file.)

5. Edit the URL and the version in the `//third_party/ccid/README.google` file.

6. Edit the version in the `//third_party/ccid/webport/include.mk` file.

7. Compile the Smart Card Connector app and all other targets, by running the
   `make-all.sh` script.

8. In case of any errors discovered at the previous step, investigate and fix
   them. (Hint: In case a new file was created in the driver, you have to add it
   into `//third_party/ccid/webport/build/Makefile` into one of the sections.)

9. Start the just-built Smart Card Connector app (e.g., by running `make run` in
   the `//smart_card_connector/build/` directory) and perform some manual QA
   testing using physical reader(s).

10. Check the logs from the App and double-check that they mention the correct
    version of the driver (it should be a string like
    `init_driver() Driver version: 1.4.30`).
    
    Troubleshooting: in case a wrong version is still mentioned somewhere, or
    some other correction was needed, you can force the CCID recompilation by
    repeating the step #2.

11. If everything is OK, prepare a commit / a Pull Request with all these
    changes.

Additional tasks to be performed later, when rolling out a new version of the
Smart Card Connector app via Web Store:

1. Mention the update of the CCID driver at the Release's description on GitHub.
   It's also nice to add a link to the CCID release notes (which are typically
   published at
   [https://blog.apdu.fr/](https://blog.apdu.fr/).

2. File an internal bug to update the
   [https://support.google.com/chrome/a/answer/7014689](https://support.google.com/chrome/a/answer/7014689)
   Help Center article to include the new information from the
   `//smart_card_connector_app/build/human_readable_supported_readers.txt` file.
   Please make sure that the newly added readers are annotated with the current
   version, like `(from version 1.23.45)`.
