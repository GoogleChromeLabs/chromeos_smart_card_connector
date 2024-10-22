# Limited driver for the HID 5021 CL reader

## Driver

To build the driver use the script `build.sh`.

The driver uses the [Meson Build system](https://mesonbuild.com/).

### Debian/Ubuntu Packages

Packages to install on a fresh install Debian 12 system:
- unzip
- meson
- libpcsclite-dev
- pkgconf
- libusb-1.0-0-dev
- pcscd

## supported features

The driver:
- detects when a supported card enters of leaves the reader RF field
- supports the Get UID APDU command
- detects collisions i.e. when 2 (or more) cards are present in the reader RF field.
  the driver then reports a card is present but unpowered

## Test commands

### Debian/Ubuntu Packages

For the tests install the packages:
- pcsc-tools
- python3-pyscard

### getUID.scriptor

The script `getUID.scriptor` is used with the `scriptor` command from
[pcsc-tools](https://pcsc-tools.apdu.fr/).

Run it with (the card must be *already* present on the reader):

```
scriptor getUID.scriptor
```

Output should look like:

```
Using given card reader: HID 5021-CL (OKCM0030705141348106362197815235) 00 00
Using T=0 protocol
Using given file: getUID.scriptor
#!/usr/bin/env scriptor

reset
> RESET
< OK: 3B 8F 80 01 80 4F 0C A0 00 00 03 06 03 00 02 00 00 00 00 69 

# get UID command
FF CA 00 00 00
> FF CA 00 00 00
< 5A 0D 1B 35 90 00 : Normal processing.

# unknown command: should fail
00 00 00 00
> 00 00 00 00
< 68 00 : Functions in CLA not supported. 
```

### getuid_v2.py

The script `getuid_v2.py` uses the Python module [pyscard](https://pypi.org/project/pyscard/).

Run it with:

```
python3 getuid_v2.py
```

Output should look like:

```
Insert a card within 10 seconds
Using: HID 5021-CL (OKCM0030705141348106362197815235) 00 00
Card ATR: 3B 8F 80 01 80 4F 0C A0 00 00 03 06 03 00 02 00 00 00 00 69
UID: 5A 0D 1B 35
SW: 90 00
Card number: 890965338
```

In case of collision (2 or more cards present in the reader RF field)
you should get:
```
Insert a card within 10 seconds
Unable to connect with protocol: T0 or T1. Card is unpowered.: Card is unpowered. (0x80100067)
```

