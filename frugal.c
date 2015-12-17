/* This is the core engine for the FRUGAL environment.

   This program is Copyright 2002 by HardCore Software, and it's
   distribution is protected by the GNU General Public License.
   See the file COPYING for details.
*/

#include <stdio.h>
#include <sys/time.h>
#include <time.h>
#include <ctype.h>
#include <stdlib.h>
#include <stdarg.h>
#include <setjmp.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <poll.h>
#include <signal.h>


#define INIT "init.fs"
#define VERSION 101
 

#ifdef TCSETS
#define IOCTL_GET TCGETS
#define IOCTL_SET TCSETS
#else
#define IOCTL_GET TIOCGETA
#define IOCTL_SET TIOCSETAF
#endif




/* Word states */
#define IMMEDIATE 1
#define COMPILE_ONLY 2

/* Execution states */
#define STATE_INTERPRETING 0
#define STATE_COMPILING 1

/* Header info */
#define HEADER_LEN 10

/* Virtual Machine Instructions */
#define VM_PRIMITIVE 1
#define VM_NUMBER 2 
#define VM_BRANCH 3
#define VM_IFBRANCH 4

jmp_buf hop_back;

struct forth_instance {
  int *st; /* Data stack base */
  int *stp; /* Data stack pointer */

  int *rst; /* Return stack base */
  int *rstp; /* Return stack pointer */

  FILE **fst; /* File input stack base */
  FILE **fstp; /* File input stack pointer */

  int *heap; /* Dict/heap base */
  int *lastdictp; /* Pointer to last dict entry */
  int *heapp; /* Dict/heap pointer */

  char *parse; /* Parse buffer base */
  char *parsep; /* Parse buffer pointer */
  int parsesize; /* Size of parse buffer */

  char *pad; /* Temporary string buffer */

  FILE *input;
  FILE *output;

  int *ip; /* Interpretive Pointer (current execution location) */
  int base; /* Numeric base */
  int state; /* Compiling or interpreting */
};


struct forth_instance *ci;

/* INTERNAL FUNCTIONS */

void error(struct forth_instance *fi) {
  if (fi->output) fprintf(fi->output, "RESETING STACKS AND STATE\n");
  longjmp(hop_back, 0);
}

int iswhitespace(char tp) {
  if (tp == ' ' || tp == '\n' || tp == '\t') return 1;
  return 0;
}

int dictentry_cmp(int *ent, char *name, int namelen) {
  int len,i;
  len = (int) *((char *) ent);
  if (namelen != len) return 0;
  for (i=0; i<len; i++) if (name[i] != ((char *)ent)[i+1]) return 0;
  return 1;
}


/* resp is whether or not the function should respond to errors */ 
int *search_dict(struct forth_instance *fi, char *name, int len, int resp) {

  int *tpdictp=fi->lastdictp;

  while (tpdictp != NULL && (dictentry_cmp(tpdictp+2, name, len)==0)) tpdictp = (int *) *tpdictp;

  /* NOTE: name[len] is overwritten. Keep this in mind if you change it. */
  if (tpdictp == NULL) {
    if (resp==1) {
      name[len] = '\0';
      if (fi->output) 
        fprintf(fi->output, "UNABLE TO FIND WORD \"%s\".\n", name);
      error(fi);
    } else {
      return (int) NULL;
    }
  }

  return tpdictp+HEADER_LEN;

}

void create_header(struct forth_instance *fi, int len, char *buf) {

  (fi->heapp)[0] = (int) fi->lastdictp;
  (fi->heapp)[1] = 0;
  fi->lastdictp = fi->heapp;
  (fi->heapp)+=2;

  ((char *)(fi->heapp))[0] = (char) len;
  memcpy(((char *)fi->heapp)+1, buf, len > 31 ? 31 : len);
  (fi->heapp)+=8;

}

void st_push(struct forth_instance *fi, int tp) {
  /* FIXME: Preform overflow checking */
  (fi->stp)[0] = tp;
  (fi->stp)++;
}

int st_pop(struct forth_instance *fi) {
  if (fi->st == fi->stp) {
    if (fi->output) fprintf(fi->output, "STACK UNDERFLOW!\n");
    error(fi);
  }
  (fi->stp)--;
  return *(fi->stp);
}

void fst_push(struct forth_instance *fi, FILE *tp) {
  /* FIXME: Preform overflow checking */
  (fi->fstp)[0] = tp;
  (fi->fstp)++;
}

