#include "sim_pipe.h"
#include <stdlib.h>
#include <iostream>
#include <iomanip>

using namespace std;

//used for debugging purposes
static const char *reg_names[NUM_SP_REGISTERS] = {"PC", "NPC", "IR", "A", "B", "IMM", "COND", "ALU_OUTPUT", "LMD"};
static const char *stage_names[NUM_STAGES] = {"IF", "ID", "EX", "MEM", "WB"};


sim_pipe::sim_pipe(unsigned mem_size, unsigned mem_latency){
   //updating data memory size and the memory latency
   this->dataMemSize  = mem_size;
   this->memLatency   = mem_latency;
   reset();
}

//fetching instruction from the instruction memory
instructT sim_pipe::fetchInstruction ( uint32_t pc ) {
   uint32_t index = (pc - this->baseAddress)/4;
   return *instMemory[index];
}

void sim_pipe::fetch() {
   // NPC is current PC
   this->pipeReg[IF][PC]        = (this->instrArray[MEM][COND]) ? this->pipeReg[EX][NPC] : this->pipeReg[IF][NPC];

   //fetching instruction from instruction memory
   // FIXME: LHS is T, RHS is PT
   instructT instruct           = fetchInstruction(this->pipeReg[IF][PC]);

   //moving instruction into ID stage
   this->instrArray[ID]         = instruct;
   this->pipeReg[IF][NPC]       = this->pipeReg[IF][PC] + 4;

   //FIXME: Updating IR register
}

bool sim_pipe::decode() {
   //local variable for instruction
   instructT instruct           = this->instrArray[ID];
   bool stall                   = false;
   switch (instruct.opcode) {
      case LW:{
                 if(this->gprFile[instruct.opr2].busy) {
                    stall = true;
                 }
                 else {
                    this->pipeReg[EX][A]              = instruct.opr2;
                    this->pipeReg[EX][IMM]            = instruct.imm;
                 }
                 break;
              }    
      case SW:{
                 if(this->gprFile[instruct.opr0].busy | this->gprFile[instruct.opr2].busy) {
                    stall = true;
                 }
                 else {
                    this->pipeReg[EX][A]              = instruct.opr0;
                    this->pipeReg[EX][B]              = instruct.opr2;
                    this->pipeReg[EX][IMM]            = instruct.imm;
                 }
                 break;
              }
      case ADD ... DIV:{
                          if(this->gprFile[instruct.opr1].busy | this->gprFile[instruct.opr2].busy) {
                             stall = true;
                          }
                          else {
                             this->pipeReg[EX][A]     = instruct.opr1;
                             this->pipeReg[EX][B]     = instruct.opr2;
                          }
                          break;
                       }
      case ADDI ... ANDI:{
                            if(this->gprFile[instruct.opr1].busy) {
                               stall = true;
                            }
                            else {
                               this->pipeReg[EX][A]     = instruct.opr1;
                               this->pipeReg[EX][IMM]   = instruct.imm;
                            }
                            break;
                         }
      case BEQZ ... JUMP:{
                            if(this->gprFile[instruct.opr0].busy) {
                               stall = true;
                            }
                         }
   }

   this->gprFile[instruct.opr0].busy = true;

}





this->instrArray[EX]         = instruct;
}

//function to generate address for LW SW instructions
uint32_t sim_pipe::agen ( instructT instruct) {
   return (instruct.opr1 + get_gp_register(instruct.opr2));
}

//function to perform ALU operations
uint32_t sim_pipe::alu (uint32_t value1, uint32_t value2, opcode_t opcode){
   if(opcode == ADD || opcode == ADDI)
      return value1 + value2; 
   else if(opcode == SUB || opcode == SUBI)
      return value1 - value2;
   else if(opcode == XOR || opcode == XORI) 
      return value1 ^ value2;
   else if(opcode == AND || opcode == ANDI)
      return value1 & value2;
   else if(opcode == OR || opcode == ORI)
      return value1 | value2;
}

//Performs the execute operations according to ISA
//updates COND and ALU_OUTPUT registers
void sim_pipe::execute() {

   //local variable for the instruction
   instructT instruct         = this->instrArray[EX]; 

   switch(instruct.opcode) {
      case LW:{
                 this->pipeReg[MEM][ALU_OUTPUT] = agen (instruct);
                 break;
              }

      case SW:{
                 this->pipeReg[MEM][ALU_OUTPUT] = agen (instruct);
                 break;
              }

      case ADD:
      case SUB:
      case XOR:
      case OR:
      case AND:{
                  this->pipeReg[MEM][ALU_OUTPUT] = alu(get_gp_register(instruct.opr1), 
                        get_gp_register(instruct.opr2), instruct.opcode);
                  break;
               }

      case ADDI:
      case SUBI:
      case XORI:
      case ORI:
      case ANDI:{
                   //int imm = instructP->opr2;
                   //imm     = (imm << 16) >> 16;
                   this->pipeReg[MEM][ALU_OUTPUT] = alu(get_gp_register(simP, instruct.opr1),
                         this->pipeReg[EX][IMM], instruct.opcode);
                   break;
                }

      case BLTZ:{
                   if(get_gp_register(simP, instruct.opr0) < 0) 
                      this->pipeReg[EX][NPC] = instruct.opr1;
                   break;
                }

      case BNEZ:{
                   if(get_gp_register(simP, instructP->opr0) != 0) 
                      this->pipeReg[EX][NPC] = instruct.opr1;
                   break;
                }
      case BEQZ:
      case BGTZ:
      case BGEZ:
      case BLEZ:
      case JUMP:
      case NOP:
      case EOP: {
                   return 1;
                }
   }
}

