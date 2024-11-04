/*****************************************************************
/
    Limited driver for the HID 5021-CL reader
    Copyright (C) 2024  Numericoach

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
/
******************************************************************/

#include <string.h>
#include <stdbool.h>

#include <wintypes.h>
#include <debuglog.h>
#include <ifdhandler.h>
#include <libusb.h>

#include "commandsV2.h"

#undef DEBUG_COMM

static libusb_context *ctx;
static libusb_device_handle *device_handle;
static int bulk_in;
static int bulk_out;
static unsigned char uid[10];		// in general 4 bytes only
static int uid_len;
static bool card_present = false;

// check value returned by escape()
#define Escape(a, b, c, d) do { if (escape(a, b, c, d) != IFD_SUCCESS) goto error; } while (0)

// send a PC_to_RDR_Escape CCID command
static int escape(unsigned char cmd[], int size, unsigned char res[], int * res_size)
{
	char unsigned ccid_cmd[1024];
	int rv;
	int rec_length;
	int timeout = 5 * 1000;		// 5 seconds
	static int bSeq;

	ccid_cmd[0] = 0x6b;		// bMessageType

	ccid_cmd[1] = size;		// dwLength
	ccid_cmd[2] = 0;		// dwLength
	ccid_cmd[3] = 0;		// dwLength
	ccid_cmd[4] = 0;		// dwLength
							//
	ccid_cmd[5] = 0;		// bSlot
	ccid_cmd[6] = bSeq++;	// bSeq

	ccid_cmd[7] = 0;		// abRFU
	ccid_cmd[8] = 0;		// abRFU
	ccid_cmd[9] = 0;		// abRFU

	if (size > 1024-10)
	{
		Log2(PCSC_LOG_CRITICAL, "Command too big: %d bytes", size);
		return IFD_COMMUNICATION_ERROR;
	}

	// copy the command after the CCID header
	memcpy(ccid_cmd + 10, cmd, size);

#ifdef DEBUG_COMM
	log_xxd(PCSC_LOG_DEBUG, "-> ", ccid_cmd, size+10);
#endif

	// send the command
	rv = libusb_bulk_transfer(device_handle, bulk_out,
			ccid_cmd, size+10, &rec_length, timeout);
	if (rv < 0)
	{
		Log2(PCSC_LOG_CRITICAL, "write failed: %s", libusb_error_name(rv));
		if (LIBUSB_ERROR_TIMEOUT == rv)
		{
			rv = libusb_reset_device(device_handle);
			Log2(PCSC_LOG_CRITICAL, "libusb_reset_device: %s", libusb_error_name(rv));
		}
		return IFD_COMMUNICATION_ERROR;
	}

	// read the response
	rv = libusb_bulk_transfer(device_handle, bulk_in,
			ccid_cmd, sizeof ccid_cmd, &rec_length, timeout);
	if (rv < 0)
	{
		Log2(PCSC_LOG_CRITICAL, "read failed: %s", libusb_error_name(rv));
		return IFD_COMMUNICATION_ERROR;
	}

#ifdef DEBUG_COMM
	log_xxd(PCSC_LOG_DEBUG, "<- ", ccid_cmd, rec_length);
#endif

	// get the payload
	if (res && res_size)
	{
		*res_size = ccid_cmd[1];	// dwLength (use only 1st byte)
		memcpy(res, ccid_cmd+10, *res_size);
	}

	return IFD_SUCCESS;
}

