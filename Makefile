CC=gcc
binaries=MyShell
all: $(binaries) # make all
clean: $(RM) -f $(binaries) *.o # make clean: rm -f MyShell *.o