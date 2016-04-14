PC/SC-Lite server NaCl port
===========================


Overview
--------

This is a port of the server side of the **PC/SC-Lite** service
(middleware to access a smart card using SCard API, see
<http://pcsclite.alioth.debian.org/pcsclite.html>) under the sandboxed
environment of the **Google Chrome Native Client**
(<https://developer.chrome.com/native-client>).

The original original PC/SC-Lite project consists of two parts:

1.  **Daemon** that works in a background process and communicates to
    the connected smart card readers.

    The daemon can accept commands through POSIX domain sockets (the
    commands have to be transmitted through the socket in a serialized
    binary form - the exact protocol is not documented).

2.  **Client library** that can be linked with client applications and
    that provides a specific API for operating with smart card readers
    (for the API documentation, refer to
    <http://pcsclite.alioth.debian.org/api/group__API.html>).

    The client library encapsulates all the logic of maintaining a
    socket to the daemon process, sending the requests data in
    serialized binary form through the socket and decoding the request
    responses received through the socket.

This is how the original PC/SC-Lite service intended to works on \*nix
and on other platforms.

In order to port PC/SC-Lite onto **Chrome OS**, different circumstances
had to be taken into account:

*   Each application (both PC/SC-Lite daemon and the client
    applications) has to be bundled as a separate Chrome App (see
    <https://developer.chrome.com/apps/about_apps>).

*   The only way how these Apps can communicate is sending JSON messages
    between them (see <https://developer.chrome.com/apps/messaging>).

    There is no full analogue of POSIX domain sockets for communication
    between different Apps.

*   In Chrome OS environment, it makes sense to write some client
    applications entirely in pure JavaScript.

    This implies that the binary protocol that was used by the original
    PC/SC-Lite daemon becomes not very useful. Instead, it makes sense
    for the server App to expose the analog of the client PC/SC-Lite
    API.

As a consequence, it was decided that the server App on Chrome OS should
consist of **both** the parts of the original PC/SC-Lite daemon and the
parts of the original PC/SC-Lite client library. This is what the
presented library consists of.

For the another part - exposing the PC/SC-Lite API as a
JSON-message-based API for other Apps - refer to the library located
under the ``../server_clients_management`` directory.


Implementation discussion
-------------------------

*   The aim was to retain the original implementation source files (from
    both PC/SC-Lite daemon and PC/SC-Lite client library) as much as
    possible.

*   The daemon configuration, startup code and the main run loop were
    transformed into library functions.

    This allows to bundle the transformed daemon as a part of a larger
    Native Client module. The daemon functionality can be started by
    calling a provided function from the main code of the resulting
    module.

*   One particularly big chunk that was reimplemented from scratch is
    the communication between the client library and the daemon.

    In the original PC/SC-Lite project, the client library and the
    daemon talked to each other through POSIX domain sockets.

    Unfortunately, the POSIX domain sockets are not currently supported
    by the Portable Native Client (see <http://crbug.com/532095>). So,
    in order to avoid massive modification of the original source files,
    the simplified stub implementation of the POSIX domain sockets was
    written.
