common/js library
=================


This library contains some common primitives for use in the JavaScript
code of the Smart Card Apps.

This library is mostly a number of extensions to the Google Closure
Library (see <https://developers.google.com/closure/library/>).

The main pieces are:

*   **Emscripten module helper library** (see the
    `src/executable-module/emscripten-module.js` file), that provides an
    interface for loading and communicating to the Emscripten/WebAssembly
    module. It also takes care of debug logging.

*   **App background page unload preventing library** (see the
    `src/background-page-unload-preventing/` directory), that prevents
    Chrome App's background page from being unloaded (for the default
    behavior description, see
    <https://developer.chrome.com/apps/event_pages>).

*   **Modal dialogs library** (see the `src/modal-dialog/` directory),
    that provides a convenient interface for running modal dialogs and
    obtaining their results.

*   **Requests helper library** (see the `src/requesting/` directory),
    that is an implementation of the request-response protocol on top of
    the messaging system (in particular, this can act as a counterpart
    for the C/C++ library from the `/common/cpp/` directory).


Using
-----

The library is intended to be used in compiled JavaScript code through
the Google Closure Compiler (see
<https://developers.google.com/closure/compiler/>).

The library is not compiled standalone; instead, it should be specified
in the input paths list of the Closure Compiler when the target App
scripts are compiled.
