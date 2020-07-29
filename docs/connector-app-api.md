# API exposed by the Smart Card Connector App

The Smart Card Connector App exposes an API that allows other Chrome OS
Extensions/Apps to make PC/SC calls (see the generic specification of this API
at
[https://pcsclite.apdu.fr/api/group__API.html](https://pcsclite.apdu.fr/api/group__API.html)).


## Permissions for the PC/SC API consumers

Due to various privacy and security concerns, the following decisions
were made:

1. When an Extension/App tries to talk to the Connector App, a **user prompt
   dialog** is generally shown, asking whether to allow or to block this
   Extension/App.

   If the user decides to allow the Extension/App, the decision is remembered in
   the Connector App's local storage, and the requests made by the Extension/App
   start being actually executed by the Connector App. Otherwise, all the
   requests made by the middleware App are refused.

   Note: this system has nothing to do with the Chrome App permissions model
   (see
   [https://developer.chrome.com/apps/declare_permissions](https://developer.chrome.com/apps/declare_permissions)),
   as Chrome allows to use the messaging API without any additional permissions.
   This intent is to close the hole left open by cross-app messaging between
   Apps with different permissions.

2. The Connector App is **bundled with a whitelist of known Extension/App
   identifiers** (and a mapping to their display names). For all Extensions/Apps
   not from this list, the user prompt will contain a big scary warning message.

   (Note: This behavior was introduced in the Smart Card Connector app version
   1.2.7.0. Before this version, all requests from unknown apps were silently
   ignored.)

3. For the **enterprise** cases, it's possible to configure the Connector App
   through an **admin policy** (see
   [https://www.chromium.org/administrators/configuring-policy-for-extensions](https://www.chromium.org/administrators/configuring-policy-for-extensions)).

   The policy can specify which Extensions/Apps are **force allowed to talk to
   the Connector App** without checking against whitelist or prompting the user.
   The policy-configured permission always has the higher priority than the
   user's selections that could have been already made.

   The corresponding policy name is `force_allowed_client_app_ids`. Its value
   should be an array of strings representing the App identifiers. This is an
   example of the policy JSON blob:

   ```json
   {
     "force_allowed_client_app_ids": {
       "Value": [
         "this_is_middleware_client_app_id",
         "this_is_some_other_client_app_id"]
     }
   }
   ```


## Protocol for making PC/SC API calls to the Smart Card Connector App

The API calls to the Connector App are made using the cross-extension messageing
functionality (see
[https://developer.chrome.com/apps/messaging](https://developer.chrome.com/apps/messaging)).

The **PC/SC-Lite API function call** is represented by a message sent to the
Connector App. The message should have the following format:

```
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
```

where `<requestId>` should be a number (unique in the whole session of the
middleware App communication to the Connector App), `<functionName>` should be a
string containing the PC/SC-Lite API function name, `<functionArguments>`
should be an array of the input arguments that have to be passed to the
PC/SC-Lite API function.

The **results** returned from the PC/SC-Lite API function call are represented
by a message sent back from the Connector App to the middleware App.

If the request was processed **successfully** (i.e. the PC/SC-Lite function was
recognized and called), then the message will have the following format:

```
{
  "type": "pcsc_lite_function_call::response",
  "data": {
    "request_id": <requestId>,
    "payload": <results>
  }
}
```

where `<requestId>` is the number taken from the request message, and
`<results>` is an array of the values containing the function return value
followed by the contents of the function output arguments.

If the request **failed** with some error (note: this is *not* the case when the
PC/SC-Lite function returns non-zero error code), then the message will have the
following format:

```
{
  "type": "pcsc_lite_function_call::response",
  "data": {
    "request_id": <requestId>,
    "error": <errorMessage>
  }
}
```

where `<requestId>` is the number taken from the request message, and
`<errorMessage>` is a string containing the error details.

Additionally, Apps on both sides of the communication channel can send **ping**
messages to each other:

```json
{
  "type": "ping",
  "data": {}
}
```

The other end should response with a **pong** message having the following
format:

```
{
  "type": "pong",
  "data": {
    "channel_id": <channelId>
  }
}
```

where `<channelId>` should be the number generated randomly in the beginning of
the communication.

Pinging allows to track whether the other end is still alive and functioning
(Chrome's long-lived messaging connections, when they are used, are able to
detect most of the cases - but the one-time messages passing API is also allowed
to be used). The `<channelId>` field value allows the other end to track cases
when the App died and restarted while a response from it was awaited.

For simplifying the middleware Apps development, the **wrapper libraries** for
**JavaScript** and **C** are provided (the latter one is basically an
implementation of the functions defined in the original PC/SC-Lite headers). See
the corresponding example Apps for the details (the
`//example_js_smart_card_client_app/` and the
`//example_cpp_smart_card_client_app/` directories), and the standalone
JavaScript library (see the `//example_js_standalone_smart_card_client_library/`
directory).
