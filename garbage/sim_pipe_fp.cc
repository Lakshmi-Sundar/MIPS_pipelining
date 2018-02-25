#include "sim_pipe_fp.h"
#include <stdlib.h>
#include <iostream>
#include <iomanip>

using namespace std;

//used for debugging purposes
static const char *reg_names[NUM_SP_REGISTERS] = {"PC", "NPC", "IR", "A", "B", "IMM", "COND", "ALU_OUTPUT", "LMD"};
static const char *stage_names[NUM_STAGES] = {"IF", "ID", "EX", "MEM", "WB"};

map <string, opcode_t> opcode_2str = { {"LW", LW}, {"SW", SW}, {"ADD", ADD}, {"ADDI", ADDI}, {"SUB", SUB}, {"SUBI", SUBI}, {"XOR", XOR}, {"XORI", XORI}, {"OR", OR}, {"ORI", ORI}, {"AND", AND}, {"ANDI", ANDI}, {"MULT", MULT}, {"DIV", DIV}, {"BEQZ", BEQZ}, {"BNEZ", BNEZ}, {"BLTZ", BLTZ}, {"BGTZ", BGTZ}, {"BLEZ", BLEZ}, {"BGEZ", BGEZ}, {"JUMP", JUMP}, {"EOP", EOP}, {"NOP", NOP}, {"LWS", LWS}, {"SWS", SWS}, {"ADDS", ADDS}, {"SUBS", SUBS}, {"MULTS", MULTS}, {"DIVS", DIVS}};

sim_pipe_fp::sim_pipe_fp(unsigned mem_size, unsigned mem_latency){
}
	
sim_pipe_fp::~sim_pipe_fp(){
}

void init_exec_unit(exe_unit_t exec_unit, unsigned latency, unsigned instances){
}

void sim_pipe_fp::load_program(const char *filename, unsigned base_address){
}

void sim_pipe_fp::run(unsigned cycles){
}
	
void sim_pipe_fp::reset(){
}

unsigned sim_pipe_fp::get_sp_register(sp_register_t reg, stage_t s){
	return 0; //please modify
}

int sim_pipe_fp::get_int_register(unsigned reg){
	return 0; //please modify
}

void sim_pipe_fp::set_int_register(unsigned reg, int value){
}

float sim_pipe_fp::get_fp_register(unsigned reg){
	return 0.0; //please modify
}

void sim_pipe_fp::set_fp_register(unsigned reg, float value){
}

float sim_pipe_fp::get_IPC(){
	return 0; //please modify
}

unsigned sim_pipe_fp::get_instructions_executed(){
	return 0; //please modify
}

unsigned sim_pipe_fp::get_stalls(){
	return 0; //please modify
}

void sim_pipe_fp::print_memory(unsigned start_address, unsigned end_address){
	cout << "data_memory[0x" << hex << setw(8) << setfill('0') << start_address << ":0x" << hex << setw(8) << setfill('0') <<  end_address << "]" << endl;
	unsigned i;
	for (i=start_address; i<end_address; i++){
		if (i%4 == 0) cout << "0x" << hex << setw(8) << setfill('0') << i << ": "; 
		cout << hex << setw(2) << setfill('0') << int(data_memory[i]) << " ";
		if (i%4 == 3) cout << endl;
	} 
}

void sim_pipe_fp::write_memory(unsigned address, unsigned value){
}

void sim_pipe_fp::print_registers(){
	cout << "Special purpose registers:" << endl;
        unsigned i, s;
	for (s=0; s<NUM_STAGES; s++){
		cout << "Stage: " << stage_names[s] << endl;  
		for (i=0; i< NUM_SP_REGISTERS; i++)
			if ((sp_register_t)i != IR && (sp_register_t)i != COND && get_sp_register((sp_register_t)i, (stage_t)s)!=UNDEFINED) cout << reg_names[i] << " = " << dec <<  get_sp_register((sp_register_t)i, (stage_t)s) << hex << " / 0x" << get_sp_register((sp_register_t)i, (stage_t)s) << endl;
	}
	cout << "General purpose registers:" << endl;
	for (i=0; i< NUM_GP_REGISTERS; i++)
		if (get_int_register(i)!=UNDEFINED) cout << "R" << dec << i << " = " << get_int_register(i) << hex << " / 0x" << get_int_register(i) << endl;
	for (i=0; i< NUM_GP_REGISTERS; i++)
		if (get_fp_register(i)!=UNDEFINED) cout << "F" << dec << i << " = " << get_fp_register(i) << hex << " / 0x" << get_fp_register(i) << endl;
}
