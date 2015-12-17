/* This is the file for adding extra primitives to the FRUGAL environment.
   See the section "Adding Primitives" in the file docs/USING

   This program is Copyright 2002 by HardCore Software, and it's
   distribution is protected by the GNU General Public License.
   See the file COPYING for details.
*/


#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <arpa/inet.h>
#include <string.h>
#include <fcntl.h>


#define NEWPRIM(cname)  void cname(struct forth_instance *fi)
#define REGPRIM(cname, fname)  reg_prim(fi, fname, &cname)
#define push(x)  st_push(fi, (int) x)
#define pop()  st_pop(fi)


NEWPRIM(read_func) {
  int fd, len;
  char *dest;

  len = pop();
  dest = (char *) pop();
  fd = pop();

  push(read(fd, dest, len));
}

NEWPRIM(write_func) {
  int fd, len;
  char *src;

  len = pop();
  src = (char *) pop();
  fd = pop();

  push(write(fd, src, len));
}

NEWPRIM(close_func) {
  close(pop());
}

NEWPRIM(poll_func) {
  /* FIXME: Some sort of API for polling multiple fds... */
  /* Hell, make the poll API actually useful. :) */

  fd_set rfds;
  struct timeval tv;
  int sd, timeout;

  timeout = pop();
  sd = pop();

  tv.tv_sec=0;
  tv.tv_usec=timeout;

  FD_ZERO(&rfds);
  FD_SET(sd, &rfds);

  select(FD_SETSIZE, &rfds, NULL, NULL, (timeout==-1) ? NULL : &tv);

  push(FD_ISSET(sd, &rfds));

}

NEWPRIM(term_raw_on) {
  struct termios tptty;

  ioctl(0, IOCTL_GET, &tptty);
  tptty.c_lflag &= ~(ICANON | ECHO);
  if (ioctl(0, IOCTL_SET, &tptty) == -1) {
    if(fi->output) fprintf(fi->output, "IOCTL FAILURE\n");
    error(fi);
  }
}

NEWPRIM(term_raw_off) {
  struct termios tptty;

  ioctl(0, IOCTL_GET, &tptty);
  tptty.c_lflag |= (ICANON | ECHO);
  if (ioctl(0, IOCTL_SET, &tptty) == -1) {
    if(fi->output) fprintf(fi->output, "IOCTL FAILURE\n");
    error(fi);
  }
}

NEWPRIM(flush) {
  fflush(stdin);
  fflush(stdout);
}


NEWPRIM(microsleep) {
  int tp;
  tp = pop();
  if (tp>=999999) for(;tp>=999999;tp-=999999) usleep(999999);
  usleep(tp);
}


NEWPRIM(time_fetch) {
  push(time(NULL));
}


NEWPRIM(gettime) {
  struct timeval tp;
  gettimeofday(&tp, NULL);
  push(tp.tv_usec);
  push(tp.tv_sec);
}

NEWPRIM(fork_func) {
  push(fork());
}

NEWPRIM(open_func) {
  int i=0, fd, mode;
  char *name, back;

  name = (char *) pop();
  mode = pop();

  while(name[i] && !iswhitespace(name[i])) i++;
  back = name[i];
  name[i] = '\0';

  if (mode==0) mode = O_RDONLY;
  else if (mode==1) mode = O_WRONLY;
  else if (mode==2) mode = O_RDWR;

  fd = open(name, 0, mode);
  if (fd == -1) push(0);
  else push(fd);

  name[i] = back;

}

NEWPRIM(chdir_func) {

  char back, *name;
  int i=0;

  name = (char *) pop();

  while(name[i] && !iswhitespace(name[i])) i++;
  back = name[i];
  name[i] = '\0';

  push(!chdir(name));

  name[i] = back;

}


NEWPRIM(dnstoip) {
/* ( dest src -- found? ) */

  struct hostent *h;
  struct in_addr s;
  char back, *name, *dest;
  int i=0;

  name = (char *)pop();
  dest = (char *)pop();

  memset(dest, '\0', 16);
  while(name[i] && !iswhitespace(name[i])) i++;
  back = name[i];
  name[i] = '\0';

  h = gethostbyname(name);
  name[i] = back;

  if(h == NULL) {
    push(0);
    return;
  }

  memcpy(&(s.s_addr), h->h_addr_list[0], h->h_length);
  snprintf(dest, 16, "%s", inet_ntoa(s));
  push(1);
  return;

}

