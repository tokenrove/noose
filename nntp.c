/* 
 * nntp.c
 * Created: Sun Feb 25 20:37:47 2001 by tek@wiw.org
 * Revised: Sun Feb 25 20:37:47 2001 (pending)
 * Copyright 2001 Julian E. C. Squires (tek@wiw.org)
 * This program comes with ABSOLUTELY NO WARRANTY.
 * $Id$
 * 
 */

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>

#include "noose.h"

#define OURMAGIC    0xDEADBEEF
#define NNTPSERVICE "nntp"
#define NNTPPROTO   "tcp"

typedef struct nntp_s {
    long magic;
    int sock;
} nntp_t;

void *nntp_connect(char *hostname);
void nntp_disconnect(void *nh_);
int nntp_cmd_group(void *nh_, char *group, int *narticles_, int *firstart_,
                   int *lastart_);
int nntp_cmd_article(void *nh_, int artnum, char **head, char **body);

static int nntp_readresponse(nntp_t *nh, char **resbody);
static int nntp_readheader(nntp_t *nh, char **header);
static int nntp_readbody(nntp_t *nh, char **body);

static char *responses[6][5][10] = {
    { { "" } }, /* 0 */
    { { "" } }, /* 1 */
    { { "" } }, /* 2 */
    { { "" } }, /* 3 */
    /* 4** */
    {   /* 40* */
        { "Service discontinued." /* 400 */ 
          "", "", "", "", "", "", "", "", "" }, 
        /* 41* */
        { "", /* 410 */
          "No such news group.", /* 411 */
          "No newsgroup has been selected.", /* 412 */
          "", "", "", "", "", "", "" }, 
        /* 42* */
        { "No current article has been selected.", /* 420 */
          "No next article in this group.", /* 421 */
          "No previous article in this group.", /* 422 */
          "No such article number in this group.", /* 423 */
          "", "", "", "", "", "" },
        /* 43* */
        { "No such article found.", /* 430 */
          "", "", "", "",
          "Article not wanted - do not send it.", /* 435 */
          "Transfer failed - try again later.", /* 436 */
          "Article rejected - do not try again.", /* 437 */
          "", "" },
        /* 44* */
        { "Posting not allowed.", /* 440 */
          "Posting failed.", /* 441 */
          "", "", "", "", "", "", "", "" }
    },

    /* 5** */
    {
        /* 50* */
        { "Command not recognized.", /* 500 */
          "Command syntax error.", /* 501 */
          "Access restriction or permission denied.", /* 502 */
          "Program fault - command not performed.", /* 503 */
          "", "", "", "", "", "" }
    }
};

void *nntp_connect(char *hostname)
{
    nntp_t *nh;
    struct hostent *hostent;
    struct servent *servent;
    struct sockaddr_in sockaddr;
    int i;

    nh = malloc(sizeof(nntp_t));
    if(nh == NULL) {
        fprintf(stderr, "%s: Out of memory.\n", PROGNAME);
        return NULL;
    }

    nh->magic = OURMAGIC;
    nh->sock = socket(AF_INET, SOCK_STREAM, 0);
    if(nh->sock == -1) {
        fprintf(stderr, "%s: %s\n", PROGNAME, strerror(errno));
        return NULL;
    }

    sockaddr.sin_family = AF_INET;
    hostent = gethostbyname(hostname);
    if(hostent == NULL) {
        fprintf(stderr, "%s: %s\n", PROGNAME, strerror(errno));
        return NULL;
    }
    memcpy(&sockaddr.sin_addr, hostent->h_addr_list[0], hostent->h_length);

    servent = getservbyname(NNTPSERVICE, NNTPPROTO);
    if(servent == NULL) {
        fprintf(stderr, "%s: %s\n", PROGNAME, strerror(errno));
        return NULL;
    }
    sockaddr.sin_port = servent->s_port;

    if(connect(nh->sock, (const struct sockaddr *)&sockaddr,
               sizeof(sockaddr)) == -1) {
        fprintf(stderr, "%s: %s\n", PROGNAME, strerror(errno));
        return NULL;
    }

    i = nntp_readresponse(nh, NULL);
    if(i != 200) {
        fprintf(stderr, "%s: %s\n", PROGNAME, responses[i/100][i/10%10][i%10]);
        return NULL;
    }

    return (void *)nh;
}

int nntp_readresponse(nntp_t *nh, char **resbody)
{
    char *buffer;
    int respnum, blen = 128, i, bufp;

    buffer = malloc(blen);
    if(buffer == NULL) {
        fprintf(stderr, "%s: Out of memory in nntp_readresponse.\n",
                PROGNAME);
        return 0;
    }
    bufp = 0;

    do {
        i = read(nh->sock, buffer+bufp, 1);
        if(i == -1) {
            fprintf(stderr, "%s: %s\n", PROGNAME, strerror(errno));
            return 0;
        }

        bufp += i;

        if(bufp >= blen) {
            blen *= 2;
            buffer = realloc(buffer, blen);
            if(buffer == NULL) {
                fprintf(stderr, "%s: Out of memory in "
                        "nntp_readresponse.\n", PROGNAME);
                return 0;
            }
        }
    } while(!(buffer[bufp-2] == '\r' && buffer[bufp-1] == '\n'));
    buffer[bufp] = 0;

    respnum = atoi(buffer);
    if(resbody != NULL)
        *resbody = buffer;
    else
        free(buffer);

    return respnum;
}

