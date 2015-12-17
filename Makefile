# NORMAL:
CFLAGS= -s -O2 -Wall

# SUNOS:
#CFLAGS= -lsocket -lnsl -s -O2 -Wall

# MACOS X: (YOU MAY BE ABLE TO USE NORMAL)
#CFLAGS= -O2 -Wall

# DEBUGGING:
#CFLAGS= -g -Wall

CC=gcc
LIBS= -L/usr/local/lib
INCLUDES= -I/usr/local/include

BINNAME=frugal
FGCCBINNAME=fgcc

all: frugal fgcc

frugal: frugal.c extprims.c
	$(CC) $(CFLAGS) $(INCLUDES) $(LIBS) -o $(BINNAME) frugal.c

fgcc: fgcc.c
	$(CC) $(CFLAGS) $(INCLUDES) $(LIBS) -o $(FGCCBINNAME) fgcc.c

clean:
	rm -f $(BINNAME) $(FGCCBINNAME) *.o core *.core *.a *.out *.cgi *.exe
