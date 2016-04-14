Example C++ Smart Card Client App
=================================


Overview
--------

This is the example of a client App that demonstrates:

*   Hooking into the **chrome.certificateProvider** Chrome JavaScript
    API (<https://developer.chrome.com/extensions/certificateProvider>).

*   Utilizing of the **PC/SC-Lite API**
    (<http://pcsclite.alioth.debian.org/api/group__API.html>) provided
    by the Connector App.

Note that the App is just a **skeleton** of an actual client App. It
does not contain any actual cryptographic code or any code that operates
with the smart cards. Instead, the entry points for where such code is
expected to be located are stubbed out. The entry points are:

*   **Handling the certificates request**.

    This is a request that is, conceptually, sent to the App by the
    Chrome browser's network stack, when it wants to obtain (or refresh)
    the list of certificates provided by the App.

    From the implementation prospective, this request arrives in the
    form of the special
    chrome.certificateProvider.onCertificatesRequested event (see
    <https://developer.chrome.com/extensions/certificateProvider#event-onCertificatesRequested>)
    which is then forwarded by the App's code to the C++ handler in the
    NaCl module (see the file ``src/pp_module.cc``).

*   **Handling the sign digest request**.

    This is a request that is, conceptually, sent to the App by the
    Chrome browser's network stack, when it needs to perform the digest
    signing operation by a private key associated with the certificate
    provided by the App.

    From the implementation prospective, this request arrives in the
    form of the special chrome.certificateProvider.onSignDigestRequested
    event (see
    <https://developer.chrome.com/extensions/certificateProvider#event-onSignDigestRequested>)
    which is then forwarded by the App's code to the C++ handler in the
    NaCl module (see the file ``src/pp_module.cc``).

Additionally, the example App contains the following pieces:

*   Linking against the library that provides an **implementation** for
    the functions defined in the original **PC/SC-Lite** public header
    files (the library is located under the
    ``/third_party/pcsc-lite/naclport/cpp_client/`` directory).

    The PC/SC-Lite public header files are taken without any
    modifications. The implementation provided by the library tries to
    follow the original PC/SC-Lite library behavior as closely as
    possible.

    However, there are some unavoidable peculiarities:

    *   PC/SC-Lite API function calls may be issued only after the
        library is initialized.

        (The initialization is performed inside the constructor of the
        ``PpInstance`` class in the file ``src/pp_module.cc``.)

    *   No PC/SC-Lite API function calls should be made on the main
        thread.

        (For the justification, please refer to comments in the file
        ``src/pp_module.cc``.)

    Apart from the described situations, PC/SC-Lite API function calls
    may be issued at any time on any number of threads.

*   Creation of a separate background thread in the NaCl module that
    performs a sequence of commands **demonstrating the use of
    PC/SC-Lite API** (see the file ``src/pp_module.cc``).

    Note that this piece is here just for the demostration purposes, so
    it can be removed safely.

*   A **PIN entry dialog** together with a corresponding C++ binding for
    convenience.

    In the demonstration purposes, the PIN entry dialog is shown at the
    end of the PC/SC-Lite demo. Once again, this piece can be removed
    safely.


Building
--------

First, please make sure that all building prerequisites are provided and
the building environment is set up - please refer to *Common building
prerequisites*, *Building*, *Debug and Release building modes* sections
of the ``README`` file located in the project root.

After that, **rebuilding** of the App can be performed by the following
command running under the ``build`` directory::

    make -j20


Debug and Release building modes
--------------------------------

For the general description of the building modes, please refer to the
*Debug and Release building modes* section of the ``README`` file
located in the project root.

One additional note is that the Debug mode in this App also turns on the
tracing of all PC/SC-Lite API function calls, which includes logging of
their input parameters and logging of the returned values.


Running
-------

There are two options: either running the App on a Chromebook or running
it locally in a desktop Chrome browser. For the discussion, please see
the *Common building prerequisites* and *Troubleshooting Apps under
desktop OSes* sections in the ``README`` file located in the project
root.

*   In order to run the App on a **Chromebook**, it has first to be
    **packaged** first (see
    <https://developer.chrome.com/extensions/packaging>).

    *   One option is to invoke the special make target that generates
        the packaged App archive for you::

            make package -j20

    *   Another option is to create the App's package manually through
        the Chrome browser.

        Follow the instructions at
        <https://developer.chrome.com/extensions/packaging#creating> for
        creating the App's package manually. The built App's root
        directory is located under the ``build/out/`` directory.

*   In order to run the App **locally in a desktop Chrome browser**, the
    following options are available:

    *   One option is to invoke the special make target that runs the
        App in Chrome with the temporary local Chrome profile::

            make run -j20

    *   Another option is to install the built App manually into Chrome.

        Follow the instructions at
        <https://developer.chrome.com/extensions/getstarted#unpacked>
        for installing the App into Chrome.

        If you would like to start a separate Chrome instance with a
        temporary local Chrome profile, the following command may be
        used::

            <path/to/chrome/binary> \
                --enable-nacl \
                --enable-pnacl \
                --no-first-run \
                --user-data-dir=user-data-dir \


Borrowing the Example App's code for building custom client Apps
----------------------------------------------------------------

The ``Example C++ Smart Card Client App`` can be used as a skeleton for
building custom smart card client Apps.

Follow these steps:

*   Copy the ``/example_cpp_smart_card_client_app/`` directory contents
    under a different name.

*   Open the file ``build/Makefile`` and edit the value of the
    ``TARGET`` variable.

*   Open the file ``src/background.js`` and edit the value of the
    ``CLIENT_TITLE`` variable.

*   Open the file ``src/_locales/en/messages.json`` and edit the values
    of the ``appName`` and the ``appDesc`` messages.