FILE *fst_pop(struct forth_instance *fi) {
  if (fi->fst == fi->fstp) {
    if (fi->output) fprintf(fi->output, "RSTACK UNDERFLOW!\n");
    error(fi);
  }
  (fi->fstp)--;
  return *(fi->fstp);
}

void rst_push(struct forth_instance *fi, int tp) {
  /* FIXME: Preform overflow checking */
  (fi->rstp)[0] = tp;
  (fi->rstp)++;
}

int rst_pop(struct forth_instance *fi) {
  if (fi->rst == fi->rstp) {
    if (fi->output) fprintf(fi->output, "RSTACK UNDERFLOW!\n");
    error(fi);
  }
  (fi->rstp)--;
  return *(fi->rstp);
}

void exit_prim(struct forth_instance *fi) {
  fi->ip = (int *) rst_pop(fi);
}

void start_exec(int *start_ip) {

  void (*tp)(struct forth_instance *);

  ci->ip = start_ip;

  while(ci->ip != NULL) {

    if (*(ci->ip) == VM_PRIMITIVE) {
    /* FIXME : Don't need tp */
      tp = (void (*)(struct forth_instance *)) (*(ci->ip + 1));
      ci->ip += 2;
      (*tp)(ci);
    } else if (*(ci->ip) == VM_BRANCH) {
      ci->ip = (int *) *(ci->ip + 1);
    } else if (*(ci->ip) == VM_IFBRANCH) {
      if (st_pop(ci)) {
        ci->ip = (int *) *(ci->ip + 1);
      } else {
        ci->ip += 2;
      }
    } else if (*(ci->ip) == VM_NUMBER) {
      st_push(ci, *(ci->ip + 1));
      ci->ip += 2;
    } else {
      rst_push(ci, (int) (ci->ip + 1));
      ci->ip = (int *) *(ci->ip);
    }

  }

}


/* EXTERNAL FUNCTIONS */


void term_raw_off(struct forth_instance *); /* Hack */

void die(struct forth_instance *fi) {
  term_raw_off(fi);
  exit(0);
}

void sig_die() {
  die(ci);
}

void ver(struct forth_instance *fi) {
  st_push(fi, VERSION);
}

void l(struct forth_instance *fi) {
  st_push(fi, (int) &(fi->lastdictp));
}

void base(struct forth_instance *fi) {
  st_push(fi, (int) &(fi->base));
}

void c_fetch(struct forth_instance *fi) {
  st_push(fi, (int) *((char *) st_pop(fi)));
}

void c_store(struct forth_instance *fi) {
  int tp = st_pop(fi);
  ((char *) tp)[0] = (char) st_pop(fi);
}

void c_comma(struct forth_instance *fi) {
  ((char *) fi->heapp)[0] = (char) st_pop(fi);
  ((char *) fi->heapp)++;
}

void align(struct forth_instance *fi) {
  while (((int)(fi->heapp) % 4) != 0) ((char *)(fi->heapp))++;
}

void sp(struct forth_instance *fi) {
  st_push(fi, (int) fi->stp);
}

void rp(struct forth_instance *fi) {
  st_push(fi, (int) (fi->rstp - 1));
}

void s0(struct forth_instance *fi) {
  st_push(fi, (int) fi->st);
}

void r0(struct forth_instance *fi) {
  st_push(fi, (int) fi->rst);
}

void emit(struct forth_instance *fi) {
  putc(st_pop(fi), stdout);
}

/* This is a faster DECIMAL ONLY version of printnum */
void qpnum(struct forth_instance *fi) {
  printf("%d", st_pop(fi));
}

void to_r(struct forth_instance *fi) {
  int tp = rst_pop(fi);
  rst_push(fi, st_pop(fi));
  rst_push(fi, tp);
}

void r_to(struct forth_instance *fi) {
  int tp = rst_pop(fi);
  st_push(fi, rst_pop(fi));
  rst_push(fi, tp);
}

void get_state(struct forth_instance *fi) {
  st_push(fi, (int) &(fi->state));
}

void store(struct forth_instance *fi) {
  int *tp;
  tp = (int *) st_pop(fi);
  tp[0] = st_pop(fi);
}

void fetch(struct forth_instance *fi) {
  st_push(fi, *((int *) st_pop(fi)));
}

void drop(struct forth_instance *fi) {
  st_pop(fi);
}

void lshift(struct forth_instance *fi) {
  int tp;
  tp = st_pop(fi);
  st_push(fi, st_pop(fi) << tp);
}

