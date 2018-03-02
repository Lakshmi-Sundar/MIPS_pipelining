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
   //updating data memory size and the memory latency
   this->dataMemSize  = mem_size;
   this->memLatency   = mem_latency;
   this->instMemory   = NULL;
   numStalls          = 0;
   reset();
}

sim_pipe_fp::~sim_pipe_fp(){
}

//FIXME: add "configurable" EX stage units
void sim_pipe_fp::init_exec_unit(exe_unit_t exec_unit, unsigned latency, unsigned instances){
}

//------------------------LOADING PROGRAM---------------------------------------------------------------//
void sim_pipe_fp::load_program(const char *filename, unsigned base_address){
   parse(filename);
   this->pipeReg[IF][PC]  = base_address;
   this->baseAddress      = base_address;
}
//---------------------------END OF LOAD PROGRAM--------------------------------------------------------//

//fetching instruction from instruction memory
instructT sim_pipe_fp::fetchInstruction ( unsigned pc ) {
   uint32_t index = (pc - this->baseAddress)/4;
   instCount++;
   return *(instMemory[index]);
}

//---------------------------------------------RUN SIMULATION--------------------------------------------//
void sim_pipe_fp::run(unsigned cycles){
}
//---------------------------------------------END OF RUN------------------------------------------------//


//---------------------------------------------RESET BLOCK-----------------------------------------------//
void sim_pipe_fp::reset(){
//initializing data_memory to UNDEFINED
   this->data_memory       = new unsigned char[dataMemSize];
   for(unsigned i = 0; i < this->dataMemSize; i++) {
      this->data_memory[i] = UNDEFINED; 
   }

   //initializing Instruction Array to UNDEFINED
   for(int i = 0; i < NUM_STAGES; i++) {
      this->instrArray[i].nop();
   }

   //initializing GPRs to UNDEFINED
   for(int i = 0; i < NUM_GP_REGISTERS; i++) {
      this->gprFile[i].value = UNDEFINED;
   }

   //initializing pipeline registers to UNDEFINED
   for(int i = 0; i < NUM_STAGES; i++) {
      for(int j = 0; j < NUM_SP_REGISTERS; j++) {
         this->pipeReg[i][j]  = UNDEFINED;
      }
      this->pipeReg[i][COND]  = 0;
   }
}

//-----------------------------------------------END OF RESET BLOCK----------------------------------------//

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

//-------------------------------------------------PRINT OPERATION BEGINS-------------------------------------------------------------------------//
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

unsigned sim_pipe_fp::get_clock_cycles(){
   //FIXME
   return 0;
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
//-------------------------------------------------PRINT OPERATION ENDS-------------------------------------------------------------------------//

//FIXME: Add floating point operations

int sim_pipe_fp::labelToPC( const char* filename, const char* label, uint32_t pc_index ){
   FILE* temp  = fopen(filename, "r");
   int line    = 0;
   do{
      char str[25];
      fscanf(temp, "%s", str);
      if(str[strlen(str)-1] == ':'){
         str[strlen(str)-1] = '\0';
         if(!strcmp(str, label)){
            break;
         }
      }
      if( opcode_2str.count( string( str ) ) > 0 )
         line++;
   }while(!feof(temp));
   fclose(temp);
   return ((line - pc_index - 1) * 4);
}

int sim_pipe_fp::parse( const char* filename ){
   FILE* trace;
   char buff[1024], label[495];
   int a, b, c, lineNo = 0;
   char imm[32];

   trace  = fopen(filename, "r");
   ASSERT(trace, "Unable to open file %s", filename);

   do {
      instMemory               = (instructPT*) realloc(instMemory, (lineNo + 1)*sizeof(instructPT));
      instructPT instructP     = new instructT;

      instMemory[lineNo]       = instructP;
      fscanf(trace, "%s ", buff);

      if( opcode_2str.count( string(buff) ) <= 0 ){
         ASSERT( buff[strlen(buff)-1] == ':', "Unkown buff(%s) encountered", buff );
         fscanf(trace, "%s ", buff);
      }

      instructP->opcode        = opcode_2str[ string(buff) ];

      switch( instructP->opcode ){
         case ADD ... DIV:
         case ADDS ... DIVS:
            a                     = parseReg(trace, instructP->dstF);
            b                     = parseReg(trace, instructP->src1F);
            c                     = parseReg(trace, instructP->src2F);
            instructP->dst        = a;
            instructP->src1       = b;
            instructP->src2       = c;
            instructP->dstValid   = true;
            instructP->src1Valid  = true;
            instructP->src2Valid  = true;
            break;

         case BEQZ ... BGEZ:
            a                     = parseReg(trace, instructP->src1F);
            fscanf(trace, "%s", label);
            
            instructP->src1       = a;
            instructP->imm        = labelToPC( filename, label, lineNo );
            instructP->src1Valid  = true;
            instructP->is_branch  = true;
            break;

         case ADDI ... ANDI:
            a                     = parseReg(trace, instructP->dstF);
            b                     = parseReg(trace, instructP->src1F);
            fscanf(trace, "%s", imm);
            if( imm[1] == 'x' || imm[1] == 'X' ){
               c                  = /*HEX*/     strtol( imm + 2, NULL, 16 );
            } else{
               c                  = /*DECIMAL*/ strtol( imm, NULL, 10 );
            }
            instructP->dst        = a;
            instructP->src1       = b;
            instructP->imm        = c;
            instructP->dstValid   = true;
            instructP->src1Valid  = true;
            break;

         case JUMP:
            fscanf(trace, "%s", label);
            instructP->imm        = labelToPC( filename, label, lineNo );
            instructP->is_branch  = true;
            break;

         case LW:
         case LWS:
            a                     = parseReg(trace, instructP->dstF);
            fscanf(trace, "%d(", &b);
            c                     = parseReg(trace, instructP->src1F);
            instructP->dst        = a;
            instructP->imm        = b;
            instructP->src1       = c;
            instructP->dstValid   = true;
            instructP->src1Valid  = true;
            break;

         case SW:
         case SWS:
            a                     = parseReg(trace, instructP->src2F);
            fscanf(trace, "%d(", &b);
            c                     = parseReg(trace, instructP->src1F);
            instructP->src2       = a;
            instructP->imm        = b;
            instructP->src1       = c;
            instructP->src2Valid  = true;
            instructP->src1Valid  = true;
            break;

         case EOP:
         case NOP:
            break;

         default:
            ASSERT(false, "Unknown operation encountered");
            break;
      }
      instructP->print();
      lineNo++;
   }while(!feof(trace));

   return lineNo;
}

uint32_t sim_pipe_fp::parseReg( FILE* trace, bool& is_float ){
   uint32_t reg;
   char     regIdentifier, dummy;
   fscanf(trace, "%c%d%c", &regIdentifier, &reg, &dummy);
   if( regIdentifier == 'R' || regIdentifier == 'r' ){
      is_float  = false;
   }
   else if( regIdentifier == 'F' || regIdentifier == 'f' ){
      is_float  = true;
   }
   else {
      ASSERT(false, "Unknown register identifier found (=%c)", regIdentifier);
   }
   return reg;
}

inline unsigned sim_pipe_fp::float2unsigned(float value){
        unsigned result;
        memcpy(&result, &value, sizeof value);
        return result;
}

inline float sim_pipe_fp::unsigned2float(unsigned value){
        float result;
        memcpy(&result, &value, sizeof value);
        return result;
}