int nntp_readheader(nntp_t *nh, char **header)
{
    char *buffer;
    int blen = 128, i, bufp;

    buffer = malloc(blen);
    if(buffer == NULL) {
        fprintf(stderr, "%s: Out of memory in nntp_readheader.\n",
                PROGNAME);
        return -1;
    }
    bufp = 0;

    do {
        i = read(nh->sock, buffer+bufp, 1);
        if(i == -1) {
            fprintf(stderr, "%s: %s\n", PROGNAME, strerror(errno));
            return -1;
        }

        bufp += i;

        if(bufp >= blen) {
            blen *= 2;
            buffer = realloc(buffer, blen);
            if(buffer == NULL) {
                fprintf(stderr, "%s: Out of memory in "
                        "nntp_readheader.\n", PROGNAME);
                return -1;
            }
        }
    } while(!(buffer[bufp-1] == '\n' &&
              buffer[bufp-2] == '\r' &&
              (buffer[bufp-3] == '.' ||
               (buffer[bufp-3] == '\n' &&
                buffer[bufp-4] == '\r'))));
    buffer[bufp] = 0;

    if(header != NULL)
        *header = buffer;
    else
        free(buffer);

    return 0;
}

int nntp_readbody(nntp_t *nh, char **body)
{
    char *buffer;
    int blen = 128, i, bufp;

    buffer = malloc(blen);
    if(buffer == NULL) {
        fprintf(stderr, "%s: Out of memory in nntp_readbody.\n", PROGNAME);
        return -1;
    }
    bufp = 0;

    do {
        i = read(nh->sock, buffer+bufp, 1);
        if(i == -1) {
            fprintf(stderr, "%s: %s\n", PROGNAME, strerror(errno));
            return -1;
        }

        bufp += i;

        if(bufp >= blen) {
            blen *= 2;
            buffer = realloc(buffer, blen);
            if(buffer == NULL) {
                fprintf(stderr, "%s: Out of memory in "
                        "nntp_readbody.\n", PROGNAME);
                return -1;
            }
        }
    } while(!(buffer[bufp-1] == '\n' &&
              buffer[bufp-2] == '\r' &&
              buffer[bufp-3] == '.' &&
              buffer[bufp-4] == '\n' &&
              buffer[bufp-5] == '\r'));
    buffer[bufp-3] = 0;

    if(body != NULL)
        *body = buffer;
    else
        free(buffer);

    return 0;
}

void nntp_disconnect(void *nh_)
{
    nntp_t *nh = (nntp_t *)nh_;

    if(nh->magic != OURMAGIC) {
        fprintf(stderr, "%s: magic check failed for nntp_disconnect!\n",
                PROGNAME);
    }

    write(nh->sock, "QUIT\r\n", strlen("QUIT\r\n"));
    close(nh->sock);

    return;
}

int nntp_cmd_group(void *nh_, char *group, int *narticles, int *firstart,
                   int *lastart)
{
    nntp_t *nh = (nntp_t *)nh_;
    int i;
    char *respbody, *p, *cmdbuf;

    if(nh->magic != OURMAGIC) {
        fprintf(stderr, "%s: magic check failed for nntp_cmd_group!\n",
                PROGNAME);
    }

    cmdbuf = malloc(strlen("GROUP ")+strlen(group)+strlen("\r\n"));
    strcpy(cmdbuf, "GROUP ");
    strcat(cmdbuf, group);
    strcat(cmdbuf, "\r\n");
    write(nh->sock, cmdbuf, strlen(cmdbuf));
    free(cmdbuf);

    i = nntp_readresponse(nh, &respbody);
    if(i != 211) {
        fprintf(stderr, "%s: %s\n", PROGNAME, responses[i/100][i/10%10][i%10]);
        return -1;
    }

    p = strtok(respbody, " \t\n");
    /* response code */
    p = strtok(NULL, " \t\n");
    if(narticles != NULL) *narticles = atoi(p);
    p = strtok(NULL, " \t\n");
    if(firstart != NULL) *firstart = atoi(p);
    p = strtok(NULL, " \t\n");
    if(lastart != NULL) *lastart = atoi(p);
    /* next would be group name */

    free(respbody);
    return 0;
}

#define MAXARTNUMLEN 10

int nntp_cmd_article(void *nh_, int artnum, char **head, char **body)
{
    nntp_t *nh = (nntp_t *)nh_;
    int i;
    char *respbody, *cmdbuf, *cmd;

    if(nh->magic != OURMAGIC) {
        fprintf(stderr, "%s: magic check failed for nntp_cmd_article!\n",
                PROGNAME);
    }

    if(head == NULL && body == NULL)
        cmd = "STAT ";
    else if(head == NULL)
        cmd = "BODY ";
    else if(body == NULL)
        cmd = "HEAD ";
    else
        cmd = "ARTICLE ";
    cmdbuf = malloc(strlen(cmd)+MAXARTNUMLEN+strlen("\r\n"));
    sprintf(cmdbuf, "%s %d\r\n", cmd, artnum);
    write(nh->sock, cmdbuf, strlen(cmdbuf));
    free(cmdbuf);

    i = nntp_readresponse(nh, &respbody);
    if(i/10 != 22) {
        fprintf(stderr, "%s: %s\n", PROGNAME, responses[i/100][i/10%10][i%10]);
        return -1;
    }
    if(head != NULL)
        if(nntp_readheader(nh, head) != 0) {
            fprintf(stderr, "%s: failed to read header.\n", PROGNAME);
            return -1;
        }
    if(body != NULL)
        if(nntp_readbody(nh, body) != 0) {
            fprintf(stderr, "%s: failed to read body.\n", PROGNAME);
            return -1;
        }

    return 0;
}

/* EOF nntp.c */
