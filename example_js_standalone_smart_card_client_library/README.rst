Example JavaScript Standalone Smart Card Client Library
=======================================================


This library allows to use the API exposed by the Smart Card Connector
App. The library is built as a standalone script, that allows to use it
without compiling all of the client code through the Google Closure
Compiler or without the need to plug the Google Closure Library
manually.

The resulting JS code in the library is wrapped into an anonymous
wrapper function, but all the library interface definitions (namely the
``GoogleSmartCard.PcscLiteClient.Context`` and the
``GoogleSmartCard.PcscLiteClient.API`` classes) and several crucial
definitions from the Closure Library (``goog.Promise``,
``goog.log.Logger``, etc.) are exported into the global ``window``
object properties.


Building
--------

First, please make sure that all building prerequisites are provided and
the building environment is set up - please refer to *Common building
prerequisites* and *Building* sections of the ``README`` file located in
the project root.

After that, the library can be built by the following command::

    make

The resulting script file will be located in the ``out/`` directory.

For the details of the code compilation, refer to the Closure Compiler
documentation:
<https://developers.google.com/closure/compiler/docs/limitations>.


Debug and Release building modes
--------------------------------

For the general discussion, please refer to the *Debug and Release
building modes* section of the ``README`` file located in the project
root.

Additional notes:

*   The Release mode triggers more advanced JavaScript compilation
    modes, which result in renaming and removing of some of the symbols.

    However, all the publicly available exported definitions and their
    internal dependencies are always kept by the compiler (or, at least,
    should be).

*   When building in Debug mode, in addition to the resulting script, a
    source map and a copy of all participated source files is put into
    the ``out/`` directory.

    These files are not required for the library functionality, but, if
    put into the App next to the library script, may simplify the
    debugging.


Usage
-----

The sample code for using the library::

    var context = new GoogleSmartCard.PcscLiteClient.Context(
        'test_smart_card_client');

    function contextInitializedListener(api) {
      // api is an instance of GoogleSmartCard.PcscLiteClient.API

      // Here PC/SC-Lite requests may be issued, e.g.:
      var pcScContextPromise = api.SCardEstablishContext(
          GoogleSmartCard.PcscLiteClient.API.SCARD_SCOPE_SYSTEM);
      pcScContextPromise.then(function(result) {
        result.get(function(pcScContextHandle) {
          console.log('PC/SC-Lite context was established: ' +
                      pcScContextHandle);
        }, function(pcScLiteErrorCode) {
          console.log('PC/SC-Lite context establishing failed ' +
                      'with the following error code: ' +
                      pcScLiteErrorCode);
        });
      }, function(error) {
        console.log('PC/SC-Lite context establishing request failed ' +
                    'with the following error: ' + error);
      });
    }

    function contextDisposedListener() {
      // Signal the error, optionally schedule a retry
    }

    context.addOnInitializedCallback(contextInitializedListener);
    context.addOnDisposeCallback(contextDisposedListener);
    context.initialize();

The more advanced examples can be found in the Example JavaScript Smart
Card Client App (see the ``/example_js_smart_card_client_app/``
directory).


A note regarding Chrome Apps lifetime
-------------------------------------

Chrome, by default, tries to unload the background pages of the Apps
whenever it thinks they are in the "idle" state. The exact algorithm is
such that the background page may be considered to be "idle" even when
it's actually doing some work - for the details please refer to the
documentation: <https://developer.chrome.com/apps/event_pages>.

Having an opened messaging port to the Connector App will keep the
client App's background page alive, but, during the periods when the
connection for some reason is lost, the background page may be unloaded.

If such behavior is undesirable for the client App, then some additional
precautions have to be taken. One of the ways to prevent the background
page from being unloaded is implemented in the common JS library (see
the ``/common/js/src/background-page-unload-preventing/`` directory),
and is utilized in the Example JavaScript Smart Card Client App.
