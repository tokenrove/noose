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


#define OURMAGIC    0xDEADBEEF
#define NNTPSERVICE "nntp"
#define NNTPPROTO   "tcp"

typedef struct nntp_s {
    long magic;
    int sock;
} nntp_t;

static char *cmd_quit = "QUIT\r\n";

void *nntp_connect(char *hostname);
void nntp_disconnect(void *nh_);
int nntp_cmd_group(void *nh_, char *group, int *narticles_, int *firstart_,
                   int *lastart_);
int nntp_cmd_article(void *nh_, int artnum, char **head, char **body);

static int nntp_readresponse(nntp_t *nh, char **resbody);
static int nntp_readheader(nntp_t *nh, char **header);
static int nntp_readbody(nntp_t *nh, char **body);

void *nntp_connect(char *hostname)
{
    nntp_t *nh;
    struct hostent *hostent;
    struct servent *servent;
    struct sockaddr_in sockaddr;
    int i;

    nh = malloc(sizeof(nntp_t));
    if(nh == NULL) {
        fprintf(stderr, "noose: Out of memory.\n");
        return NULL;
    }

    nh->magic = OURMAGIC;
    nh->sock = socket(AF_INET, SOCK_STREAM, 0);
    if(nh->sock == -1) {
        fprintf(stderr, "noose: %s\n", strerror(errno));
        return NULL;
    }

    sockaddr.sin_family = AF_INET;
    hostent = gethostbyname(hostname);
    if(hostent == NULL) {
        fprintf(stderr, "noose: %s\n", strerror(errno));
        return NULL;
    }
    memcpy(&sockaddr.sin_addr, hostent->h_addr_list[0], hostent->h_length);

    servent = getservbyname(NNTPSERVICE, NNTPPROTO);
    if(servent == NULL) {
        fprintf(stderr, "noose: %s\n", strerror(errno));
        return NULL;
    }
    sockaddr.sin_port = servent->s_port;

    if(connect(nh->sock, (const struct sockaddr *)&sockaddr,
               sizeof(sockaddr)) == -1) {
        fprintf(stderr, "noose: %s\n", strerror(errno));
        return NULL;
    }

    i = nntp_readresponse(nh, NULL);
    if(i != 200) {
        fprintf(stderr, "noose: got %d instead of 200 for response code!\n",
                i);
        return NULL;
    }

    return (void *)nh;
}

int nntp_readresponse(nntp_t *nh, char **resbody)
{
    char *buffer;
    int respnum, blen = 16, i, bufp;

    buffer = malloc(blen);
    if(buffer == NULL) {
        fprintf(stderr, "noose: Out of memory in nntp_readresponse.\n");
        return 0;
    }
    bufp = 0;

    do {
        i = read(nh->sock, buffer+bufp, blen-bufp);
        if(i == -1) {
            fprintf(stderr, "noose: %s\n", strerror(errno));
            return NULL;
        }
        
        bufp += i;

        if(bufp >= blen) {
            blen *= 2;
            buffer = realloc(buffer, blen);
            if(buffer == NULL) {
                fprintf(stderr, "noose: Out of memory in "
                        "nntp_readresponse.\n");
                return 0;
            }
        }
    } while(buffer[bufp-1] != '\n');
    buffer[bufp] = 0;

    respnum = atoi(buffer);
    if(resbody != NULL)
        *resbody = buffer;
    else
        free(buffer);

    return respnum;
}

void nntp_disconnect(void *nh_)
{
    nntp_t *nh = (nntp_t *)nh_;

    if(nh->magic != OURMAGIC) {
        fprintf(stderr, "noose: magic check failed for nntp_disconnect!\n");
    }

    write(nh->sock, cmd_quit, strlen(cmd_quit));
    close(nh->sock);

    return;
}

int nntp_cmd_group(void *nh_, char *group, int *narticles, int *firstart,
                   int *lastart)
{
    nntp_t *nh = (nntp_t *)nh_;
    int i;
    char *respbody, *p, *cmdbuf;

    cmdbuf = malloc(strlen("GROUP ")+strlen(group)+strlen("\r\n"));
    strcpy(cmdbuf, "GROUP ");
    strcat(cmdbuf, group);
    strcat(cmdbuf, "\r\n");
    write(nh->sock, cmdbuf, strlen(cmdbuf));
    free(cmdbuf);

    i = nntp_readresponse(nh, &respbody);
    if(i == 411) {
        fprintf(stderr, "noose: No such group `%s'.\n", group);
        return -1;
    } else if(i != 211) {
        fprintf(stderr, "noose: Got bad response in nntp_cmd_group.\n");
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

int nntp_readheader(nntp_t *nh, char **header)
{
    return -1;
}

int nntp_readbody(nntp_t *nh, char **body)
{
    return -1;
}

#define MAXARTNUMLEN 10

int nntp_cmd_article(void *nh_, int artnum, char **head, char **body)
{
    nntp_t *nh = (nntp_t *)nh_;
    int i;
    char *respbody, *cmdbuf, *cmd;

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
        return -1;
    }
    if(head != NULL)
        nntp_readheader(nh, head);
    if(body != NULL)
        nntp_readbody(nh, body);

    return 0;
}

/* EOF nntp.c */
