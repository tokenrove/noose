/* 
 * rl.c
 * Created: Sun Feb 25 22:30:43 2001 by tek@wiw.org
 * Revised: Sun Feb 25 22:30:43 2001 (pending)
 * Copyright 2001 Julian E. C. Squires (tek@wiw.org)
 * This program comes with ABSOLUTELY NO WARRANTY.
 * $Id$
 * 
 */

#include <stdio.h>
#include <stdlib.h>

#include "noose.h"

rangelist_t *rl_new(int begin, int end, rangelist_t *next);
void rl_delete(rangelist_t *rl);
void rl_exclude(rangelist_t **rl, int begin, int end);

rangelist_t *rl_new(int begin, int end, rangelist_t *next)
{
    rangelist_t *p;

    p = malloc(sizeof(rangelist_t));
    if(p == NULL) {
        fprintf(stderr, "%s: Out of memory.\n", PROGNAME);
        return NULL;
    }
    p->begin = begin;
    p->end = end;
    p->next = next;
    return p;
}

void rl_delete(rangelist_t *rl)
{
    rangelist_t *dead;

    while(rl != NULL) {
        dead = rl;
        rl = rl->next;
        free(dead);
    }
    return;
}

void rl_exclude(rangelist_t **rlp, int begin, int end)
{
    rangelist_t *rl;

    rl = *rlp;
    while(rl != NULL) {
        if(begin <= rl->begin && end >= rl->end) { /* no parts left */
            *rlp = rl->next;
            rl = rl->next;
            rl_delete(rl);
            continue;
        } else if(begin <= rl->begin && end >= rl->begin && end < rl->end) {
            /* shrink right */
            rl->begin = end+1;
        } else if(begin >= rl->begin && begin < rl->end && end >= rl->end) {
            /* shrink left */
            rl->end = begin-1;
        } else if(begin > rl->begin && end < rl->end) {
            /* two parts */
            rl->next = rl_new(end+1, rl->end, rl->next);
            rl->end = begin-1;
        }

        rl = rl->next;
    }
}

/* EOF rl.c */
