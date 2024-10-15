libusb NaCl port
================


Overview
--------

This is a port of the **libusb** library (<http://libusb.info/>) under
the sandboxed environment of the **Google Chrome Native Client**
(<https://developer.chrome.com/native-client>).

The Native Client (or simply NaCl) environment does not allow any direct
communication to hardware devices. Instead, the **chrome.usb** Chrome
Apps JavaScript API (<https://developer.chrome.com/apps/usb>) can only
be used. The presented libusb NaCl port library is essentially just a
wrapper around this JavaScript API.

This library allows to simplify the transition of the existing software,
that was written against the original libusb API, for being packaged as
a Chrome App (<https://developer.chrome.com/apps/about_apps>).


Known issues and incompatibilities
----------------------------------

Currently, the libusb NaCl port contains quite a limited functionality
comparing to the original libusb library - only a minimal subset
allowing to use some basic functions is implemented.

Here is the description of the **differences** between this NaCl port
and the original libusb library features:

*   The library requires additional initialization before any of the
    libusb_* functions can be called.

*   Isochronous transfers are not supported.

*   Hotplug features are not implemented.

*   Transfer timeout is returned as a transfer error (i.e.
    instead of LIBUSB_TRANSFER_TIMED_OUT it's always returned
    LIBUSB_TRANSFER_ERROR).

*   The visible set of devices (even for the libusb_* functions that
    return the list of all devices) is controlled by the chrome.usb API.
    It means that, by default, most probably no devices will be
    accessible. In order to gain access to a USB device, either a user
    gesture may be requested (see
    <https://developer.chrome.com/apps/usb#method-getUserSelectedDevices>)
    or some special Chrome App manifest fields should be utilized (see
    <https://developer.chrome.com/apps/app_usb#manifest>).

*   Device product name string, manufacturer name string, serial number
    string are not returned by the API functions.

*   Device bus number that is returned by some API functions is actually
    a fake constant stub value.

*   LIBUSB_TRANSFER_ADD_ZERO_PACKET transfer flag is not supported.

*   The following libusb_* functions are not implemented:

    * libusb_attach_kernel_driver
    * libusb_clear_halt
    * libusb_detach_kernel_driver
    * libusb_event_handler_active
    * libusb_event_handling_ok
    * libusb_free_bos_descriptor
    * libusb_free_container_id_descriptor
    * libusb_free_pollfds
    * libusb_free_ss_endpoint_companion_descriptor
    * libusb_free_ss_usb_device_capability_descriptor
    * libusb_free_usb_2_0_extension_descriptor
    * libusb_get_bos_descriptor
    * libusb_get_config_descriptor
    * libusb_get_config_descriptor_by_value
    * libusb_get_configuration
    * libusb_get_container_id_descriptor
    * libusb_get_device
    * libusb_get_device_speed
    * libusb_get_max_iso_packet_size
    * libusb_get_max_packet_size
    * libusb_get_next_timeout
    * libusb_get_parent
    * libusb_get_pollfds
    * libusb_get_port_number
    * libusb_get_port_numbers
    * libusb_get_port_path
    * libusb_get_ss_endpoint_companion_descriptor
    * libusb_get_ss_usb_device_capability_descriptor
    * libusb_get_usb_2_0_extension_descriptor
    * libusb_get_version
    * libusb_handle_events_locked
    * libusb_handle_events_timeout
    * libusb_handle_events_timeout_completed
    * libusb_has_capability
    * libusb_hotplug_deregister_callback
    * libusb_hotplug_register_callback
    * libusb_kernel_driver_active
    * libusb_lock_event_waiters
    * libusb_lock_events
    * libusb_pollfds_handle_timeouts
    * libusb_set_auto_detach_kernel_driver
    * libusb_set_configuration
    * libusb_set_debug
    * libusb_set_interface_alt_setting
    * libusb_set_pollfd_notifiers
    * libusb_setlocale
    * libusb_strerror
    * libusb_transfer_get_stream_id
    * libusb_transfer_set_stream_id
    * libusb_try_lock_events
    * libusb_unlock_event_waiters
    * libusb_unlock_events
    * libusb_wait_for_event

*   Cancellation of asynchronous transfers is supported only partially:

    * Cancellation of output transfers is not supported;
    * Cancellation of input transfers is emulated: the actual chrome.usb
      transfer still continues to work, but its results will be received
      by the further input transfers from the same device with the same
      parameters.

*   The libusb_get_device_address function will crash if the device
    identifiers returned by chrome.usb API exceed 255 (this may happen,
    for example, after unplugging and plugging a device back that
    number of times).

*   Trying to resubmit an asynchronous transfer while its previous
    submission is still in flight will produce undefined behavior,
    instead of returning LIBUSB_ERROR_BUSY.

Additionally, the **performance** of the libusb_* functions
implementation can be very low: each non-trivial libusb request results
in going along the following path:

    the NaCl sandbox => the library's JavaScript code in the renderer
    process => the Chrome JavaScript API code in the renderer process =>
    the main browser process,

and, when the request finishes with some result, back along the same
path.


Discussion of libusb porting
----------------------------

The original libusb library is implemented to be OS-independent as much
as possible: basically, there is a cross-platform "core" part and there
is a "libusb backend API" that can be implemented for any given
platform.

However, it was decided against using this framework when building this
NaCl port. The reasons for the decision are the following:

1.  The libusb backend API is heavily based on the "pollable" file
    descriptors concept. However, this does not work well with the NaCl
    environment: the \*nix domain socket support is fragmentary (though
    this may change in the future), and implementing the libusb backend
    API would require some not very clean tricks.

2.  The timeouts support: libusb on most platforms uses the timerfd
    capabilities for working with timeouts. Without timerfd, some bad
    side effects may happen (like "missing" a transfer result and
    waiting a whole timeout instead). Unfortunately, the Portable Native
    Client environment has no built-in timerfd support, and its
    emulation would probably require several other dirty tricks.

3.  The third reason is that the chrome.usb API already provides a
    high-level API, similar to the libusb interface. So there is not
    much benefit from using the libusb abstraction of the "core" and the
    "backend" parts.

So the NaCl port presented here is basically a completely separate
implementation, sharing with the original library only the public libusb
header files.


Architecture overview
---------------------

As it was already said, this NaCl port is essentially a bridge linking
the **original libusb API** and the **chrome.usb JavaScript API**. Both
of the APIs provide essentially very similar set of operations: e.g.
listing of the devices, obtaining the device properties, claiming the
device interface, performing control/bulk/interrupt/isochronous
transfers, etc.

The major difference between them is that all operations in the
chrome.usb JavaScript API are asynchronous operations (due to the nature
of the JavaScript). This means that all blocking libusb API functions
should be implemented so that they block until the corresponding result
is received from the JavaScript side.

The NaCl port implementation is built basing on the primitives provided
by the libraries located in the /common/ directory.

Basically, each non-trivial libusb request is transformed into a message
sent from the NaCl module to the JavaScript side (see
<https://developer.chrome.com/native-client/devguide/coding/message-system>);
the JavaScript side contains a code that transforms received messages
into chrome.usb API calls; the results of the chrome.usb API calls, once
they return them through asynchronous callbacks, are then sent as a
message back to the NaCl module. Each request has an associated unique
identifier, which allows to handle multiple libusb API calls
simultaneously.