void ulshift(struct forth_instance *fi) {
  unsigned int tp;
  tp = (unsigned int) st_pop(fi);
  st_push(fi, (unsigned int)st_pop(fi) << tp);
}

void rshift(struct forth_instance *fi) {
  int tp;
  tp = st_pop(fi);
  st_push(fi, st_pop(fi) >> tp);
}

void urshift(struct forth_instance *fi) {
  unsigned int tp;
  tp = (unsigned int) st_pop(fi);
  st_push(fi, (unsigned int)st_pop(fi) >> tp);
}

void and(struct forth_instance *fi) {
  st_push(fi, st_pop(fi) & st_pop(fi));
}

void or(struct forth_instance *fi) {
  st_push(fi, st_pop(fi) | st_pop(fi));
}

void xor(struct forth_instance *fi) {
  st_push(fi, st_pop(fi) ^ st_pop(fi));
}

void pad(struct forth_instance *fi) {
  st_push(fi, (int) fi->pad);
}

void plus(struct forth_instance *fi) {
  st_push(fi, st_pop(fi) + st_pop(fi));
}

void minus(struct forth_instance *fi) {
  int tp = st_pop(fi);
  st_push(fi, st_pop(fi) - tp);
}

void mult(struct forth_instance *fi) {
  st_push(fi, st_pop(fi) * st_pop(fi));
}

void divide(struct forth_instance *fi) {
  int tp;
  tp = st_pop(fi);
  st_push(fi, st_pop(fi) / tp);
}

void mod(struct forth_instance *fi) {
  int tp;
  tp = st_pop(fi);
  st_push(fi, st_pop(fi) % tp);
}

void pick(struct forth_instance *fi) {
  int tp;
  tp = st_pop(fi);
  if (tp >= (fi->stp - fi->st)) {
    if (fi->output) fprintf(fi->output, "STACK UNDERFLOW!\n");
    error(fi);
  }
  st_push(fi, *(fi->stp - tp - 1));
}

void roll(struct forth_instance *fi) {
  int top, num, i;
  num = st_pop(fi);
  if (num < 1) return;
  if (num > (fi->stp - fi->st - 1)) {
    if (fi->output) fprintf(fi->output, "STACK UNDERFLOW!\n");
    error(fi);
  }
  top = *(fi->stp - 1);
  for(i=0; i<=num; i++) (fi->stp - i - 1)[0] = (fi->stp - i - 2)[0];
  (fi->stp - num - 1)[0] = top;
}

void immediate(struct forth_instance *fi) {
  *(fi->lastdictp + 1) |= IMMEDIATE;
}

void compile_only(struct forth_instance *fi) { 
  *(fi->lastdictp + 1) |= COMPILE_ONLY;
}

void h(struct forth_instance *fi) { 
  st_push(fi, (int) &(fi->heapp));
}

void to_in(struct forth_instance *fi) { 
  st_push(fi, strlen(fi->parsep));
}

void source(struct forth_instance *fi) {
  st_push(fi, (int) (fi->parse));
  st_push(fi, strlen(fi->parse));
}

void lessthan(struct forth_instance *fi) { 
  int tp;
  tp = st_pop(fi);
  st_push(fi, st_pop(fi) < tp);
}

void greaterthan(struct forth_instance *fi) { 
  int tp;
  tp = st_pop(fi);
  st_push(fi, st_pop(fi) > tp);
}

void equal(struct forth_instance *fi) { 
  st_push(fi, st_pop(fi) == st_pop(fi));
}

void reset(struct forth_instance *fi) {
  fi->ip = (int *) rst_pop(fi);
  fi->stp = fi->st;
  fi->rstp = fi->rst;
  fi->state = STATE_INTERPRETING;
}

void comma(struct forth_instance *fi) {
  *(fi->heapp) = st_pop(fi);
  (fi->heapp)++;
}

void exit_word(struct forth_instance *fi) {
  st_push(fi, (int) &exit_prim);
  st_push(fi, VM_PRIMITIVE);
  comma(fi);
  comma(fi);
}

