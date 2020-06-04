/* Uncancelable open.  */
#define open_not_cancel(name, flags, mode) open((name), (flags), (mode))
#define open_not_cancel_2(name, flags) open((name), (flags))

/* Uncancelable close.  */
#define close_not_cancel(fd) close((fd))
#define close_not_cancel_no_status(fd) (void)close((fd))

/* Uncancelable read.  */
#define read_not_cancel(fd, buf, n) read((fd), (buf), (n))

/* Uncancelable write.  */
#define write_not_cancel(fd, buf, n) write((fd), (buf), (n))

/* Uncancelable writev.  */
#define writev_not_cancel_no_status(fd, iov, n) \
  (void) writev((fd), (iov), (n))

/* Uncancelable fcntl.  */
#define fcntl_not_cancel(fd, cmd, val) fcntl((fd), (cmd), (val))
