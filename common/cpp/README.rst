common/cpp library
==================


This library contains some common primitives for being used in the C/C++
code of the Smart Card Apps.

The main pieces are:

*   **Logging subsystem** (see the `src/logging/` directory), that
    provides a convenient way of formatting log messages, support of
    different log verbosity levels, emitting of log messages to both
    `stderr` stream and the JS console (through the counterpart in the
    `/common/js/` library).

    For the information on the standard NaCl tools for logging, see
    <https://developer.chrome.com/native-client/devguide/devcycle/debugging#basic-debugging>.

*   **Pepper values utility library** (see the `src/pp_var_utils/`
    directory), that provides convenient tools for operating with Pepper
    values and converting them to/from the C/C++ values (of standard and
    user-defined types).

*   **Pepper messaging helper library** (see the `src/messaging/`
    directory), that implements the Observer pattern on top of the
    Pepper messaging system.

    For the reference of the Pepper value types, see
    <https://developer.chrome.com/native-client/pepper_dev/cpp/classpp_1_1_var>.

*   **Requests helper library** (see the `src/requesting/` directory),
    that is an implementation (with both asynchronous and synchronous
    means) of the request-response protocol on top of the messaging
    system (the primary usage is in conjunction with the counterpart
    from the `/common/js/` library).


Building
--------

Go to the `build/` directory and execute::

    make -j20

This will build the `google_smart_card_common` static library and place
it into the NaCl SDK's linker search directory, and install the
library's headers directory (`google_smart_card_common`) under the NaCl
SDK's compiler include directory.

For running **tests**, execute from the `build/` directory::

    make test -j20
