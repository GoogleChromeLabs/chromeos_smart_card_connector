== LINUX Additional buildbot setup. ==

To run the i686 plumbing tests (running i686 chromium), these additional
packages are required:

libglib2.0-0:i386 libnss3:i386 libgconf-2-4:i386 libfontconfig:i386
libpango1.0-0:i386 libxi6:i386 libxcursor1:i386 libxcomposite1:i386
libasound2:i386 libxdamage1:i386 libxtst6:i386 libxrandr2:i386 libcap2:i386
libudev0:i386 libgtk2.0-0:i386 libxss1:i386 libexif12:i386
libgl1-mesa-glx:i386

Additionally, on older Ubuntu system such as Precise, we currently need
some symlinks to be manually created:
$ cd /usr/lib/i386-linux-gnu/
$ sudo ln -s libcrypto.so.0.9.8 libcrypto.so
$ sudo ln -s libssl.so.0.9.8 libssl.so

Additionally, if nacl tests are to be run, these packages are required:
libgpm2:i386 libncurses5:i386

This can be done en-masse to the buildbots via:
for i in $(seq <start> <end>); do ssh -t slave${i}-c4 'sudo aptitude install libglib2.0-0:i386 libnss3:i386 libgconf-2-4:i386 libfontconfig:i386 libpango1.0-0:i386 libxi6:i386 libxcursor1:i386 libxcomposite1:i386 libasound2:i386 libxdamage1:i386 libxtst6:i386 libxrandr2:i386 libcap2:i386 libudev0:i386 libgtk2.0-0:i386 libxss1:i386 libexif12:i386 libgl1-mesa-glx:i386 libssl0.9.8:i386 lib32z1-dev libgpm2:i386 libncurses5:i386 && cd /usr/lib/i386-linux-gnu/ && sudo ln -s libcrypto.so.0.9.8 libcrypto.so && sudo ln -s libssl.so.0.9.8 libssl.so' ; done
