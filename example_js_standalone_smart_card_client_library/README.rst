Example JavaScript Standalone Smart Card Client Library
=======================================================


This library allows to use the API exposed by the Smart Card Connector
App. The library is prebuilt, allowing you to use it in your
application without having to deal with the library's build process
(which is currently based on Closure Compiler). The prebuilt library
comes in two flavours:

* As a standalone script ``google-smart-card-client-library.js`` (which
  can be directly executed to get the necessary
  ``GoogleSmartCard``/``goog`` definitions in the global scope; all
  other library's internal definitions are hidden in an anonymous
  wrapper function to avoid symbol collision);
* As an ES module script
  ``google-smart-card-client-library-es-module.js`` (from which the
  ``GoogleSmartCard``/``goog`` definitions can be imported).


Obtaining the library .JS files
-------------------------------

Even though you can build the library yourself, the recommended flow is
to download a prebuilt library from
<https://github.com/GoogleChromeLabs/chromeos_smart_card_connector/releases>.

You have a choice between 4 variations: ES module or non ES module, and
each in either Release (recommended for production) or Debug variant.


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
