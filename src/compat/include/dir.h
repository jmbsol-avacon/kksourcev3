/*
 * dir.h - Borland <dir.h> replacements (drive and directory helpers).
 */
#ifndef KK_DIR_H
#define KK_DIR_H

#ifdef __cplusplus
extern "C" {
#endif

int   getdisk(void);                 /* 0 = A:, 1 = B:, 2 = C: ...        */
int   setdisk(int drive);            /* returns the number of drives      */
char *getcurdir(int drive, char *directory);

#ifdef __cplusplus
}
#endif

#endif /* KK_DIR_H */
