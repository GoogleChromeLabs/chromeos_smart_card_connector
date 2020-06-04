#ifndef _SGTTY_H
#define _SGTTY_H 1

#include <sys/ioctl.h>

struct sgttyb;

#ifdef __cplusplus
extern "C" {
#endif

extern int gtty (int fd, struct sgttyb *params);
extern int stty (int fd, const struct sgttyb *params);

#ifdef __cplusplus
}
#endif

#endif
