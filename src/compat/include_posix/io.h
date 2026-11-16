/*
 * io.h - POSIX stand-in for the Borland/Microsoft <io.h>.
 * (On Windows the C run-time's own <io.h> is used.)
 */
#ifndef KK_IO_H
#define KK_IO_H

#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "kk_platform.h"   /* eof(), filelength(), tell(), lock(), unlock(), _commit() */

#endif /* KK_IO_H */
