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
it into the "lib" subdirectory in Emscripten's path.

For running **tests**, execute from the `build/` directory::

    make test -j20
