Smart Card Connector App for Chrome OS
======================================


Overview
--------

The project aim is to bring the support of smart cards (see
<https://en.wikipedia.org/wiki/Smart_card>) into Chrome OS (see
<http://www.chromium.org/chromium-os>) by utilizing the already existing
Chrome APIs, mainly:

*   **chrome.usb** API (see <https://developer.chrome.com/apps/usb>),
    that provides a way to interact with connected USB devices.

*   **chrome.certificateProvider** API (see
    <https://developer.chrome.com/extensions/certificateProvider>), that
    provides a way to inject certificates into the browser's network
    stack that can be used for mutual TLS authentication.

Using these APIs allows developers to implement a smart card stack
without the need to modify the Chrome OS and/or Chrome. Instead, all the
new code is implemented in the form of Chrome Apps (see
<https://developer.chrome.com/apps/about_apps>).

The high-level overview of the architecture is:

1.  The App named "**Smart Card Connector**" (available on Chrome Web
    Store at
    <https://chrome.google.com/webstore/detail/khpfeaanjngmcnplbdlpegiifgpfgdco>).

    The App is basically an implementation of the PC/SC API (see
    <https://en.wikipedia.org/wiki/PC/SC>). The API allows middleware
    Apps to operate smart card readers through a unified interface.

    The API is exposed to other Apps through a message-exchange
    protocol (see <https://developer.chrome.com/apps/messaging>). For
    the details of the API, see the *Smart Card Connector App API*
    section below.

    The App is bundled with the **free USB CCID driver** (see
    <https://ccid.apdu.fr/>). This driver is
    patched appropriately in order to be able to work through the
    **chrome.usb** Chrome API.

    *Note*: no other smart card **reader** drivers except the previously
    mentioned free USB CCID driver are supplied. Support of pluggable
    drivers is also not implemented yet, though it's possible
    technically (each driver can be created as a separate App talking to
    the Connector App through some defined interface). A list of readers
    supported by CCID can be found on their website
    <https://ccid.apdu.fr/ccid/section.html>.

2.  A number of **third-party middleware Apps** containing the smart
    card drivers.

    The middleware Apps are written based on the PC/SC API provided by
    the Connector App.

    The actual features implemented by the Apps may vary, but the
    typical example would be to read the certificates stored on the
    smart card, inject them into the browser's network stack (using the
    **chrome.certificateProvider** Chrome API) and proxy digest signing
    operations occurring during TLS handshake phase to the smart card.


Smart Card Connector App
------------------------

As was mentioned before, the Connector App acts as the provider of the
**PC/SC API** to third-party Apps.

The App plays basically the same role as the **PC/SC-Lite Daemon** does
on Linux (see <http://linux.die.net/man/8/pcscd>). And, actually, the
App sources are heavily based on the **PC/SC-Lite middleware** sources
(see <https://salsa.debian.org/rousseau/PCSC>).

For the details of the internal structure and the implementation of the
Smart Card Connector App, please refer to its own ``README`` file (see
the ``smart_card_connector_app/`` directory).


Smart Card Connector App API Permissions
----------------------------------------

Due to various privacy and security concerns, the following decisions
were made:

1.  When a middleware App tries to talk to the Connector App, a **user
    prompt dialog** is generally shown, asking whether to allow or to
    block this middleware App.

    If the user decides to allow the middleware App, the decision is
    remembered in the Connector App's local storage, and the requests
    made by the middleware App start being actually executed by the
    Connector App. Otherwise, all the requests made by the middleware
    App are refused.

    Note: this system has nothing to do with the Chrome App permissions
    model (see <https://developer.chrome.com/apps/declare_permissions>),
    as Chrome allows to use the messaging API without any additional
    permissions. This intent is to close the hole left open by cross-app
    messaging between Apps with different permissions.

2.  The Connector App is **bundled with a whitelist of known middleware
    App identifiers** (and a mapping to their display names). For all
    middleware Apps not from this list, the user prompt will contain a
    big scary warning message.

    (Note: This behavior was introduced in the Smart Card Connector app
    version 1.2.7.0. Before this version, all requests from unknown apps
    were silently ignored.)

3.  For the **enterprise** cases, it's possible to configure the
    Connector App through an **admin policy** (see
    <https://www.chromium.org/administrators/configuring-policy-for-extensions>).

    The policy can specify which middleware Apps are **force allowed to
    talk to the Connector App** without checking against whitelist or
    prompting the user. The policy-configured permission always has the
    higher priority than the user's selections that could have been
    already made.

    The corresponding policy name is ``force_allowed_client_app_ids``.
    Its value should be an array of strings representing the App
    identifiers. This is an example of the policy JSON blob::

        {
          "force_allowed_client_app_ids": {
            "Value": [
              "this_is_middleware_client_app_id",
              "this_is_some_other_client_app_id"]
          }
        }


Smart Card Connector App API
----------------------------

The API exposed by the Connector App is basically a PC/SC-Lite API (see
<https://pcsclite.apdu.fr/api/group__API.html>) adopted for
the message-exchanging nature of the communication between Chrome Apps
(see <https://developer.chrome.com/apps/messaging>).

The **PC/SC-Lite API function call** is represented by a message sent to
the Connector App. The message should have the following format::

    {
      "type": "pcsc_lite_function_call::request",
      "data": {
        "request_id": <requestId>,
        "payload": {
          "function_name": <functionName>,
          "arguments": <functionArguments>
        }
      }
    }

where ``<requestId>`` should be a number (unique in the whole session of
the middleware App communication to the Connector App),
``<functionName>`` should be a string containing the PC/SC-Lite API
function name, ``<functionArguments>`` should be an array of the input
arguments that have to be passed to the PC/SC-Lite API function.

The **results** returned from the PC/SC-Lite API function call are
represented by a message sent back from the Connector App to the
middleware App.

If the request was processed **successfully** (i.e. the PC/SC-Lite
function was recognized and called), then the message will have the
following format::

    {
      "type": "pcsc_lite_function_call::response",
      "data": {
        "request_id": <requestId>,
        "payload": <results>
      }
    }

where ``<requestId>`` is the number taken from the request message, and
``<results>`` is an array of the values containing the function return
value followed by the contents of the function output arguments.

If the request **failed** with some error (note: this is *not* the case
when the PC/SC-Lite function returns non-zero error code), then the
message will have the following format::

    {
      "type": "pcsc_lite_function_call::response",
      "data": {
        "request_id": <requestId>,
        "error": <errorMessage>
      }
    }

where ``<requestId>`` is the number taken from the request message, and
``<errorMessage>`` is a string containing the error details.

Additionally, Apps on both sides of the communication channel can send
**ping** messages to each other::

    {
      "type": "ping",
      "data": {}
    }

The other end should response with a **pong** message having the
following format::

    {
      "type": "pong",
      "data": {
        "channel_id": <channelId>
      }
    }

where ``<channelId>`` should be the number generated randomly in the
beginning of the communication.

Pinging allows to track whether the other end is still alive and
functioning (Chrome's long-lived messaging connections, when they are
used, are able to detect most of the cases - but the one-time messages
passing API is also allowed to be used). The ``<channelId>`` field value
allows the other end to track cases when the App died and restarted
while a response from it was awaited.

For simplifying the middleware Apps development, the **wrapper
libraries** for **JavaScript** and **C** are provided (the latter one is
basically an implementation of the functions defined in the original
PC/SC-Lite headers). See the corresponding example Apps for the details
(the ``example_js_smart_card_client_app/`` and the
``example_cpp_smart_card_client_app/`` directories), and the standalone
JavaScript library (see the
``example_js_standalone_smart_card_client_library/`` directory).


Common building prerequisites
-----------------------------

Following are the common **prerequisites** required for building of the
Apps:

*   **OS: Linux**.

    Building under different \*nix system, Mac OS or Windows should be
    possible too, though most probably will require more efforts.

*   The following tools should be present in the system: **bash**,
    **make**, **curl**, **sed**, **mktemp**, **realpath**, **xxd**.

*   **Python 2.7**, including the dev package.

    Python 3.x is not supported yet.

*   **git** (version 2.2.1+ is recommended).

*   **OpenSSL** (version 1.0+ is recommended).

*   (for 64-bit Linux) **32-bit version of libstdc++**.

    For example, on Ubuntu it's provided by the libstdc++6:i386 package.

*   **Java Runtime Environment 7**.

In order to **run** the built Apps, you will need *either* of these:

*   a **Chromebook** with Chrome OS >= 48.

    This will provide the closest environment to the real world's one.

    However, the disadvantage of this option is the inconvenient way of
    doing short development iterations: each time the built Apps will
    have to be somehow transferred to the Chromebook and installed onto
    it.

*   a locally installed **Chrome** browser with version >= 48.

    This option will save time during development, allowing to install
    and run the Apps easily on the local machine.

    For convenience, each App's Makefile provides a special ``run``
    target that creates a temporary local Chrome profile and runs the
    browser with having the App installed and run into it. This allows
    to test the Apps locally, without interfering with the real Chrome
    profile.

    One downside of this option is that the desktop Chrome does not
    provide all the APIs that are provided under Chrome OS. The most
    noticeable example is the **chrome.certificateProvider** API: it's
    only available under Chrome OS, so its usages in the Apps will have
    to be stubbed out when executing locally.

    Another downside is that the desktop OS may require additional setup
    in order to allow Chrome (and, consequently, the Apps being executed
    in it) to access the USB devices. Some instructions are given in the
    *Troubleshooting Apps under desktop OSes* below.


Building
--------

Follow these steps for performing the *initial build*:

1.  Execute::

         env/initialize.sh

    It's enough to execute this command only once, after you have cloned
    the whole repository (unless you would like to update to the latest
    tools versions).

    This will download and install locally the following dependencies
    required for building the Apps:

    *   *depot_tools* (see
        <https://commondatastorage.googleapis.com/chrome-infra-docs/flat/depot_tools/docs/html/depot_tools.html>)
    *   *NaCl SDK* (see
        <https://developer.chrome.com/native-client/sdk/download>)
    *   *webports* (see <https://chromium.googlesource.com/webports/>)

2.  Execute::

        source env/activate

    This command sets the environment variables required for enabling
    the use of the tools downloaded at step 1.

3.  Execute::

        ./make-all.sh

    This builds the Connector App, the C++ Example App and the JS
    Example App and all the libraries shared between them.

After that, you can *perform incremental building* of either all of the
Apps (by running the command from step 3.) or of the single App you work
on (by following its build instructions).

You should only make sure, however, that the environment definitions are
always here - and, if not, use the command from step 2 for setting them
up back.


Debug and Release building modes
--------------------------------

During the development process, it's useful to enable the extended
levels of logging and (depending on the actual App) the more extensive
debug assertions checks.

Switching to the **Debug** building mode can be performed by adjusting
the ``CONFIG`` environment variable, i.e. by executing the following
shell command before building the Apps::

    export CONFIG=Debug

This triggers a number of things, basically (for some additional details
regarding concrete Apps refer to their own ``README`` files):

*   For the compiled JavaScript code - enables the creation of the
    source map allowing to view the uncompiled code when debugging.

*   For the JavaScript code built using the Closure library logging
    subsystem - selects more verbose logging level by
    default and enables printing extended details in the log messages
    (e.g. dumps of all parameters for some functions).

*   For the C/C++ code - undefines the ``NDEBUG`` macro, which enables
    some extended debug assertion checks, more verbose logging level and
    enables printing extended details in the log messages (e.g. dumps of
    all parameters for some functions).

However, please ensure that the publicly released Apps are always using
the **Release** mode. Otherwise, the **user's privacy may be harmed** as
the debug log messages may contain sensitive data.

The Release mode is the default building mode; you can switch to it back
from the Debug build by adjusting the ``CONFIG`` environment variable,
for example::

    export CONFIG=Release

or simply::

    unset CONFIG


Troubleshooting Apps under desktop OSes
---------------------------------------

Despite that the target platform of the Apps is Chrome OS, most of their
functions can work correctly when run under desktop OSes (i.e. Linux,
Windows, etc.).

However, there may be some limitations and difficulties met when working
under desktop OSes:

*   **chrome.certificateProvider Chrome API is unavailable**.

    This is working as intended. This Chrome API, along with several
    others, is provided only on Chrome OS (see the Chrome App APIs
    documentation at <https://developer.chrome.com/apps/api_index>).

    The usages of such APIs will have to be stubbed out when running
    under desktop OSes.

*   **On \*nix systems, a system-wide PCSCD daemon may prevent Chrome
    from accessing the USB devices**.

    The simplest solution is to stop the system-wide daemon.

    For example, under Ubuntu this can be done with the following
    command::

        sudo service pcscd stop

    On macOS systems (since at least El Capitan, 10.11) the pcscd daemon
    has been replaced by a daemon called com.apple.ifdreader. You can
    stop it using::

        sudo pkill -HUP com.apple.ifdreader

*   **On \*nix systems, the USB device file permissions may prevent
    Chrome from accessing the device**.

    The simplest solution, described below, is to give the writing
    permissions for the USB device file to all users; note that,
    however, this is **unsafe on multi-user systems**!

    So granting the write access for all users can be performed in
    two ways:

    *   One quick option is to add the permissions manually::

            sudo chmod 666 /dev/bus/usb/<BUS>/<DEVICE>

        Where ``<BUS>`` and ``<DEVICE>`` numbers can be taken, for
        example, from the output of the lsusb tool::

            lsusb

    *   Another, more robust, option is to add a udev rule (see, for
        example, the documentation at
        <https://www.kernel.org/pub/linux/utils/kernel/hotplug/udev/udev.html>).

*   **On Windows, a generic USB driver may be required to make the smart
    card reader devices available to Chrome**.

    For example, this can be done with the **Zadig** tool (Note: this is
    a third-party application that is not affiliated with Google in any
    way. **Use at your own risk!**): <http://zadig.akeo.ie>.