RESPONSECODE IFDHCreateChannel ( DWORD Lun, DWORD Channel ) {

  /* Lun - Logical Unit Number, use this for multiple card slots
     or multiple readers. 0xXXXXYYYY -  XXXX multiple readers,
     YYYY multiple slots. The resource manager will set these
     automatically.  By default the resource manager loads a new
     instance of the driver so if your reader does not have more than
     one smartcard slot then ignore the Lun in all the functions.
     Future versions of PC/SC might support loading multiple readers
     through one instance of the driver in which XXXX would be important
     to implement if you want this.
  */

  /* Channel - Channel ID.  This is denoted by the following:
     0x000001 - /dev/pcsc/1
     0x000002 - /dev/pcsc/2
     0x000003 - /dev/pcsc/3

     USB readers may choose to ignore this parameter and query
     the bus for the particular reader.
  */

  /* This function is required to open a communications channel to the
	 port listed by Channel.  For example, the first serial reader on
	 COM1 would link to /dev/pcsc/1 which would be a sym link to
	 /dev/ttyS0 on some machines This is used to help with intermachine
	 independance.

	 Once the channel is opened the reader must be in a state in which
	 it is possible to query IFDHICCPresence() for card status.

     returns:

     IFD_SUCCESS
     IFD_COMMUNICATION_ERROR
  */

	Log0(PCSC_LOG_DEBUG);

	// ignore unused parameters
	(void)Lun;
	(void)Channel;

	// support of 1 reader only
	if (ctx)
	{
		Log1(PCSC_LOG_CRITICAL, "Driver already used");
		return IFD_COMMUNICATION_ERROR;
	}

	// init libusb
	int rv = libusb_init(&ctx);
	if (rv)
	{
		Log2(PCSC_LOG_CRITICAL, "libusb_init: %s", libusb_error_name(rv));
		goto error;
	}

	// open the HID 5021 CL USB device
	device_handle = libusb_open_device_with_vid_pid(ctx, 0x076B, 0x5320);
	if (!device_handle)
	{
		Log1(PCSC_LOG_CRITICAL, "Device not found");
		goto error;
	}

	struct libusb_device * device = libusb_get_device(device_handle);

	struct libusb_config_descriptor *descriptor;
	rv = libusb_get_active_config_descriptor(device, &descriptor);
	if (rv < 0)
	{
		Log2(PCSC_LOG_CRITICAL, "libusb_get_active_config_descriptor: %s", libusb_error_name(rv));
		goto error;
	}

	// use first interface and first setting
	const struct libusb_interface_descriptor *setting = descriptor->interface->altsetting;

	for (int i=0; i<setting->bNumEndpoints; i++)
	{
		if (setting->endpoint[i].bmAttributes != LIBUSB_TRANSFER_TYPE_BULK)
			continue;

		int ep_address = setting->endpoint[i].bEndpointAddress;

		if ((ep_address & LIBUSB_ENDPOINT_DIR_MASK) == LIBUSB_ENDPOINT_IN)
			bulk_in = ep_address;
		if ((ep_address & LIBUSB_ENDPOINT_DIR_MASK) == LIBUSB_ENDPOINT_OUT)
			bulk_out = ep_address;
	}

	// claim the USB interface
	rv = libusb_claim_interface(device_handle, setting->bInterfaceNumber);
	if (rv < 0)
	{
		Log2(PCSC_LOG_CRITICAL, "libusb_claim_interface: %s", libusb_error_name(rv));
		goto error;
	}

	// sequence to initialize the reader

	// if the driver was stopped _before_ a USB read the reader will
	// timeout on a USB write. A reset is performed in escape() and we
	// can retry the same command
	if (escape(command_00001, sizeof command_00001, NULL, NULL) != IFD_SUCCESS)
		Escape(command_00001, sizeof command_00001, NULL, NULL);

	Escape(command_00002, sizeof command_00002, NULL, NULL);
	Escape(command_00003, sizeof command_00003, NULL, NULL);
	Escape(command_00004, sizeof command_00004, NULL, NULL);

	// this command may also timeout on USB write
	if (escape(command_00005, sizeof command_00005, NULL, NULL) != IFD_SUCCESS)
		Escape(command_00005, sizeof command_00005, NULL, NULL);

	Escape(command_00006, sizeof command_00006, NULL, NULL);
	Escape(command_00006, sizeof command_00006, NULL, NULL);
	Escape(command_00007, sizeof command_00007, NULL, NULL);
	Escape(command_00008, sizeof command_00008, NULL, NULL);
	Escape(command_00009, sizeof command_00009, NULL, NULL);

	Escape(command_00010, sizeof command_00010, NULL, NULL);
	Escape(command_00011, sizeof command_00011, NULL, NULL);
	Escape(command_00012, sizeof command_00012, NULL, NULL);
	Escape(command_00007, sizeof command_00007, NULL, NULL);
	Escape(command_00014, sizeof command_00014, NULL, NULL);
	Escape(command_00015, sizeof command_00015, NULL, NULL);
	Escape(command_00016, sizeof command_00016, NULL, NULL);
	Escape(command_00017, sizeof command_00017, NULL, NULL);
	Escape(command_00018, sizeof command_00018, NULL, NULL);
	Escape(command_00005, sizeof command_00005, NULL, NULL);

	Escape(command_00020, sizeof command_00020, NULL, NULL);
	Escape(command_00006, sizeof command_00006, NULL, NULL);
	Escape(command_00022, sizeof command_00022, NULL, NULL);
	Escape(command_00023, sizeof command_00023, NULL, NULL);
	Escape(command_00024, sizeof command_00024, NULL, NULL);
	Escape(command_00025, sizeof command_00025, NULL, NULL);
	Escape(command_00026, sizeof command_00026, NULL, NULL);
	Escape(command_00027, sizeof command_00027, NULL, NULL);
	Escape(command_00028, sizeof command_00028, NULL, NULL);
	Escape(command_00029, sizeof command_00029, NULL, NULL);

	Escape(command_00030, sizeof command_00030, NULL, NULL);
	Escape(command_00030, sizeof command_00030, NULL, NULL);
	Escape(command_00030, sizeof command_00030, NULL, NULL);
	Escape(command_00030, sizeof command_00030, NULL, NULL);

	Escape(command_00092, sizeof command_00092, NULL, NULL);
	Escape(command_00093, sizeof command_00093, NULL, NULL);
	Escape(command_00094, sizeof command_00094, NULL, NULL);
	Escape(command_00095, sizeof command_00095, NULL, NULL);
	Escape(command_00096, sizeof command_00096, NULL, NULL);
	Escape(command_00097, sizeof command_00097, NULL, NULL);
	Escape(command_00098, sizeof command_00098, NULL, NULL);
	Escape(command_00099, sizeof command_00099, NULL, NULL);
	Escape(command_00100, sizeof command_00100, NULL, NULL);
	Escape(command_00101, sizeof command_00101, NULL, NULL);
	Escape(command_00094, sizeof command_00094, NULL, NULL);

	return IFD_SUCCESS;

error:
	// close device
	if (device_handle)
		libusb_close(device_handle);
	device_handle = NULL;

	// close libusb
	if (ctx)
		libusb_exit(ctx);
	ctx = NULL;

	return IFD_COMMUNICATION_ERROR;
}

