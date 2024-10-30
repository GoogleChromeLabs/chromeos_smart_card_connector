# Updating the PC/SC-Lite daemon

Note: This page is mainly intended for the ChromeOS Smart Card Connector
project maintainers.


## Background

The ChromeOS Smart Card Connector app is shipped with a specific (pinned)
version of the PC/SC-Lite daemon. This daemon exposes the PC/SC API to client
extensions/apps, transforming this API calls into commands to the drivers
(currently, we have only one driver - the CCID Free Software driver).

Reasons for pinning to a specific version of the daemon:

* Potentially, adjustments in our code or in the building scripts might be
  required after updating to new versions (note that we don't run the daemon
  verbatim, but instead port it under the Native Client technology to run inside
  the JavaScript-based App);
* We can test the daemon version and assure its quality on ChromeOS.

However, this approach also implies some amount of maintenance work to update
the daemon (say, every few months). It also means that the users typically get a
somewhat outdated version of the PC/SC-Lite daemon.


## Update procedure

0. The currently used version of the daemon is located under the
   `//third_party/pcsc-lite/` directory. Specifically, the
   `//third_party/pcsc-lite/README.google` file contains the version number, and
   the `//third_party/pcsc-lite/src/` directory contains the original files of
   the daemon.

1. Download a new release of the PC/SC-Lite daemon from the official project
   page: [https://pcsclite.apdu.fr/](https://pcsclite.apdu.fr/).

2. Clean the compilation artifacts from the old version of the PC/SC-Lite daemon
   by running this command in the directories related to PC/SC-Lite:

   ```shell
   CONFIG=Debug make clean && CONFIG=Release make clean
   ```

   The list of directories where this has to be done:

   * `//third_party/pcsc-lite/webport/common/build/`
   * `//third_party/pcsc-lite/webport/cpp_client/build/`
   * `//third_party/pcsc-lite/webport/cpp_demo/build/`
   * `//third_party/pcsc-lite/webport/server/build/`
   * `//third_party/pcsc-lite/webport/server_clients_management/build/`

3. Delete all files of the previous version from `//third_party/pcsc-lite/src/`,
   and unpack the new files from the downloaded archive into this directory.
   (Hint: Pay attention to the directory structure; e.g., you should have the
   `AUTHORS` file located at `//third_party/pcsc-lite/src/AUTHORS` in the end.)

4. Apply the patches from the `//third_party/pcsc-lite/patches/` directory to
   the files unpacked at the previous step. (Hint: We only have a single tiny
   patch currently, so probably it's the simplest to just manually apply the
   modification shown in that patch file.)

5. Edit the URL and the version in the `//third_party/pcsc-lite/README.google`
   file.

6. Compile the Smart Card Connector app and all other targets, by running the
   `make-all.sh` script.

7. In case of any errors discovered at the previous step, investigate and fix
   them. (Hint: In case a new file was created in the daemon, you have to add it
   into `//third_party/pcsc-lite/webport/server/build/Makefile` into one of the
   sections.)

8. Start the just-built Smart Card Connector app (e.g., by running `make run` in
   the `//smart_card_connector/build/` directory) and perform some manual QA
   testing using physical reader(s).

9. If everything is OK, prepare a commit / a Pull Request with all these
   changes.

Additional tasks to be performed later, when rolling out a new version of the
Smart Card Connector app via Web Store:

1. Mention the update of the PC/SC-Lite daemon at the Release's description on
   GitHub. It's also nice to add a link to the PC/SC-Lite release notes (which
   are typically published at
   [https://blog.apdu.fr/](https://blog.apdu.fr/)).
