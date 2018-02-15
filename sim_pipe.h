#ifndef SIM_PIPE_H_
#define SIM_PIPE_H_

#include <stdio.h>
#include <inttypes.h>

#define UNDEFINED 0xFFFFFFFF //constant used to initialize registers
#define NUM_SP_REGISTERS 9
#define NUM_GP_REGISTERS 32
#define NUM_OPCODES 23
#define NUM_STAGES 5

typedef enum {PC, NPC, IR, A, B, IMM, COND, ALU_OUTPUT, LMD} sp_register_t;

// The NOP instruction should be automatically inserted by the processor to implement pipeline bubbles
typedef enum {LW, SW, ADD, SUB, XOR, OR, AND, MULT, DIV, ADDI, SUBI, XORI, ORI, ANDI, BEQZ, BNEZ, BLTZ, BGTZ, BLEZ, BGEZ, JUMP, EOP, NOP} opcode_t;

typedef enum {IF, ID, EX, MEM, WB} stage_t;

class sim_pipe{

public:

   typedef struct instructT* instructPT;

   struct instructT{
      opcode_t           opcode;
      uint32_t           opr0;
      uint32_t           opr1;
      uint32_t           opr2;
      uint32_t           imm;
   };

   struct gprFileT{
      uint32_t       gprValue;
      bool           busy;
   };

   int               cycleCount;
   int               instCount;

   gprFileT          gprFile[NUM_GPR];
   uint32_t          sprFile[NUM_SP_REGISTERS];
   uint32_t          pipeReg[NUM_STAGES][NUM_SP_REGISTERS];

   instructT         instrArray[NUM_STAGES];

   uchar*            dataMemory;
   instructPT        *instMemory;
   unsigned          dataMemSize;
   unsigned          memLatency;
   unsigned          baseAddress;



   sim_pipe(unsigned data_mem_size, unsigned data_mem_latency);

   //de-allocates the simulator
   ~sim_pipe();

   void     fetch();
   void     decode(); 
   uint32_t agen();
   uint32_t alu();
   void     execute();
   void     memory();
   void     writeBack();

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

	//prints the values of the registers 
	void print_registers();

};

#endif /*SIM_PIPE_H_*/
