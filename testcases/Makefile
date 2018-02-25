CC = g++
OPT = -g -std=c++11
WARN = -Wall
INCLUDE = -I..
CFLAGS = $(OPT) $(WARN) $(INCLUDE)

#################################

# default rule
all: .cc.o

# generic rule for converting any .cc file to any .o file
.cc.o:
	$(CC) $(CFLAGS) -c *.cc