RESPONSECODE IFDHCloseChannel ( DWORD Lun ) {

  /* This function should close the reader communication channel
     for the particular reader.  Prior to closing the communication channel
     the reader should make sure the card is powered down and the terminal
     is also powered down.

     returns:

     IFD_SUCCESS
     IFD_COMMUNICATION_ERROR
  */

	Log0(PCSC_LOG_DEBUG);

	(void)Lun;

	// close device
	libusb_close(device_handle);
	device_handle = NULL;

	// close libusb
	libusb_exit(ctx);
	ctx = NULL;

	return IFD_NOT_SUPPORTED;
}

RESPONSECODE IFDHGetCapabilities ( DWORD Lun, DWORD Tag,
				   PDWORD Length, PUCHAR Value ) {

  /* This function should get the slot/card capabilities for a
	 particular slot/card specified by Lun.  Again, if you have only 1
	 card slot and don't mind loading a new driver for each reader then
	 ignore Lun.

     Tag - the tag for the information requested
         example: TAG_IFD_ATR - return the Atr and it's size (required).
         these tags are defined in ifdhandler.h

     Length - the length of the returned data
     Value  - the value of the data

     returns:

     IFD_SUCCESS
     IFD_ERROR_TAG
  */

	Log2(PCSC_LOG_DEBUG, "tag: 0x%04lX", Tag);

	(void)Lun;
	(void)Length;
	(void)Value;

	return IFD_NOT_SUPPORTED;
}

RESPONSECODE IFDHSetCapabilities ( DWORD Lun, DWORD Tag,
			       DWORD Length, PUCHAR Value ) {

  /* This function should set the slot/card capabilities for a particular
	 slot/card specified by Lun.  Again, if you have only 1 card slot
	 and don't mind loading a new driver for each reader then ignore
	 Lun.

     Tag - the tag for the information needing set

     Length - the length of the returned data
     Value  - the value of the data

     returns:

     IFD_SUCCESS
     IFD_ERROR_TAG
     IFD_ERROR_SET_FAILURE
     IFD_ERROR_VALUE_READ_ONLY
  */

	Log0(PCSC_LOG_DEBUG);

	(void)Lun;
	(void)Tag;
	(void)Length;
	(void)Value;

	return IFD_NOT_SUPPORTED;
}