void parse(struct forth_instance *fi) {
  int len=0;
  char delim = (char) st_pop(fi);

  if (delim == ' ') {
    while (iswhitespace(*(fi->parsep))) (fi->parsep)++;
    st_push(fi, (int) (fi->parsep));
    while (!iswhitespace(*(fi->parsep)) && *(fi->parsep)) { (fi->parsep)++; len++; }
  } else if (delim == '\n') {
    st_push(fi, (int) (fi->parsep));
    len = strlen(fi->parsep);
    fi->parsep += len;
  } else {
    (fi->parsep)++;
    st_push(fi, (int) (fi->parsep));
    while (*(fi->parsep) != delim && *(fi->parsep)) { (fi->parsep)++; len++; }

    if (*(fi->parsep) == '\0') {
      if (fi->output) fprintf(fi->output, "PARSING ERROR\n");
      error(fi);
    }

    (fi->parsep)++;
  }

  st_push(fi, len);

}


void include_quote(struct forth_instance *fi) {

  char backup;
  int len;
  char *name;

  st_push(fi, '"');
  parse(fi);
  len = st_pop(fi);
  name = (char *) st_pop(fi);

  backup = name[len];
  name[len] = '\0';

  if (access(name, R_OK) == -1) {
    if(fi->output) fprintf(fi->output, "UNABLE TO OPEN '%s'\n", name);
    error(fi);
  } else {
    fst_push(fi, fi->input);
    fi->input = fopen(name, "r");
  }

  name[len] = backup;

}


void create_addr(struct forth_instance *fi) {
  int len;

  len = st_pop(fi);
  create_header(fi, len, (char *) st_pop(fi));
}


void create(struct forth_instance *fi) {
  st_push(fi, (int) ' ');
  parse(fi);
  create_addr(fi);
}


void colon(struct forth_instance *fi) {

  if (fi->state == STATE_COMPILING) {
    if(fi->output) fprintf(fi->output, "CAN'T NEST COLON DEFINITIONS\n");
    error(fi);
  }
  fi->state = STATE_COMPILING;
  create(fi);

}


void semicolon(struct forth_instance *fi) {

  if (fi->state != STATE_COMPILING) {
    if(fi->output) fprintf(fi->output, "SEMICOLON ENCOUNTERED WHEN NOT COMPILING\n");
    error(fi);
  }
  exit_word(fi);
  fi->state = STATE_INTERPRETING;

}


void tick_addr(struct forth_instance *fi) {

  int len;
  char *name;

  len = st_pop(fi);
  name = (char *) st_pop(fi);

  st_push(fi, (int) search_dict(fi, name, len, 1));
  
}


void tick(struct forth_instance *fi) {

  st_push(fi, (int) ' ');
  parse(fi);

  tick_addr(fi);

}

int my_isdigit(char tp, struct forth_instance *fi) {
  if (tp>='0' && tp<=('0'+fi->base-1)) return tp-'0';
  if (fi->base > 10 && tp>='a' && tp<='a'+fi->base-11) return tp-'a'+10;
  return -1;
}

int my_atoi(char *tp, struct forth_instance *fi) {
  int i,count=0,sign=1;
  if (tp[0] == '-') { sign=-1; tp++; }
  for(i=0; !iswhitespace(tp[i]); i++) {
    count*=fi->base;
    count+=my_isdigit(tp[i], fi);
  }
  return count*sign;
}

void number(struct forth_instance *fi) {
  int len,i=0;
  char *name;

  len = st_pop(fi);
  name = (char *) st_pop(fi);

  if (name[0] == '-' && my_isdigit((int)name[1], fi)!=-1) i = 2;
  while(!iswhitespace(name[i]) && name[i] != '\0') {
    if (my_isdigit((int)name[i], fi) == -1) {
      st_push(fi, 0);
      return;
    }
    i++;
  }

  st_push(fi, my_atoi(name, fi));
  st_push(fi, 1);

}

void query(struct forth_instance *fi) {

  if (fi->input == stdin && fi->output && fi->state == STATE_INTERPRETING)
    fprintf(fi->output, "  ok\n");

  if (fgets(fi->parse, fi->parsesize-32, fi->input) == NULL) {
    if (fi->fstp == fi->fst) die(fi);
    fclose(fi->input);
    fi->input = fst_pop(fi);
    fi->parse[0] = '\0';
  }
  fi->parsep = fi->parse;

}


