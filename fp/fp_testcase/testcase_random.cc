#include "sim_pipe_fp.h"
#include <iostream>
#include <cstring>
#include <stdlib.h>

using namespace std;

/* Test case for pipelined simuator */
/* DO NOT MODIFY */

/* convert a float into an unsigned */
inline unsigned float2unsigned(float value){
        unsigned result;
        memcpy(&result, &value, sizeof value);
        return result;
}

/* convert an unsigned into a float */
inline float unsigned2float(unsigned value){
        float result;
        memcpy(&result, &value, sizeof value);
        return result;
}


int main(int argc, char **argv){

	unsigned i, j;

	// instantiates the simulator with a 1MB data memory
	sim_pipe_fp *mips = new sim_pipe_fp(1024*1024, 9);

	mips->init_exec_unit(INTEGER, 5, 2);
	mips->init_exec_unit(ADDER, 6, 2);
	mips->init_exec_unit(MULTIPLIER, 1, 6);
	mips->init_exec_unit(DIVIDER, 9, 5);

	//loads program in instruction memory at address 0x10000000
	mips->load_program("asm/random.asm", 0x10000000);

	//initialize data memory and prints its content (for the specified address ranges)
	for (i = 0; i<5; i++) mips->write_memory(i*4,float2unsigned((float)i/2));
	mips->set_fp_register(1, 0.0);
	mips->set_int_register(4, 1);
	
	cout << "\nBEFORE PROGRAM EXECUTION..." << endl;
	cout << "======================================================================" << endl << endl;
	
	//prints the value of the memory and registers
	mips->print_registers();
	mips->print_memory(0x0, 0x2F);

	// executes the program	
	cout << "\n*****************************" << endl;
	cout << "STARTING THE PROGRAM..." << endl;
	cout << "*****************************" << endl << endl;

	// runs program to completion
	cout << "EXECUTING PROGRAM TO COMPLETION..." << endl << endl;
	mips->run(); 

	cout << "PROGRAM TERMINATED\n";
	cout << "===================" << endl << endl;

	//prints the value of registers and data memory
	mips->print_registers();
	mips->print_memory(0x0, 0x2F);
	
	cout << endl;

	// prints the number of instructions executed and IPC
	cout << "Instruction executed = " << dec << mips->get_instructions_executed() << endl;
	cout << "Clock cycles = " << dec << mips->get_clock_cycles() << endl;
	cout << "Stall inserted = " << dec  << mips->get_stalls() << endl;
	cout << "IPC = " << dec << mips->get_IPC() << endl;
}
