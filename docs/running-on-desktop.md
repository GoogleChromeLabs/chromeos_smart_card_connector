# Running on desktop OS'es

Note: The target platform of the Smart Card Connector App and other code in this
repository is **only Chrome OS**.

However, for developers there's currently a possibility to run this code on
desktop OS'es as well (i.e., Linux, Windows, etc.).

## Limitations and workarounds

* **chrome.certificateProvider Chrome API is unavailable**.

  This is working as intended. This Chrome API, along with several others, is
  provided only on Chrome OS (see the Chrome App APIs documentation at
  [https://developer.chrome.com/apps/api_index](https://developer.chrome.com/apps/api_index)).

  The usages of such APIs will have to be stubbed out when running under desktop
  OS'es.

* **On \*nix systems, a system-wide PCSCD daemon may prevent Chrome from
  accessing the USB devices**.

  The simplest solution is to stop the system-wide daemon.

  For example, under Ubuntu this can be done with the following command:

  ```sudo service pcscd stop
  ```

  On macOS systems (since at least El Capitan, 10.11) the pcscd daemon has been
  replaced by a daemon called com.apple.ifdreader. You can stop it using:

  ```sudo pkill -HUP com.apple.ifdreader
  ```

* **On \*nix systems, the USB device file permissions may prevent Chrome from
  accessing the device**.

  The simplest solution, described below, is to give the writing permissions for
  the USB device file to all users; note that, however, this is **unsafe on
  multi-user systems**!

  So granting the write access for all users can be performed in two ways:

  * One quick option is to add the permissions manually:

    ```sudo chmod 666 /dev/bus/usb/<BUS>/<DEVICE>
    ```

    Where `<BUS>` and `<DEVICE>` numbers can be taken, for example, from the
    output of the lsusb tool:

    ```lsusb
    ```

  * Another, more robust, option is to add a udev rule (see, for example, the
    documentation at
    [https://www.kernel.org/pub/linux/utils/kernel/hotplug/udev/udev.html](https://www.kernel.org/pub/linux/utils/kernel/hotplug/udev/udev.html)).

* **On Windows, a generic USB driver may be required to make the smart card
  reader devices available to Chrome**.

  For example, this can be done with the **Zadig** tool (Note: this is a
  third-party application that is not affiliated with Google in any way. **Use
  at your own risk!**): [http://zadig.akeo.ie](http://zadig.akeo.ie).