RESPONSECODE IFDHSetProtocolParameters ( DWORD Lun, DWORD Protocol,
				   UCHAR Flags, UCHAR PTS1,
				   UCHAR PTS2, UCHAR PTS3) {

  /* This function should set the PTS of a particular card/slot using
     the three PTS parameters sent

     Protocol  - 0 .... 14  T=0 .... T=14
     Flags     - Logical OR of possible values:
     IFD_NEGOTIATE_PTS1 IFD_NEGOTIATE_PTS2 IFD_NEGOTIATE_PTS3
     to determine which PTS values to negotiate.
     PTS1, PTS2, PTS3 - PTS Values.

     returns:

     IFD_SUCCESS
     IFD_ERROR_PTS_FAILURE
     IFD_COMMUNICATION_ERROR
     IFD_PROTOCOL_NOT_SUPPORTED
  */

	Log0(PCSC_LOG_DEBUG);

	(void)Lun;
	(void)Protocol;
	(void)Flags;
	(void)PTS1;
	(void)PTS2;
	(void)PTS3;

	return IFD_NOT_SUPPORTED;
}


RESPONSECODE IFDHPowerICC ( DWORD Lun, DWORD Action,
			    PUCHAR Atr, PDWORD AtrLength ) {

  /* This function controls the power and reset signals of the smartcard
	 reader at the particular reader/slot specified by Lun.

     Action - Action to be taken on the card.

     IFD_POWER_UP - Power and reset the card if not done so
     (store the ATR and return it and it's length).

	 IFD_POWER_DOWN - Power down the card if not done already
	 (Atr/AtrLength should be zero'd)

	 IFD_RESET - Perform a quick reset on the card.  If the card is not
	 powered power up the card.
	 (Store and return the Atr/Length)

	 Atr - Answer to Reset of the card.  The driver is responsible for
	 caching this value in case IFDHGetCapabilities is called requesting
	 the ATR and it's length.  This should not exceed MAX_ATR_SIZE.

     AtrLength - Length of the Atr.  This should not exceed MAX_ATR_SIZE.

     Notes:

     Memory cards without an ATR should return IFD_SUCCESS on reset
     but the Atr should be zero'd and the length should be zero

     Reset errors should return zero for the AtrLength and return
     IFD_ERROR_POWER_ACTION.

     returns:

     IFD_SUCCESS
     IFD_ERROR_POWER_ACTION
     IFD_COMMUNICATION_ERROR
     IFD_NOT_SUPPORTED
  */

	Log0(PCSC_LOG_DEBUG);

	(void)Lun;

	switch (Action)
	{
		case IFD_POWER_DOWN:
			*AtrLength = 0;
			break;

		case IFD_POWER_UP:
		case IFD_RESET:
			if (!card_present)
				return IFD_ERROR_POWER_ACTION;

			if (!uid_len)
				return IFD_ERROR_POWER_ACTION;

			unsigned char default_atr[] = {0x3B, 0x8F, 0x80, 0x01, 0x80, 0x4F, 0x0C, 0xA0, 0x00, 0x00, 0x03, 0x06, 0x03, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00, 0x69};

			memcpy(Atr, default_atr, sizeof default_atr);
			*AtrLength = sizeof default_atr;
			break;
	}


	return IFD_SUCCESS;
}

RESPONSECODE IFDHTransmitToICC ( DWORD Lun, SCARD_IO_HEADER SendPci,
				 PUCHAR TxBuffer, DWORD TxLength,
				 PUCHAR RxBuffer, PDWORD RxLength,
				 PSCARD_IO_HEADER RecvPci ) {

  /* This function performs an APDU exchange with the card/slot specified by
	 Lun.  The driver is responsible for performing any protocol
	 specific exchanges such as T=0/1 ... differences.  Calling this
	 function will abstract all protocol differences.

     SendPci
     Protocol - 0, 1, .... 14
     Length   - Not used.

     TxBuffer - Transmit APDU example (0x00 0xA4 0x00 0x00 0x02 0x3F 0x00)
     TxLength - Length of this buffer.
     RxBuffer - Receive APDU example (0x61 0x14)
     RxLength - Length of the received APDU.  This function will be passed
     the size of the buffer of RxBuffer and this function is responsible for
     setting this to the length of the received APDU.  This should be ZERO
     on all errors.  The resource manager will take responsibility of zeroing
     out any temporary APDU buffers for security reasons.

     RecvPci
     Protocol - 0, 1, .... 14
     Length   - Not used.

     Notes:
	 The driver is responsible for knowing what type of card it has.  If
	 the current slot/card contains a memory card then this command
	 should ignore the Protocol and use the MCT style commands for
	 support for these style cards and transmit them appropriately.  If
	 your reader does not support memory cards or you don't want to then
	 ignore this.


     returns:

     IFD_SUCCESS
     IFD_COMMUNICATION_ERROR
     IFD_RESPONSE_TIMEOUT
     IFD_ICC_NOT_PRESENT
     IFD_PROTOCOL_NOT_SUPPORTED
  */

	Log0(PCSC_LOG_DEBUG);

	(void)Lun;
	(void)SendPci;
	(void)RecvPci;

	unsigned char getUID[] = {0xFF, 0xCA, 0x00, 0x00, 0x00};

	// only the getUID APDU command is supported
	if ((TxLength != sizeof getUID) || memcmp(TxBuffer, getUID, sizeof getUID))
	{
		// unsupported command
		RxBuffer[0] = 0x68;
		RxBuffer[1] = 0;
		*RxLength = 2;
	}
	else
	{
		// copy the UID we received in IFDHICCPresence()
		memcpy(RxBuffer, uid, uid_len);
		RxBuffer[uid_len] = 0x90;
		RxBuffer[uid_len+1] = 0x00;
		*RxLength = uid_len +2;
	}
	return IFD_SUCCESS;
}

