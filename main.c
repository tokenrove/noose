/* 
 * main.c
 * Created: Sun Feb 25 20:11:31 2001 by tek@wiw.org
 * Revised: Sun Apr 22 19:09:44 2001 by tek@wiw.org
 * Copyright 2001 Julian E. C. Squires (tek@wiw.org)
 * This program comes with ABSOLUTELY NO WARRANTY.
 * $Id$
 * 
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "noose.h"

typedef struct noose_s {
    int verbose;
    void *nh;
    char *newsrc;
} noose_t;

void usage(char *msg);
int do_check(noose_t *ndat, char *group);
int checksingle(noose_t *ndat, char *group);
int do_read(noose_t *ndat, char *group, int article);
int setnewsrc(noose_t *ndat);

int main(int argc, char **argv)
{
    enum { nocommand, check, read } command = nocommand;
    char *group, *server;
    noose_t ndat;
    int i, j, article, ret;

    ndat.verbose = 1;
    ndat.newsrc = NULL;
    server = getenv("NNTPSERVER");

    for(i = 1; i < argc; i++) {
        if(argv[i][0] == '-') {
            for(j = 1; j < strlen(argv[i]); j++)
                switch(argv[i][j]) {
                case 'q':
                    ndat.verbose--;
                    break;

                case 'v':
                    ndat.verbose++;
                    break;

                case 'n':
                    i++;
                    if(i == argc)
                        usage("Option `n' requires an argument.");
                    server = argv[i];
                    break;

                case 'r':
                    i++;
                    if(i == argc)
                        usage("Option `r' requires an argument.");
                    ndat.newsrc = malloc(strlen(argv[i])+1);
                    strcpy(ndat.newsrc, argv[i]);
                    break;

                default:
                    usage("Bad option.");
                }
        } else if(strcmp(argv[i], "check") == 0) {
            command = check;
            i++;
            if(i < argc) {
                group = argv[i];
            } else
                group = NULL;

        } else if(strcmp(argv[i], "read") == 0) {
            command = read;
            i++;
            if(i+1 >= argc) usage("Too few arguments for `read'.");
            group = argv[i];
            i++;
            article = atoi(argv[i]);
            if(article == 0) usage("Invalid article number.");

        } else
            usage("Bad argument.");
    }
    if(command == nocommand) {
        usage("No command specified.");
    }

    if(server == NULL)
        server = DEFNNTPSERVER;

    ndat.nh = nntp_connect(server);
    if(ndat.nh == NULL) {
        if(ndat.verbose >= 0)
            fprintf(stderr, "%s: Failed to connect to `%s'.\n", PROGNAME,
                    server);
        exit(EXIT_FAILURE);
    }

    switch(command) {
    case check:
        ret = do_check(&ndat, group);
        break;

    case read:
        ret = do_read(&ndat, group, article);
        break;

    default:
        fprintf(stderr, "Command unsupported.\n");
        exit(EXIT_FAILURE);
    }

    if(ndat.newsrc) free(ndat.newsrc);
    nntp_disconnect(ndat.nh);
    exit((ret == 0)?EXIT_SUCCESS:EXIT_FAILURE);
}

int setnewsrc(noose_t *ndat)
{
    char *home;
    char *newsrcs[] = { "/.jnewsrc", "/.newsrc", NULL };
    int i;
    FILE *fp;

    home = getenv("HOME");
    if(home == NULL) {
        fprintf(stderr, "%s: Please set HOME in your environment.\n",
                PROGNAME);
        return -1;
    }
    for(i = 0; newsrcs[i] != NULL; i++) {
        ndat->newsrc = malloc(strlen(home)+strlen(newsrcs[i]));
        strcpy(ndat->newsrc, home);
        strcat(ndat->newsrc, newsrcs[i]);
        if((fp = fopen(ndat->newsrc, "r")) != NULL) {
            fclose(fp);
            break;
        }
        free(ndat->newsrc);
	ndat->newsrc = NULL;
    }
    if(newsrcs[i] == NULL) {
        fprintf(stderr, "%s: Please specify a newsrc file to use.\n",
                PROGNAME);
        return -1;
    }
    return 0;
}

int do_check(noose_t *ndat, char *group)
{
    int nsubbedgroups, i;
    char **subbedgroups;

    if(ndat->newsrc == NULL)
        if(setnewsrc(ndat) != 0) return -1;

    if(group == NULL) {
        i = newsrc_getsubscribedgroups(ndat->newsrc, &nsubbedgroups,
                                       &subbedgroups);
        if(i != 0) {
            if(ndat->verbose >= 0)
                fprintf(stderr, "%s: Couldn't read subscribed groups from "
                        "your newsrc.\n", PROGNAME);
            return -1;
        }
        for(i = 0; i < nsubbedgroups; i++) {
            checksingle(ndat, subbedgroups[i]);
            free(subbedgroups[i]);
        }
    } else {
        checksingle(ndat, group);
    }

    return 0;
}

int checksingle(noose_t *ndat, char *group)
{
    rangelist_t *rl, *p;
    int i, narticles;

    rl = rl_new(0, 0, NULL);

    if(nntp_cmd_group(ndat->nh, group, NULL, &rl->begin, &rl->end) != 0) {
        if(ndat->verbose >= 0)
            fprintf(stderr, "%s: nntp_cmd_group failed.\n", PROGNAME);
        return -1;
    }

    newsrc_filter(ndat->newsrc, group, &rl);

    narticles = 0;
    p = rl;
    while(p != NULL) {
        for(i = p->begin; i > 0 && i <= p->end; i++) {
            if(nntp_cmd_article(ndat->nh, i, NULL, NULL) == 0) {
                narticles++;
            }
        }
        p = p->next;
    }

    if(narticles > 0 || ndat->verbose > 0) {
        printf("There %s %d unread article%s in newsgroup %s.\n",
               (narticles == 1)?"is":"are", narticles,
               (narticles == 1)?"":"s", group);
    }

    rl_delete(rl);
    return 0;
}

int do_read(noose_t *ndat, char *group, int article)
{
    char *artbody, *arthead;

    if(nntp_cmd_group(ndat->nh, group, NULL, NULL, NULL) == -1) {
        if(ndat->verbose >= 0)
            fprintf(stderr, "%s: nntp_cmd_group failed.\n", PROGNAME);
        return -1;
    }
    
    if(nntp_cmd_article(ndat->nh, article, &arthead, &artbody) == -1) {
        if(ndat->verbose >= 0)
            fprintf(stderr, "%s: nntp_cmd_article failed.\n", PROGNAME);
        return -1;
    }

    printf("%s%s", arthead, artbody);
    free(arthead);
    free(artbody);
    return 0;
}

void usage(char *msg)
{
    printf("%s: %s\n\n", PROGNAME, msg);
    printf("usage: %s [options...] <command>\n\n"
"Valid options are:\n"
"  -q                    Quiet mode. This makes `check' only report groups\n"
"                        with more than zero new messages in them.\n"
"  -v                    Verbose. The inverse of quiet.\n"
"  -n server             Specify NNTP server.\n"
"  -r rcfile             Specify newsrc file.\n"
"\n"
"Valid commands are:\n"
"  check [g]             Return how many unread posts are in all subscribed\n"
"                        groups, or just group g if specified.\n"
"  read <g> <n>          Grab the entirity of article n from group g.\n",
           PROGNAME);
    exit(EXIT_FAILURE);
}

/* EOF main.c */
