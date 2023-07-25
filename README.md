# Smart Card Connector App for ChromeOS

This repository contains sources of the ChromeOS **Smart Card Connector App**
(distributed at
[https://chrome.google.com/webstore/detail/smart-card-connector/khpfeaanjngmcnplbdlpegiifgpfgdco](https://chrome.google.com/webstore/detail/smart-card-connector/khpfeaanjngmcnplbdlpegiifgpfgdco))
and examples how programs can communicate with this app.


## Documentation

This documentation is split into several parts for different target audiences:

* **Are you a ChromeOS user?**

  Please refer to this Help Center article:
  [https://support.google.com/chrome/a/answer/7014689](https://support.google.com/chrome/a/answer/7014689).

* **Are you a ChromeOS administrator?**

  Please refer to this Help Center article:
  [https://support.google.com/chrome/a/answer/7014520](https://support.google.com/chrome/a/answer/7014520).

* **Are you a developer of a ChromeOS extension/app that needs to access the
  PC/SC API in order to talk to smart cards?**

  Please refer to this README:
  [docs/index-third-party-application-developer.md](docs/index-third-party-application-developer.md).

* **Are you a developer who needs to build the Smart Card Connector App and/or a
  maintainer of this repository?**

  Please refer to this README:
  [docs/index-developer.md](docs/index-developer.md).


## FAQ

### What is a smart card?

Please refer to
[https://en.wikipedia.org/wiki/Smart_card](https://en.wikipedia.org/wiki/Smart_card).

Note that there also some devices that emulate a smart card, e.g., some of
Yubikey devices (see
[https://www.yubico.com/authentication-standards/smart-card/](https://www.yubico.com/authentication-standards/smart-card/)).

### Do I need the Smart Card Connector App?

You only need this in case you have a smart card (or a device that emulates it)
and need to use it on your ChromeOS device for authenticating at web pages,
remote desktop applications, logging into ChromeOS in enterprise deployments,
etc.

### Can I use it with my memory card (microSD, etc.)?

No. The Smart Card Connector App is only useful with **smart cards**.
