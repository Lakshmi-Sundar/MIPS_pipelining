CC = g++
OPT = -g -std=c++11
WARN = -Wall
CFLAGS = $(OPT) $(WARN) 

# List corresponding compiled object files here (.o files)
SIM_OBJ = sim_pipe_fp.o

TESTCASES = testcase_cf testcase_fp1 testcase_fp1_1 testcase_fp2 testcase_fp3 testcase_fp4 testcase_fp5 testcase1 testcase2 testcase3 testcase4 testcase5 testcase6
 
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
testcase_cf: .cc.o testcase 
	$(CC) -o bin/testcase_cf $(CFLAGS) $(SIM_OBJ) fp_testcase/testcase_cf.o

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

testcase_fp1: .cc.o testcase
	$(CC) -o bin/testcase_fp1 $(CFLAGS) $(SIM_OBJ) fp_testcase/testcase_fp1.o

testcase_fp1_1: .cc.o testcase
	$(CC) -o bin/testcase_fp1_1 $(CFLAGS) $(SIM_OBJ) fp_testcase/testcase_fp1_1.o

testcase_fp2: .cc.o testcase
	$(CC) -o bin/testcase_fp2 $(CFLAGS) $(SIM_OBJ) fp_testcase/testcase_fp2.o

testcase_fp3: .cc.o testcase
	$(CC) -o bin/testcase_fp3 $(CFLAGS) $(SIM_OBJ) fp_testcase/testcase_fp3.o

testcase_fp4: .cc.o testcase
	$(CC) -o bin/testcase_fp4 $(CFLAGS) $(SIM_OBJ) fp_testcase/testcase_fp4.o

testcase_fp5: .cc.o testcase
	$(CC) -o bin/testcase_fp5 $(CFLAGS) $(SIM_OBJ) fp_testcase/testcase_fp5.o

# type "make clean" to remove all .o files plus the sim binary
clean:
	rm -f fp_testcase/*.o
	rm -f *.o 
	rm -f bin/*
