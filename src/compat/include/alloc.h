/*
 * alloc.h - Borland <alloc.h>: everything useful is in <stdlib.h>; the
 * memory statistics come from dos.h.
 */
#ifndef KK_ALLOC_H
#define KK_ALLOC_H

#include <stdlib.h>
#include "dos.h"

#define farmalloc(n)      malloc(n)
#define farcalloc(n, s)   calloc((n), (s))
#define farrealloc(p, n)  realloc((p), (n))
#define farfree(p)        free(p)

#endif /* KK_ALLOC_H */
