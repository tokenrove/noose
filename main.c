/* 
 * main.c
 * Created: Sun Feb 25 20:11:31 2001 by tek@wiw.org
 * Revised: Sun Feb 25 20:11:31 2001 (pending)
 * Copyright 2001 Julian E. C. Squires (tek@wiw.org)
 * This program comes with ABSOLUTELY NO WARRANTY.
 * $Id$
 * 
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "noose.h"

void usage(char *msg);
void do_check(void *nh, char *group);
void do_read(void *nh, char *group, int article);

int main(int argc, char **argv)
{
    int i, article;
    enum { nocommand, check, list, read } command = nocommand;
    char *group, *server;
    void *nh;

    for(i = 1; i < argc; i++) {
        if(argv[i][0] == '-') {
        } else if(strcmp(argv[i], "check") == 0) {
            command = check;
            i++;
            if(i >= argc) usage("Too few arguments for `check'.");
            group = argv[i];

        } else if(strcmp(argv[i], "list") == 0) {
            command = list;

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

    server = getenv("NNTPSERVER");
    nh = nntp_connect(server);
    if(nh == NULL) {
        fprintf(stderr, "%s: Failed to connect to `%s'.\n", PROGNAME,
                server);
        exit(EXIT_FAILURE);
    }

    switch(command) {
    case check:
        do_check(nh, group);
        break;

    case read:
        do_read(nh, group, article);
        break;

    default:
        fprintf(stderr, "Command unsupported.\n");
        exit(EXIT_FAILURE);
    }

    nntp_disconnect(nh);
    exit(EXIT_SUCCESS);
}

void do_check(void *nh, char *group)
{
    rangelist_t *rl, *p;
    char *newsrc;
    int i, narticles;

    rl = rl_new(0, 0, NULL);

    if(nntp_cmd_group(nh, group, NULL, &rl->begin, &rl->end) != 0) {
        fprintf(stderr, "%s: nntp_cmd_group failed.\n", PROGNAME);
        exit(EXIT_FAILURE);
    }

    newsrc = strdup(getenv("HOME"));
    strcat(newsrc, "/.jnewsrc");
    newsrc_filter(newsrc, group, &rl);
    free(newsrc);

    narticles = 0;
    p = rl;
    while(p != NULL) {
        for(i = p->begin; i > 0 && i <= p->end; i++) {
            if(nntp_cmd_article(nh, i, NULL, NULL) == 0) {
                narticles++;
            }
        }
        p = p->next;
    }

    printf("There %s %d unread article%s in newsgroup %s.\n",
           (narticles == 1)?"is":"are", narticles,
           (narticles == 1)?"":"s", group);

    rl_delete(rl);
    return;
}

void do_read(void *nh, char *group, int article)
{
    char *artbody, *arthead;

    if(nntp_cmd_group(nh, group, NULL, NULL, NULL) == -1) {
        fprintf(stderr, "%s: nntp_cmd_group failed.\n", PROGNAME);
        exit(EXIT_FAILURE);
    }
    
    if(nntp_cmd_article(nh, article, &arthead, &artbody) == -1) {
        fprintf(stderr, "%s: nntp_cmd_article failed.\n", PROGNAME);
        exit(EXIT_FAILURE);
    }

    printf("%s%s", arthead, artbody);
    free(arthead);
    free(artbody);
    return;
}

void usage(char *msg)
{
    printf("%s: %s\n\n", PROGNAME, msg);
    printf("usage: %s [options...] <command>\n\n"
           "Valid commands are:\n"
"  check <g>             Return how many new posts are in group g.\n"
"  list                  List the known groups on the server.\n"
"  read <g> <n>          Grab the entirity of article n from group g.\n",
           PROGNAME);
    exit(EXIT_FAILURE);
}

/* EOF main.c */
