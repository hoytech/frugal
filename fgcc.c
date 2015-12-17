/* This is the Frugal GCC Wrapper Program. See docs/FGCC for details.

   This program is Copyright 2002 by HardCore Software, and it's
   distribution is protected by the GNU General Public License.
   See the file COPYING for details.
*/


#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>


FILE *infd=NULL;
char *outfile=NULL, *cgen=NULL, *cc=NULL;
int bare=0, quiet=0, norm=0;


void forth_print(FILE *outfd, char *buf) {

  char *obuf;

  obuf = buf;
  buf[strlen(buf)-1] = '\0';

  /* This is a cheap hack. :) */
  if (strncmp(obuf, ": \\ ", 4) == 0) {
    fprintf(outfd, "\": \\\\ 10 parse drop drop ; immediate\\n\",\n");
    return;
  }

  while (*buf != '\0') {
    if (strncmp(buf, " \\ ", 3)==0) { *buf='\0'; break; }
    if (*buf == '"') { memmove(buf+1, buf, strlen(buf)+1); *buf = '\\'; buf++; }
    buf++;
  }

  if (*obuf == '\0' || *obuf == '\\') return;

  buf[0] = '\\'; buf[1] = 'n'; buf[2] = '\0';
  fprintf(outfd, "\"%s\",\n", obuf);

}


void q_printfile(FILE *outfd, char *fname) {
  FILE *infd;
  char ibuf[2048];

  infd = fopen(fname, "r");
  if (infd == NULL) {
    fprintf(stderr, "ERROR: Couldn't open \"%s\"!\n", fname);
    exit(-1);
  }

  while(fgets(ibuf, sizeof(ibuf), infd) != NULL) forth_print(outfd, ibuf);

  fclose(infd);

}


void n_printfile(FILE *outfd, char *fname) {
  FILE *infd;
  char *tp, ibuf[2048];

  infd = fopen(fname, "r");
  if (infd == NULL) {
    fprintf(stderr, "ERROR: Couldn't open \"%s\"!\n", fname);
    exit(-1);
  }

  while(fgets(ibuf, sizeof(ibuf), infd) != NULL) {
    tp = strstr(ibuf, "#include \"");
    if (tp==NULL) { fprintf(outfd, "%s", ibuf); continue; }
    tp+=10;
    if (strstr(tp, "\"") == NULL) { fprintf(outfd, "%s", ibuf); continue; }
    strstr(tp, "\"")[0] = '\0';
    n_printfile(outfd, tp);
  }

  fclose(infd);

}


void do_compile(char *tpfile) {
  char tpcommand[200];

  if (cgen) {
    printf("Generating C source file: \"%s\".\n", cgen);
    rename(tpfile, cgen);
  } else {
    if (outfile) sprintf(tpcommand, "%s -o %s -s -O2 %s", cc ? cc : "gcc", outfile, tpfile);
    else sprintf(tpcommand, "%s -o a.out -s -O2 %s", cc ? cc : "gcc", tpfile);

    printf("%s\n", tpcommand);
    system(tpcommand);

    remove(tpfile);
  }
}


void usage() {
  fprintf(stderr, "fgcc  -  HardCore SoftWare's Frugal GCC Wrapper\n\n");
  fprintf(stderr, "Usage: fgcc [-o file] [-cc name] [-cgen file.c] [-bare] [-q] sources.fs\n");
  fprintf(stderr, "       -o file      Output executable filename\n");
  fprintf(stderr, "       -cgen file   Generate C code - do not compile\n");
  fprintf(stderr, "       -cc name     Generate C code - do not compile\n");
  fprintf(stderr, "       -q           Generate \"quiet\" code\n");
  fprintf(stderr, "       -bare        Don't include init.fs\n");
  fprintf(stderr, "       -norm        Compile a normal interpreter.\n\n");

  exit(-1);
}


int main (int argc, char *argv[]) {

  FILE *outfd;
  int i;
  char tpfile[100];

  if (argc<=1) usage();

  /* Parse args for options */
  for(i=1;i<argc;i++) {
    if (strcmp(argv[i], "-o") == 0) { i++; outfile=argv[i]; continue; }
    if (strcmp(argv[i], "-cgen") == 0) { i++; cgen=argv[i]; continue; }
    if (strcmp(argv[i], "-cc") == 0) { i++; cc=argv[i]; continue; }
    if (strcmp(argv[i], "-bare") == 0) { bare=1; continue; }
    if (strcmp(argv[i], "-norm") == 0) { norm=1; continue; }
    if (strcmp(argv[i], "-q") == 0) { quiet=1; continue; }
  }

  if (cgen && outfile) {
    fprintf(stderr, "ERROR: You can't combine -cgen and -o!\n");
    exit(-1);
  }

  /* Open output file */
  sprintf(tpfile, "./temp-fgcc.%d.c", getpid());
  outfd = fopen(tpfile, "w+");
  if (outfd == NULL) {
    fprintf(stderr, "ERROR: Couldn't create temporary file \"%s\"!\n", tpfile);
    exit(-1);
  }

  if (quiet) fprintf(outfd, "#define QUIET\n");
  fprintf(outfd, "#define INTERNAL_CODE\nchar *internal_code[] = {\n");
  if (bare==0) q_printfile(outfd, "init.fs");
  for(i=1;i<argc;i++) {
    if (strcmp(argv[i], "-o") == 0) { i++; continue; }
    if (strcmp(argv[i], "-cgen") == 0) { i++; continue; }
    if (strcmp(argv[i], "-cc") == 0) { i++; continue; }
    if (strcmp(argv[i], "-bare") == 0) { continue; }
    if (strcmp(argv[i], "-norm") == 0) { continue; }
    if (strcmp(argv[i], "-q") == 0) { continue; }
    q_printfile(outfd, argv[i]);
  }
  fprintf(outfd, "0\n};\n");
  n_printfile(outfd, "frugal.c");

  fclose(outfd);

  do_compile(tpfile);

  return 0;

}
