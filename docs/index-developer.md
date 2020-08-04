# Documentation for developers of the Smart Card Connector App


## Overview

Main components of the Smart Card Connector App:

* PC/SC-Lite Daemon,
* CCID Free Software Driver,
* UI.

### PC/SC-Lite Daemon

The app is bundled with a port of the PC/SC-Lite daemon. For the background, see
the original documentation of the PC/SC-Lite daemon on Linux:
[http://linux.die.net/man/8/pcscd](http://linux.die.net/man/8/pcscd).

The daemon is ported using the Native Client technology for running inside the
JavaScript-based app.

Our port of the daemon exposes the PC/SC API to other Chrome OS extensions/apps:
see [docs/connector-app-api.md](connector-app-api.md).

### CCID Free Software Driver

The app is bundled with the **CCID Free Software Driver**
([https://ccid.apdu.fr/](https://ccid.apdu.fr/)). This driver implements talking
to smart card readers that are compatible with the CCID specification.

Similar to the PC/SC_lite Daemon, the CCID Free Software Driver is ported using
the Native Client technology.

The low-level USB operations are implemented using our port of the **libusb**
library, which is redirecting all requests to the **chrome.usb** API (see
[https://developer.chrome.com/apps/usb](https://developer.chrome.com/apps/usb)).


## Building the code

See [docs/building.md](building.md).


## Potential future enhancements

* **Pluggable smart card reader drivers.**

  Currently, there's only the CCID Free Software Driver supported. We might want
  to extend this in the future, by exposing a new API that would allow separate
  Chrome OS extensions/apps to implement drivers for other devices.