//Loads / stores to the data memory
void sim_pipe::memory() {

   instructT instruct          = this->instrArray[MEM]; 
   switch(instruct.opcode) {
      case LW:{
                 this->gprFile[instruct.opr0] = this->dataMemory[this->pipeReg[MEM][ALU_OUTPUT]];
                 break;
              }

      case SW:{
                 this->dataMemory[this->pipeReg[MEM][ALU_OUTPUT]] = get_gp_register(instruct.opr0);
                 break;
              }
   }
   this->instrArray[WB]         = instruct;
}

//Updates value of GPR according to destination
void sim_pipe::writeBack() {
   instructT instruct             = this->instrArray[WB]; 
   this->gprFile[instruct.opr0] = this->pipeReg[MEM][ALU_OUTPUT];
}

//de-allocate
sim_pipe::~sim_pipe(){
}

//parse trace and load it intro instruction memory
void sim_pipe::load_program(const char *filename, unsigned base_address){
}

//run the program until EOP or end of file
void sim_pipe::run(unsigned cycles){
}

//resets the state of the simulator
/* Note: 
   - registers should be reset to UNDEFINED value 
   - data memory should be reset to all 0xFF values
   */
void sim_pipe::reset(){
   //initializing dataMemory to UNDEFINED
   for(int i = 0; i < dataMemSize; i++) {
      this->dataMemory[i] = UNDEFINED; 
   }

   //initializing Instruction Array to UNDEFINED
   for(int i = 0; i < NUM_STAGES; i++) {
      this->instrArray[i] = UNDEFINED;
   }

   //initializing GPRs to UNDEFINED
   for(int i = 0; i < NUM_GPR; i++) {
      this->gprFile[i] = UNDEFINED;
   }

   //initializing pipeline registers to UNDEFINED
   for(int i = 0; i < NUM_STAGES; i++) {
      for(int i = 0; i < NUM_SP_REGISTERS; i++) {
         this->instrArray[i] = UNDEFINED;
      }
   }
}

unsigned sim_pipe::get_sp_register(sp_register_t reg, stage_t s){
   return 0; //please modify
}

int sim_pipe::get_gp_register(unsigned reg){
   return this->gprFile[reg].gprValue;
}

void sim_pipe::set_gp_register(unsigned reg, int value){
}

float sim_pipe::get_IPC(){
   return 0; //please modify
}

unsigned sim_pipe::get_instructions_executed(){
   return 0; //please modify
}

unsigned sim_pipe::get_stalls(){
   return 0; //please modify
}

unsigned sim_pipe::get_clock_cycles(){
   return 0; //please modify
}


//--------------------------------------PRINT OPERATIONS--------------------------------------------------------------------//
void sim_pipe::print_memory(unsigned start_address, unsigned end_address){
   cout << "data_memory[0x" << hex << setw(8) << setfill('0') << start_address << ":0x" << hex << setw(8) << setfill('0') <<  end_address << "]" << endl;
   unsigned i;
   for (i=start_address; i<end_address; i++){
      if (i%4 == 0) cout << "0x" << hex << setw(8) << setfill('0') << i << ": "; 
      cout << hex << setw(2) << setfill('0') << int(data_memory[i]) << " ";
      if (i%4 == 3) cout << endl;
   } 
}

void sim_pipe::write_memory(unsigned address, unsigned value){
}

void sim_pipe::print_registers(){
   cout << "Special purpose registers:" << endl;
   unsigned i, s;
   for (s=0; s<NUM_STAGES; s++){
      cout << "Stage: " << stage_names[s] << endl;  
      for (i=0; i< NUM_SP_REGISTERS; i++)
         if ((sp_register_t)i != IR && (sp_register_t)i != COND && get_sp_register((sp_register_t)i, (stage_t)s)!=UNDEFINED) cout << reg_names[i] << " = " << dec <<  get_sp_register((sp_register_t)i, (stage_t)s) << hex << " / 0x" << get_sp_register((sp_register_t)i, (stage_t)s) << endl;
   }
   cout << "General purpose registers:" << endl;
   for (i=0; i< NUM_GP_REGISTERS; i++)
      if (get_gp_register(i)!=UNDEFINED) cout << "R" << dec << i << " = " << get_gp_register(i) << hex << " / 0x" << get_gp_register(i) << endl;
}
