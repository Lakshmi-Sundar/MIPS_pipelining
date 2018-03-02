CC = g++
OPT = -g -std=c++11
WARN = -Wall
CFLAGS = $(OPT) $(WARN) 

# List corresponding compiled object files here (.o files)
SIM_OBJ = sim_pipe_fp.o

TESTCASES = fp_testcase
 
#################################

# default rule
all:	$(TESTCASES)

# generic rule for converting any .cc file to any .o file
.cc.o:
	$(CC) $(CFLAGS) -c *.cc

#rule for creating the object files for all the testcases in the "testcases" folder
testcase: 
	$(MAKE) -C fp_testcase

# rules for making testcases
fp_testcase: .cc.o testcase 
	$(CC) -o bin/fp_testcase $(CFLAGS) $(SIM_OBJ) fp_testcase/testcase_fp1.o

# type "make clean" to remove all .o files plus the sim binary
clean:
	rm -f testcases/*.o
	rm -f *.o 
	rm -f bin/*
