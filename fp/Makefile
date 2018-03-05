CC = g++
OPT = -g -std=c++11
WARN = -Wall
CFLAGS = $(OPT) $(WARN) 

# List corresponding compiled object files here (.o files)
SIM_OBJ = sim_pipe_fp.o

TESTCASES = fp_testcase testcase1 testcase2 testcase3 testcase4 testcase5 testcase6
 
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

testcase1: .cc.o testcase 
	$(CC) -o bin/testcase1 $(CFLAGS) $(SIM_OBJ) fp_testcase/testcase1.o

testcase2: .cc.o testcase
	$(CC) -o bin/testcase2 $(CFLAGS) $(SIM_OBJ) fp_testcase/testcase2.o

testcase3: .cc.o testcase 
	$(CC) -o bin/testcase3 $(CFLAGS) $(SIM_OBJ) fp_testcase/testcase3.o

testcase4: .cc.o testcase
	$(CC) -o bin/testcase4 $(CFLAGS) $(SIM_OBJ) fp_testcase/testcase4.o

testcase5: .cc.o testcase 
	$(CC) -o bin/testcase5 $(CFLAGS) $(SIM_OBJ) fp_testcase/testcase5.o

testcase6: .cc.o testcase
	$(CC) -o bin/testcase6 $(CFLAGS) $(SIM_OBJ) fp_testcase/testcase6.o

# type "make clean" to remove all .o files plus the sim binary
clean:
	rm -f testcases/*.o
	rm -f *.o 
	rm -f bin/*