NEWPRIM(iptodns) {
/* ( dest len src -- found? ) */

  struct hostent *h;
  struct in_addr a;
  int i=0,n[4], len;
  char back,*src, *dest;

  src = (char *) pop();
  len = pop();
  dest = (char *) pop();

  memset(dest, '\0', len);
  while(src[i] && !iswhitespace(src[i])) i++;
  back = src[i];
  src[i] = '\0';

  if (sscanf(src, "%d.%d.%d.%d", &n[0], &n[1], &n[2], &n[3]) != 4) {
    src[i] = back;
    push(0);
    return;
  }

  for (i=0; i<4; i++) {
    if (n[i] < 0 || n[i] > 255) {
      src[i] = back;
      push(0);
      return;
    }
    ((char *)&a.s_addr)[i] = (char)n[i];
  }

  if((h = gethostbyaddr((const char *)&a, sizeof(a), AF_INET)) == NULL) {
    src[i] = back;
    push(0);
    return;
  }

  strncpy(dest, h->h_name, len);
  src[i] = back;
  push(1);

}

NEWPRIM(openconn) {
/* ( port toconn -- fd/NULL ) */

  struct hostent *he;
  struct sockaddr_in their_addr; /* connector's address information */
  int sd,i=0,port;
  char *toconn,back;

  toconn = (char *) pop();
  port = pop();

  while(toconn[i] && !iswhitespace(toconn[i])) i++;
  back = toconn[i];
  toconn[i] = '\0';

  if ((he=gethostbyname(toconn)) == NULL) {  /* get the host info */
    push(0);
    toconn[i] = back;
    return;
  }

  toconn[i] = back;

  if ((sd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
    push(0);
    return;
  }

  their_addr.sin_family = AF_INET;         /* host byte order */
  their_addr.sin_port = htons(port);     /* short, network byte order */
  their_addr.sin_addr = *((struct in_addr *)he->h_addr);
  memset(&(their_addr.sin_zero), '\0', 8); /* zero the rest of the struct */

  if (connect(sd, (struct sockaddr *)&their_addr, sizeof(struct sockaddr)) == -1) {
    push(0);
    return;
  }

  push(sd);
  return;

}

#define BACKLOG 15

NEWPRIM(openlistener) {
/* ( port -- socket ) */

  int tp,sd,port;
  struct sockaddr_in my_addr;

  port = pop();

  if ((sd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
    push(0);
    return;
  }

  /*
   * This excellent bugfix added by Fabrice Marie to Anti-Web.
   */
  setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, &tp, sizeof(tp));
    
  my_addr.sin_family = AF_INET;         /* host byte order */
  my_addr.sin_port = htons(port);    /* short, network byte order */
  my_addr.sin_addr.s_addr = INADDR_ANY; /* automatically fill with my IP */

  memset(&(my_addr.sin_zero), 0, 8);        /* zero the rest of the struct */

  if (bind(sd, (struct sockaddr *)&my_addr, sizeof(struct sockaddr)) == -1) {
    push(0);
    return;
  }

  if (listen(sd, BACKLOG) == -1) {
    push(0);
    return;
  }

  push(sd);

}

NEWPRIM(acceptconn) {
/* ( listen_sock -- connection_sock ) */

  struct sockaddr_in their_addr;
  socklen_t tp = sizeof(struct sockaddr_in);
  int listenfd,connfd;

  listenfd = pop();

  connfd = accept(listenfd, (struct sockaddr *)&their_addr, &tp);

  if (connfd == -1) {
    push(0);
    return;
  }

  push(connfd);

}



void external_prims(struct forth_instance *fi) {

  REGPRIM(term_raw_on, "term-raw-on");
  REGPRIM(term_raw_off, "term-raw-off");
  REGPRIM(flush, "flush");
  REGPRIM(microsleep, "usleep");
  REGPRIM(time_fetch, "time@");
  REGPRIM(gettime, "gettime");
  REGPRIM(fork_func, "fork");
  REGPRIM(open_func, "open");
  REGPRIM(chdir_func, "chdir");

  REGPRIM(read_func, "read");
  REGPRIM(write_func, "write");
  REGPRIM(close_func, "close");
  REGPRIM(poll_func, "poll");
  REGPRIM(dnstoip, "dnstoip");
  REGPRIM(iptodns, "iptodns");
  REGPRIM(openconn, "openconn");
  REGPRIM(openlistener, "openlistener");
  REGPRIM(acceptconn, "acceptconn");

}