void interpret(struct forth_instance *fi) {

  char *name;
  int len,i=0;
  int *addr;

  while(iswhitespace(*(fi->parsep))) fi->parsep++;
  if (*(fi->parsep) == 0) return;

  st_push(fi, ' ');
  parse(fi);

  len = st_pop(fi);
  name = (char *) st_pop(fi);

  if ((addr = search_dict(fi, name, len, 0)) == NULL) {
    st_push(fi, (int) name);
    st_push(fi, len);
    number(fi);
    if (st_pop(fi)) {
      /* This means we already have the number on the stack */
      if (fi->state == STATE_COMPILING) {
        st_push(fi, VM_NUMBER);
        comma(fi);
        comma(fi);
      }
    return;
    }
    name[len] = '\0';
    if (fi->output) 
      fprintf(fi->output, "UNABLE TO FIND WORD \"%s\".\n", name);
    error(fi);
  } else {
    st_push(fi, (int) addr);
    st_push(fi, (int) addr);
    if (fi->state == STATE_INTERPRETING) {
      if (*((int *)st_pop(fi) - 9) & COMPILE_ONLY) {
        while(!iswhitespace(name[i])) i++;
          name[i] = '\0';
        if (fi->output) fprintf(fi->output, "YOU TRIED TO EXECUTE A COMPILE-ONLY WORD: \"%s\".\n", name);
        error(fi);
      }
      fi->ip = (int *) st_pop(fi);
    } else if (fi->state == STATE_COMPILING) {
      if (*((int *)st_pop(fi) - 9) & IMMEDIATE) {
        fi->ip = (int *) st_pop(fi);
      } else {
        comma(fi);
      }
    }
  }

}





/* STARTUP STUFF */

void reg_prim(struct forth_instance *fi, char *name, void (*func)(struct forth_instance *)) {

  fi->state = STATE_COMPILING;

  create_header(fi, strlen(name), name);
  st_push(fi, VM_PRIMITIVE);
  comma(fi);
  st_push(fi, (int) func);
  comma(fi);

  semicolon(fi);

}


#include "extprims.c"


void init_forth(struct forth_instance *fi) {

  fi->rst = fi->rstp = (int *) malloc(sizeof(int) * 1000);
  fi->parse = fi->parsep = (char *) malloc(4000);
  fi->parsesize = 4000;
  fi->st = fi->stp = (int *) malloc(sizeof(int) * 1000);
  fi->fst = fi->fstp = (FILE **) malloc(sizeof(int) * 100);
  fi->heap = fi->heapp = (int *) malloc(sizeof(int) * 10000);
  fi->lastdictp = NULL;
  /* Make sure the numbers on the following 2 lines are the same */
  fi->pad = (char *) malloc(1000);

  fi->input = stdin;
  fi->output = stdout;

  fi->ip = NULL;
  fi->base = 10;
  fi->state = STATE_INTERPRETING;

  align(fi);
  
    reg_prim(fi, "compile-exit", &exit_word);
    reg_prim(fi, "number", &number);
    reg_prim(fi, "interpret", &interpret);
    reg_prim(fi, "query", &query);
    reg_prim(fi, "h", &h);
    reg_prim(fi, "immediate", &immediate);
    reg_prim(fi, "compile-only", &compile_only);
    reg_prim(fi, "reset", &reset);
    reg_prim(fi, ",", &comma);
    reg_prim(fi, "align", &align);
    reg_prim(fi, "parse", &parse);
    reg_prim(fi, "create-addr", &create_addr);
    reg_prim(fi, "create", &create);
    reg_prim(fi, ":", &colon);
    reg_prim(fi, ";", &semicolon); immediate(fi); compile_only(fi);
    reg_prim(fi, "'-addr", &tick_addr);
    reg_prim(fi, "'", &tick);
    reg_prim(fi, ">in", &to_in);
    reg_prim(fi, "source", &source);
    reg_prim(fi, "<", &lessthan);
    reg_prim(fi, ">", &greaterthan);
    reg_prim(fi, "=", &equal);
    reg_prim(fi, "drop", &drop);
    reg_prim(fi, "pick", &pick);
    reg_prim(fi, "roll", &roll);
    reg_prim(fi, "!", &store);
    reg_prim(fi, "@", &fetch);
    reg_prim(fi, "+", &plus);
    reg_prim(fi, "-", &minus);
    reg_prim(fi, "*", &mult);
    reg_prim(fi, "/", &divide);
    reg_prim(fi, "mod", &mod);
    reg_prim(fi, ">r", &to_r);
    reg_prim(fi, "r>", &r_to);
    reg_prim(fi, "state", &get_state);
    reg_prim(fi, "emit", &emit);
    reg_prim(fi, "qpnum", &qpnum);
    reg_prim(fi, "s0", &s0);
    reg_prim(fi, "r0", &r0);
    reg_prim(fi, "sp", &sp);
    reg_prim(fi, "rp", &rp);
    reg_prim(fi, "c@", &c_fetch);
    reg_prim(fi, "c!", &c_store);
    reg_prim(fi, "c,", &c_comma);
    reg_prim(fi, "lshift", &lshift);
    reg_prim(fi, "rshift", &rshift);
    reg_prim(fi, "ulshift", &ulshift);
    reg_prim(fi, "urshift", &urshift);
    reg_prim(fi, "and", &and);
    reg_prim(fi, "or", &or);
    reg_prim(fi, "xor", &xor);
    reg_prim(fi, "pad", &pad);
    reg_prim(fi, "die", &die);
    reg_prim(fi, "base", &base);
    reg_prim(fi, "l", &l);
    reg_prim(fi, "include\"", &include_quote);
    reg_prim(fi, "ver", &ver);


    external_prims(fi);

    /* Make the "quit" word. Would do it in forth, but the begin loop
       is defined there... */
    /* Essentially, it looks like this:
             : quit reset
	            begin
		      query
		      begin
		        interpret >in 0 >
		      while
		    again ;
    */
    create_header(fi, 4, "quit");
    st_push(fi, (int) search_dict(fi, "reset", 5, 1)); comma(fi);
    st_push(fi, (int) fi->heapp);
    st_push(fi, (int) search_dict(fi, "query", 5, 1)); comma(fi);
    st_push(fi, (int) fi->heapp);
    st_push(fi, (int) search_dict(fi, "interpret", 9, 1)); comma(fi);
    st_push(fi, (int) search_dict(fi, ">in", 3, 1)); comma(fi);
    st_push(fi, VM_NUMBER); comma(fi);
    st_push(fi, 0); comma(fi);
    st_push(fi, (int) search_dict(fi, ">", 1, 1)); comma(fi);
    st_push(fi, VM_IFBRANCH); comma(fi);
    comma(fi);
    st_push(fi, VM_BRANCH); comma(fi);
    comma(fi);
    st_push(fi, VM_PRIMITIVE); comma(fi); /* So we can "see" the word */
    st_push(fi, (int) &exit_word); comma(fi);

}



