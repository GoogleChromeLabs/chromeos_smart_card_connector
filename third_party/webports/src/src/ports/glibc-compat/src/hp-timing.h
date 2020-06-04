#ifndef _HP_TIMING_H
#define _HP_TIMING_H	1

/* Just dummy stubs. */
#define HP_TIMING_AVAIL		(0)
#define HP_TIMING_INLINE	(0)
typedef int hp_timing_t;
#define HP_TIMING_ZERO(Var)
#define HP_TIMING_NOW(var)
#define HP_TIMING_DIFF_INIT()
#define HP_TIMING_DIFF(Diff, Start, End)
#define HP_TIMING_ACCUM(Sum, Diff)
#define HP_TIMING_ACCUM_NT(Sum, Diff)
#define HP_TIMING_PRINT(Buf, Len, Val)

/* Since this implementation is not available we tell the user about it.  */
#define HP_TIMING_NONAVAIL	1

#endif	/* hp-timing.h */
