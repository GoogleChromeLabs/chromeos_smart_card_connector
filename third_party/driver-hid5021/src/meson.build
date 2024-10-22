project('driver-5021CL', 'c',
  meson_version : '>=0.58.0',
  version : '1.0.0')

pcsc_dep = dependency('libpcsclite')
pcsc_cflags = pcsc_dep.partial_dependency(compile_args : true)
libusb_dep = dependency('libusb-1.0')

driver_src = [
  'ifdhandler.c',
  ]

library('driver-5021CL',
  driver_src,
  dependencies: [libusb_dep, pcsc_cflags],
  override_options : ['b_lundef=false'],
  install : true,
  install_dir : pcsc_dep.get_variable('usbdropdir') / 'driver-5021CL.bundle/Contents/Linux',
  )

install_data('Info.plist',
  install_dir : pcsc_dep.get_variable('usbdropdir') / 'driver-5021CL.bundle/Contents',
  )
