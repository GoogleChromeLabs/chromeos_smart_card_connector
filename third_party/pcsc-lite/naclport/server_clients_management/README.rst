PC/SC-Lite server clients management library
============================================


Overview
--------

This library is responsible for exposing the API, which is based on the
original PC/SC-Lite API (see
<http://pcsclite.alioth.debian.org/pcsclite.html>), from a server App
to other Apps.

The library implementation actually consists of two steps:

1.  Communication between Native Client module and JavaScript code.

    The reason for this is because the actual PC/SC-Lite API
    implementation (for its implementation, see the ``../server``
    directory) is written in C/C++ and therefore, in order to be
    included into Chrome App, has to be sandboxed through Native Client
    (see <https://developer.chrome.com/native-client>).

    Native Client module and JavaScript code communicate by exchanging
    messages (see
    <https://developer.chrome.com/native-client/devguide/coding/message-system>).

2.  Communication between the server App's JavaScript code and the client
    Apps.

    The only way how different Chrome Apps may communicate is sending
    JSON messages from one App to another (see
    <https://developer.chrome.com/apps/messaging>).

This implies that the original PC/SC-Lite API (which consisted of a
bunch of synchronous C functions) had to be adopted for the
JSON-message-exchanging nature of the communication layers.


Exposed API
-----------

The exposed API is based on exchanging of JSON messages.

A PC/SC-Lite function call request from a client App to the server App
is represented by sending a JSON message from a client App to the server
App with some special format. The result of the function call, once it
finishes, is represented by sending a response message from the server
App to the client App.

For the details of the API, refer to the ``Smart Card Connector App
API`` section of the root project ``README`` file.


Implementation discussion
-------------------------

*   One particularly important piece is that the JavaScript layer of the
    presented library has to check whether each client operates only
    with the handles that were previously acquired by it (i.e. client
    has to be prevented from trying to steal some other client's
    handle).

    These checks are provided by the class and functions defined in
    the src/clients_manager.h and the src/client_request_processor.h
    headers.

*   Another security-important feature is checking of the client
    permissions.

    The library will accept requests only from the client Apps that are
    present in the bundled whitelist; moreover, the user confirmation
    dialog will be shown when a first request from the given client is
    receiver. Additionally, there is a way to override these permissions
    through an admin policy.

    For the detailed description, see the ``Smart Card Connector App API
    Permissions`` section of the root project ``README`` file.

    The implementation of this feature is located under the
    ``src/permissions_checking`` directory.
