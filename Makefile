
LIBS=-L/lsf/7.0/linux2.6-glibc2.3-x86_64/lib/ -llsf -lbat -lm -lnsl
#LIBS=-L/n/home00/jcuff/lava/lib/ -L/lsf/7.0/linux2.6-glibc2.3-x86_64/lib/ -llsf -lbat -lm -lnsl
CC=icc
CFLAGS= -O3 -vec-report1 -I/lsf/7.0/include
#CFLAGS= -O3 -I/n/home00/jcuff/lava/include -I/lsf/7.0/include
#CFLAGS= -g -traceback -I/lsf/7.0/include
RM=/bin/rm

EXECS = showq
OBJS = showq.o 

all: $(EXECS)

$(EXECS): $(OBJS) Makefile
	$(CC) $(CFLAGS) -o $(EXECS) $(OBJS) $(LIBS)

$(OBJS): Makefile

clean:
	$(RM) -f $(EXECS) $(OBJS) *~ *.o *.oo


