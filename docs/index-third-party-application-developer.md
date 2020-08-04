# Documentation for developers of Chrome OS extensions/apps that need the PC/SC API


## Background

The Smart Card Connector App acts as the provider of the **PC/SC API** to other
Chrome OS extensions or apps.

The Smart Card Connector App plays, basically, the same role as the **PC/SC-Lite
Daemon** does on Linux (see
[http://linux.die.net/man/8/pcscd](http://linux.die.net/man/8/pcscd)).

The Smart Card Connector App is also bundled with the **CCID Free Software
Driver** ([https://ccid.apdu.fr/](https://ccid.apdu.fr/)), which means that most
of CCID-compatible readers are supported.


## Providing smart card certificates and keys to Chrome OS

The Smart Card Connector App itself has no smart card middleware and therefore
isn't able to provide access to certificates and keys to Chrome OS.

This role is delegated to separate (first-party or third-party) Chrome OS
extensions and apps that can consume the PC/SC API and use the
**chrome.certificateProvider** API
([https://developer.chrome.com/extensions/certificateProvider](https://developer.chrome.com/extensions/certificateProvider))
in order to provide them to various Chrome OS components, namely:

* TLS client authentication when visiting Web pages;
* Chrome OS user authentication (when the SAML-based login is used and the
  Identity Provider is configured to use the smart card based authentication).


## Accessing the PC/SC API exposed by the Smart Card Connector App

See [docs/connector-app-api.md](connector-app-api.md).
