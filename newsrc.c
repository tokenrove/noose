/* 
 * newsrc.c
 * Created: Sun Feb 25 22:07:34 2001 by tek@wiw.org
 * Revised: Sun Feb 25 22:07:34 2001 (pending)
 * Copyright 2001 Julian E. C. Squires (tek@wiw.org)
 * This program comes with ABSOLUTELY NO WARRANTY.
 * $Id$
 * 
 */

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "noose.h"

int newsrc_filter(char *rcfile, char *group, rangelist_t **rl);

int newsrc_filter(char *rcfile, char *group, rangelist_t **rl)
{
    FILE *fp;
    char *buffer, *p;
    int buflen = 80, begin, end;

    fp = fopen(rcfile, "r");
    if(fp == NULL) {
        fprintf(stderr, "%s: %s\n", PROGNAME, strerror(errno));
        return -1;
    }

    buffer = malloc(buflen);
    if(buffer == NULL) {
        fprintf(stderr, "%s: Out of memory.\n", PROGNAME);
        return -1;
    }

    do {
        fgets(buffer, buflen, fp);
        while(buffer[strlen(buffer)-1] != '\n') {
            buflen *= 2;
            buffer = realloc(buffer, buflen);
            if(buffer == NULL) {
                fprintf(stderr, "%s: Out of memory.\n", PROGNAME);
                return -1;
            }
            fgets(buffer+strlen(buffer)-1, buflen-strlen(buffer)-1, fp);
        }
    } while(strncmp(buffer, group, strlen(group)) != 0 && !feof(fp));

    if(!feof(fp)) {
        for(p = buffer; *p != ' ' && *p; p++);
        p++;

        p = strtok(p, ",\n");
        do {
            begin = atoi(p);
            if(strchr(p, '-') != NULL) {
                end = atoi(strchr(p, '-')+1);
            } else
                end = begin;
            rl_exclude(rl, begin, end);
        } while(p = strtok(NULL, ",\n"), p != NULL);
    }

    free(buffer);
    fclose(fp);
    return 0;
}

/* EOF newsrc.c */
