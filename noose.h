/* 
 * noose.h
 * Created: Sun Feb 25 22:13:41 2001 by tek@wiw.org
 * Revised: Sun Feb 25 22:13:41 2001 (pending)
 * Copyright 2001 Julian E. C. Squires (tek@wiw.org)
 * This program comes with ABSOLUTELY NO WARRANTY.
 * $Id$
 * 
 */

#ifndef NOOSE_H
#define NOOSE_H

#define PROGNAME "noose"

typedef struct rangelist_s {
    int begin, end;
    struct rangelist_s *next;
} rangelist_t;

extern void *nntp_connect(char *);
extern void nntp_disconnect(void *);
extern int nntp_cmd_group(void *, char *, int *, int *, int *);
extern int nntp_cmd_article(void *, int, char **, char **);

extern int newsrc_filter(char *, char *, rangelist_t **);
extern int newsrc_getsubscribedgroups(char *, int *, char ***);

extern rangelist_t *rl_new(int, int, rangelist_t *);
extern void rl_delete(rangelist_t *);
extern void rl_exclude(rangelist_t **, int, int);

#endif

/* EOF noose.h */
