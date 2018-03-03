#ifndef SIM_PIPE_H_
#define SIM_PIPE_H_

#include <stdio.h>
#include <string.h>
#include <inttypes.h>
#include <stdlib.h>
#include <iostream>
#include <iomanip>
#include <cassert>
#include <map>
#include <cstdlib>
#include <malloc.h>

#include <vector>
#include <algorithm>

#define UNDEFINED 0xFFFFFFFF //constant used to initialize registers
#define DATA_UNDEF 0
#define NUM_SP_REGISTERS 9
#define NUM_GP_REGISTERS 32
#define NUM_OPCODES 23
#define NUM_STAGES 5

#define ASSERT( condition, statement, ... ) \
   if( !(condition) ) { \
      printf( "[ASSERT] In File: %s, Line: %d => " #statement "\n", __FILE__, __LINE__, ##__VA_ARGS__ ); \
      abort(); \
   }

using namespace std;
typedef enum {PC, NPC, IR, A, B, IMM, COND, ALU_OUTPUT, LMD} sp_register_t;

// The NOP instruction should be automatically inserted by the processor to implement pipeline bubbles
typedef enum {LW, SW, ADD, SUB, XOR, OR, AND, MULT, DIV, ADDI, SUBI, XORI, ORI, ANDI, BEQZ, BNEZ, BLTZ, BGTZ, BLEZ, BGEZ, JUMP, EOP, NOP} opcode_t;
const string opcode_str[] = {"LW", "SW", "ADD", "SUB", "XOR", "OR", "AND", "MULT", "DIV", "ADDI", "SUBI", "XORI", "ORI", "ANDI", "BEQZ", "BNEZ", "BLTZ", "BGTZ", "BLEZ", "BGEZ", "JUMP", "EOP", "NOP"};

typedef enum {IF, ID, EX, MEM, WB} stage_t;


typedef struct instructT* instructPT;

struct instructT{
   opcode_t           opcode;
   uint32_t           dst;
   uint32_t           src1;
   uint32_t           src2;
   uint32_t           imm;
   bool               dstValid;
   bool               src1Valid;
   bool               src2Valid;
   bool               is_stall;
   bool               is_branch;

   instructT(){
      nop();
   }

   void nop(){
      opcode     = NOP;
      dst        = UNDEFINED;
      src1       = UNDEFINED;
      src2       = UNDEFINED;
      imm        = UNDEFINED;
      dstValid   = false;
      src1Valid  = false;
      src2Valid  = false;
      is_stall   = false;
      is_branch  = false;
   }

   void stall(){
      nop();
      is_stall   = true;
   }
};


class sim_pipe{

public:
   struct gprFileT{
      int            value;
      int            busy;
   };

   int               cycleCount;
   int               instCount;
   int               latCount;
   int               numStalls;
   bool              latency;

   gprFileT          gprFile[NUM_GP_REGISTERS];
   uint32_t          sprFile[NUM_SP_REGISTERS];
   uint32_t          pipeReg[NUM_STAGES][NUM_SP_REGISTERS];

   instructT         instrArray[NUM_STAGES];

   unsigned char*    data_memory;
   instructPT        *instMemory;
   unsigned          dataMemSize;
   unsigned          memLatency;
   unsigned          memFlag;
   unsigned          baseAddress;



   sim_pipe(unsigned data_mem_size, unsigned data_mem_latency);

   //de-allocates the simulator
   ~sim_pipe();
   instructT fetchInstruction ( uint32_t pc );

   void     fetch(bool stall);
   bool     decode(); 
   uint32_t agen(instructT instruct);
   uint32_t alu (uint32_t value1, uint32_t value2, opcode_t opcode);
   void     execute();
   bool     memory();
   bool     writeBack();

   int parse(const char *filename);
   int  labelToPC( const char* filename, const char* label, uint32_t pc_index );

   //loads the assembly program in file "filename" in instruction memory at the specified address
   void load_program(const char *filename, unsigned base_address=0x0);

   //runs the simulator for "cycles" clock cycles (run the program to completion if cycles=0) 
   void run(unsigned cycles=0);
	
	//resets the state of the simulator
        /* Note: 
	   - registers should be reset to UNDEFINED value 
	   - data memory should be reset to all 0xFF values
	*/
	void reset();

	// returns value of the specified special purpose register for a given stage (at the "entrance" of that stage)
        // if that special purpose register is not used in that stage, returns UNDEFINED
        //
        // Examples (refer to page C-37 in the 5th edition textbook, A-32 in 4th edition of textbook)::
        // - get_sp_register(PC, IF) returns the value of PC
        // - get_sp_register(NPC, ID) returns the value of IF/ID.NPC
        // - get_sp_register(NPC, EX) returns the value of ID/EX.NPC
        // - get_sp_register(ALU_OUTPUT, MEM) returns the value of EX/MEM.ALU_OUTPUT
        // - get_sp_register(ALU_OUTPUT, WB) returns the value of MEM/WB.ALU_OUTPUT
	// - get_sp_register(LMD, ID) returns UNDEFINED
	/* Note: you are allowed to use a custom format for the IR register.
           Therefore, the test cases won't check the value of IR using this method. 
	   You can add an extra method to retrieve the content of IR */
	unsigned get_sp_register(sp_register_t reg, stage_t stage);
   void set_sp_register(sp_register_t reg, stage_t s, uint32_t value);

	//returns value of the specified general purpose register
	int get_gp_register(unsigned reg);

	// set the value of the given general purpose register to "value"
	void set_gp_register(unsigned reg, int value);

	//returns the IPC
	float get_IPC();

	//returns the number of instructions fully executed
	unsigned get_instructions_executed();

	//returns the number of stalls added by processor
	unsigned get_stalls();

	//returns the number of clock cycles
	unsigned get_clock_cycles();

	//prints the content of the data memory within the specified address range
	void print_memory(unsigned start_address, unsigned end_address);

	// writes an integer value to data memory at the specified address (use little-endian format: https://en.wikipedia.org/wiki/Endianness)
	void write_memory(unsigned address, unsigned value);

   unsigned read_memory(unsigned address);

	//prints the values of the registers 
	void print_registers();

};

#endif /*SIM_PIPE_H_*/
