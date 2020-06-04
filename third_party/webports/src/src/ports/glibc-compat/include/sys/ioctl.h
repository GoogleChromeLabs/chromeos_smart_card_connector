/* libc/sys/linux/sys/ioctl.h - ioctl prototype */

/* Written 2000 by Werner Almesberger */


#ifndef _SYS_IOCTL_H
#define _SYS_IOCTL_H

/* TODO(olonho): hack */
#if 0
#include <bits/ioctls.h>
#endif

#define TCGETS		0x5401
#define TCSETS		0x5402
#define TCSETSW		0x5403
#define TCSETSF		0x5404
#define TCGETA		0x5405
#define TCSETA		0x5406
#define TCSETAW		0x5407
#define TCSETAF		0x5408
#define TIOCGPGRP	0x540F
#define TIOCGWINSZ	0x5413
#define TIOCSWINSZ	0x5414
#define TCXONC          0x540A

struct winsize
  {
    unsigned short int ws_row;
    unsigned short int ws_col;
    unsigned short int ws_xpixel;
    unsigned short int ws_ypixel;
  };

#define NCC 8
struct termio
  {
    unsigned short int c_iflag;		/* input mode flags */
    unsigned short int c_oflag;		/* output mode flags */
    unsigned short int c_cflag;		/* control mode flags */
    unsigned short int c_lflag;		/* local mode flags */
    unsigned char c_line;		/* line discipline */
    unsigned char c_cc[NCC];		/* control characters */
};

/* modem lines */
#define TIOCM_LE	0x001
#define TIOCM_DTR	0x002
#define TIOCM_RTS	0x004
#define TIOCM_ST	0x008
#define TIOCM_SR	0x010
#define TIOCM_CTS	0x020
#define TIOCM_CAR	0x040
#define TIOCM_RNG	0x080
#define TIOCM_DSR	0x100
#define TIOCM_CD	TIOCM_CAR
#define TIOCM_RI	TIOCM_RNG

/* ioctl (fd, TIOCSERGETLSR, &result) where result may be as below */

/* line disciplines */
#define N_TTY		0
#define N_SLIP		1
#define N_MOUSE		2
#define N_PPP		3
#define N_STRIP		4
#define N_AX25		5
#define N_X25		6	/* X.25 async  */
#define N_6PACK		7
#define N_MASC		8	/* Mobitex module  */
#define N_R3964		9	/* Simatic R3964 module  */
#define N_PROFIBUS_FDL	10	/* Profibus  */
#define N_IRDA		11	/* Linux IR  */
#define N_SMSBLOCK	12	/* SMS block mode  */
#define N_HDLC		13	/* synchronous HDLC  */
#define N_SYNC_PPP	14	/* synchronous PPP  */
#define	N_HCI		15	/* Bluetooth HCI UART  */

#define TCGETA		0x5405
#define TCSETA		0x5406

#define FIONREAD 0x541B
#define FIONBIO 0x5421

#define SIOCGIFCONF	0x8912		/* get iface list		*/
#define SIOCGIFFLAGS	0x8913		/* get flags			*/
#define SIOCGIFDSTADDR	0x8917		/* get remote PA address	*/
#define SIOCGIFBRDADDR	0x8919		/* get broadcast PA address	*/
#define SIOCGIFNETMASK	0x891b		/* get network PA mask		*/

/* end olonho hack */

#ifdef __cplusplus
extern "C" {
#endif

int ioctl(int fd,int request,...);

#ifdef __cplusplus
}
#endif

#endif