void openfilestream(struct forth_instance *fi, char *fname) {
  if (access(fname, R_OK) == -1) {
    fprintf(stdout, "Warning: Unable to open %s!\n", fname);
  } else {
    fst_push(fi, fi->input);
    fi->input = fopen(fname, "r");
  }
}


#ifdef INTERNAL_CODE
void procinternal_code(struct forth_instance *fi) {
  int i, curr=0, depth=0, pfds[2];

  pipe(pfds);
  fcntl(pfds[1], F_SETFL, O_NONBLOCK);

  while(internal_code[curr] != NULL) {
    if (write(pfds[1], internal_code[curr], strlen(internal_code[curr]))==-1) {
      depth++;
      close(pfds[1]);
      st_push(fi, (int) fdopen(pfds[0], "r"));
      pipe(pfds);
      fcntl(pfds[1], F_SETFL, O_NONBLOCK);
    }
    curr++;
  }
  st_push(fi, (int) fdopen(pfds[0], "r"));
  close(pfds[1]);

  fst_push(fi, fi->input);
  for(i=0;i<=depth;i++) {
    fst_push(fi, (FILE *) st_pop(fi));
  }
  fi->input = (FILE *) fst_pop(fi);
  
}
#endif


int main (int argc, char *argv[]) {

  int i;
  int desc[2];
  struct forth_instance root;

  signal(SIGINT, sig_die);
  signal(SIGQUIT, sig_die);
  signal(SIGTERM, sig_die);

  init_forth(&root); 
  ci = &root;

  for(i=argc-1; i>0; i--) {
    if (*(argv[i]) == '-') {
      pipe(desc);
      write(desc[1], argv[i]+1, strlen(argv[i]+1));
      close(desc[1]);
      fst_push(ci, ci->input);
      ci->input = fdopen(desc[0], "r");
    } else {
      openfilestream(ci, argv[i]);
    }
  }

  #ifdef INTERNAL_CODE
    procinternal_code(ci);
  #endif
  #ifndef INTERNAL_CODE
    openfilestream(ci, INIT);
  #endif

  #ifndef QUIET
    printf("HardCore SoftWare's Forth System: Frugal   V%d.%d.%d\n\n",
            VERSION/100, (VERSION/10)%10, VERSION%10);
  #endif

  setjmp(hop_back);

  start_exec(search_dict(&root, "quit", 4, 1));

  return 0;

}
