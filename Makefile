CC=gcc
binaries=JBash.c JBash.h
all: $(binaries) # make all
clean: $(RM) -f $(binaries) *.o # make clean: rm -f JBash *.o