RESPONSECODE IFDHControl ( DWORD Lun, DWORD dwControlCode,
			PUCHAR TxBuffer, DWORD TxLength,
			PUCHAR RxBuffer, DWORD RxLength,
			LPDWORD pdwBytesReturned) {

  /* This function performs a data exchange with the reader (not the card)
     specified by Lun.  Here XXXX will only be used.
     It is responsible for abstracting functionality such as PIN pads,
     biometrics, LCD panels, etc.  You should follow the MCT, CTBCS
     specifications for a list of accepted commands to implement.

     TxBuffer - Transmit data
     TxLength - Length of this buffer.
     RxBuffer - Receive data
     RxLength - Length of the received data.  This function will be passed
     the length of the buffer RxBuffer and it must set this to the length
     of the received data.

     Notes:
     RxLength should be zero on error.
  */

	Log0(PCSC_LOG_DEBUG);

	(void)Lun;
	(void)dwControlCode;
	(void)TxBuffer;
	(void)TxLength;
	(void)RxBuffer;
	(void)RxLength;
	(void)pdwBytesReturned;

	return IFD_NOT_SUPPORTED;
}

RESPONSECODE IFDHICCPresence( DWORD Lun ) {

  /* This function returns the status of the card inserted in the
     reader/slot specified by Lun.  It will return either:

     returns:
     IFD_ICC_PRESENT
     IFD_ICC_NOT_PRESENT
     IFD_COMMUNICATION_ERROR
  */

	Log0(PCSC_LOG_DEBUG);

	(void)Lun;

	unsigned char res[1024] = {0};
	int res_size = sizeof res;
	static bool card_was_present = false;

	Escape(command_00025, sizeof command_00025, res, &res_size);
	if (res[54] != 13)
	{
		Escape(command_02469, sizeof command_02469, NULL, NULL);
		Escape(command_02470, sizeof command_02470, NULL, NULL);

		res_size = sizeof res;
		Escape(command_02471, sizeof command_02471, res, &res_size);

#define UID_OFFSET 8
		if (res_size > UID_OFFSET)
		{
			int len = res[UID_OFFSET -1] -1;
			Log2(PCSC_LOG_INFO, "size: %d", len);
			log_xxd(PCSC_LOG_INFO, "UID: ", res + UID_OFFSET, len);

			// compute checksum
			int chk = res[UID_OFFSET + len];
			int b = chk;
			for (int i=0; i<len; i++)
				b ^= res[UID_OFFSET + i];
			if (b)
			{
				char collision[] = {0, 0, 0};
				if (0 == memcmp(collision, res + UID_OFFSET +1, sizeof collision))
				{
					Log1(PCSC_LOG_INFO, "collision detected");
					card_present = true;
					card_was_present = true;
					uid_len = 0;
				}
				else
					Log1(PCSC_LOG_CRITICAL, "invalid UID checksum");
			}
			else
			{
				memcpy(uid, res + UID_OFFSET, len);
				uid_len = len;

				card_present = true;
				card_was_present = true;
			}
		}
	}
	else
	{
		if (card_was_present)
		{
			card_present = true;
			card_was_present = false;
		}
		else
			card_present = false;
	}

	Escape(command_00027, sizeof command_00027, NULL, NULL);
	Escape(command_00028, sizeof command_00028, NULL, NULL);
	Escape(command_00029, sizeof command_00029, NULL, NULL);
	Escape(command_00030, sizeof command_00030, NULL, NULL);

	return card_present ? IFD_ICC_PRESENT : IFD_ICC_NOT_PRESENT;

error:
	return IFD_COMMUNICATION_ERROR;
}

