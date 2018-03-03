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

void sim_pipe_fp::init_exec_unit(exe_unit_t exec_unit, unsigned latency, unsigned instances){
   execFp[exec_unit]  = execUnitT(instances, latency);
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

void sim_pipe_fp::fetch(bool stall) {
   // NPC is current PC
   bool cond                       = get_sp_register(COND, MEM);
   uint32_t alu_output             = get_sp_register(ALU_OUTPUT, MEM);
   pipeReg[IF][PC]                 = cond ? alu_output : pipeReg[IF][PC];
   uint32_t currentFetchPC         = this->pipeReg[IF][PC];

   // The following if condition will not happen in actual RTL
   if( !stall ){
      instructT instruct           = fetchInstruction(currentFetchPC);
      if(instruct.opcode != EOP )
         set_sp_register(PC, IF, currentFetchPC + 4);

      this->pipeReg[ID][NPC]       = this->pipeReg[IF][PC];

      this->instrArray[ID]         = instruct;
   }
}

bool sim_pipe_fp::regBusy(uint32_t regNo, bool isF) {
   return isF ? fpFile[regNo].busy : gprFile[regNo].busy;
}

bool sim_pipe_fp::intBranch(){
   for(int i = 0; i < execFp[INTEGER].numLanes; i++) {
      if( execFp[INTEGER].lanes[i].instructP->is_branch )
         return true;
   }
   return false;
}

exe_unit_t sim_pipe_fp::opcodeToExUnit(opcode_t opcode){
   switch( opcode ){
      case ADD ... SWS:
         return INTEGER;
         break;

      case ADDS:
      case SUBS:
         return ADDER;
         break;

      case MULTS:
         return MULTIPLIER;
         break;

      case DIVS:
         return DIVIDER;
         break;
      default: 
         ASSERT (false, "Opcode not supported");
         return INTEGER;
         break;
   }
}

int sim_pipe_fp::exLatency(opcode_t opcode) {
   return execFp[opcodeToExUnit(opcode)].latency;
}

bool sim_pipe_fp::decode() {
   bool stallEx                         = false;

   instructT instruct                   = this->instrArray[ID];
   this->pipeReg[EX][NPC]               = this->pipeReg[ID][NPC];

   //------------------------------ RAW BEGIN --------------------------------------
   if( (instruct.src1Valid && regBusy(instruct.src1, instruct.src1F)) ||
         (instruct.src2Valid && regBusy(instruct.src2, instruct.src2F)) ) {
      stallEx                 = true;
   }
   //------------------------------- RAW ENDS ---------------------------------------
   //----------------------- WAW & EX CONTENTION BEGINS -----------------------------
   for(int i = 0; i < EXEC_UNIT_TOTAL && !stallEx; i++) {
      for(int j = 0; j < execFp[i].numLanes; j++) {
         execLaneT lane       = execFp[i].lanes[j];
         int latency          = exLatency(instruct.opcode);
         if(latency == lane.ttl) {
            stallEx           = true;
            break;
         }
         else if( (instruct.dstValid && lane.instructP->dstValid) && // Destinations should be valid
               (instruct.dst == lane.instructP->dst) && // Reg numbers should match
               (instruct.dstF == lane.instructP->dstF) && // Both should either be R or F
               (latency <= lane.ttl) ){ // Latency must be LTE TTL
            stallEx     = true;
            break;
         }
      }
   }
   //------------------------ WAW & EX CONTENTION ENDS ------------------------------
   //-------------------------- BRANCH CONTROL BEGINS -------------------------------
   bool stallBranch                     = instruct.is_branch || instrArray[EX].is_branch || intBranch();
   if( stallBranch ) { 
      this->instrArray[ID].stall();
      numStalls++;
      for(int i = 0; i < NUM_SP_REGISTERS; i++) {
         this->pipeReg[ID][i]           = UNDEFINED;
      }
   }
   //--------------------------- BRANCH CONTROL ENDS --------------------------------

   if( stallEx ){
      // Stalling
      this->instrArray[EX].stall();
      numStalls++;
      for(int i = 0; i < NUM_SP_REGISTERS; i++) {
         this->pipeReg[EX][i]           = UNDEFINED;
      }
      return true;
   } 
   else{
      // Issue to EX
      //incrementing busy to handle hazards caused by RAW and WAW-R dependencies
      if(instruct.dstValid)
         this->gprFile[instruct.dst].busy++;

      this->pipeReg[EX][A]                 = (instruct.src1Valid) ? get_int_register(instruct.src1) : UNDEFINED;
      this->pipeReg[EX][B]                 = (instruct.src2Valid) ? get_int_register(instruct.src2) : UNDEFINED;
      this->pipeReg[EX][IMM]               = instruct.imm;

      this->instrArray[EX]                 = instruct;
      return (instruct.opcode == EOP || stallBranch);
   }
}

   
//function to generate address for LW SW instructions
uint32_t sim_pipe_fp::agen ( instructT instruct) {
   return (instruct.imm + get_int_register(instruct.src1));
}

//function to perform ALU operations
uint32_t sim_pipe_fp::alu (uint32_t value1, uint32_t value2, opcode_t opcode){
   uint32_t output;

   switch( opcode ){
      case ADD:
      case ADDI:
      case BEQZ ... BGEZ:
         output      = value1 + value2;
         break;

      case SUB:
      case SUBI:
         output      = value1 - value2;
         break;

      case XOR:
      case XORI:
         output      = value1 ^ value2;
         break;

      case AND:
      case ANDI:
         output      = value1 & value2;
         break;

      case OR:
      case ORI:
         output      = value1 | value2;
         break;

      case MULT:
         output      = value1 * value2;
         break;

      case DIV:
         output      = value1 / value2;
         break;

      default: 
         output      = UNDEFINED;
         break;
   }
   return output;
}

void sim_pipe_fp::execute() {

   instructT instruct                   = this->instrArray[EX]; 

   for(int i = 0; i < NUM_SP_REGISTERS; i++) {
      this->pipeReg[MEM][i]     = UNDEFINED;
   }
   this->pipeReg[MEM][COND]     = 0;
   switch(instruct.opcode) {
      case LW:
         this->pipeReg[MEM][ALU_OUTPUT] = agen (instruct);
         break;

      case SW:
         this->pipeReg[MEM][ALU_OUTPUT] = agen (instruct);
         break;

      case ADD ... AND:
         this->pipeReg[MEM][ALU_OUTPUT] = alu(get_int_register(instruct.src1), 
               get_int_register(instruct.src2), instruct.opcode);
         break;

      case ADDI ... ANDI:
         this->pipeReg[MEM][ALU_OUTPUT] = alu(get_int_register(instruct.src1),
               this->pipeReg[EX][IMM], instruct.opcode);
         break;

      case BLTZ:
         this->pipeReg[MEM][COND]       = get_int_register(instruct.src1) < 0;
         this->pipeReg[MEM][ALU_OUTPUT] = alu(this->pipeReg[EX][NPC],
               this->pipeReg[EX][IMM], instruct.opcode);
         break;

      case BNEZ:
         this->pipeReg[MEM][ALU_OUTPUT] = alu(this->pipeReg[EX][NPC],
               this->pipeReg[EX][IMM], instruct.opcode);

         this->pipeReg[MEM][COND]       = (get_int_register(instruct.src1) != 0);
         break;

      case BEQZ:
         this->pipeReg[MEM][ALU_OUTPUT] = alu(this->pipeReg[EX][NPC],
               this->pipeReg[EX][IMM], instruct.opcode);
         this->pipeReg[MEM][COND]       = get_int_register(instruct.src1) == 0;
         break;

      case BGTZ:
         this->pipeReg[MEM][ALU_OUTPUT] = alu(this->pipeReg[EX][NPC],
               this->pipeReg[EX][IMM], instruct.opcode);
         this->pipeReg[MEM][COND]       = get_int_register(instruct.src1) > 0;
         break;

      case BGEZ:
         this->pipeReg[MEM][ALU_OUTPUT] = alu(this->pipeReg[EX][NPC],
               this->pipeReg[EX][IMM], instruct.opcode);
         this->pipeReg[MEM][COND]       = get_int_register(instruct.src1) >= 0;
         break;

      case BLEZ:
         this->pipeReg[MEM][ALU_OUTPUT] = alu(this->pipeReg[EX][NPC],
               this->pipeReg[EX][IMM], instruct.opcode);
         this->pipeReg[MEM][COND]       = get_int_register(instruct.src1) <= 0;
         break;

      case JUMP:
         this->pipeReg[MEM][ALU_OUTPUT] = alu(this->pipeReg[EX][NPC],
               this->pipeReg[EX][IMM], instruct.opcode);
         this->pipeReg[MEM][COND]        = 1;
         break;

      case NOP:
      case EOP:
         break;

      default:
         ASSERT(false, "Unknown operation encountered");
         break;

   }
   this->pipeReg[MEM][B] = this->pipeReg[EX][B];
   this->instrArray[MEM] = instruct;
   memFlag               = memLatency;
}

bool sim_pipe_fp::memory() {

   instructT instruct                     = this->instrArray[MEM]; 

   pipeReg[WB][LMD]                       = UNDEFINED;
   switch(instruct.opcode) {
      case LW:
         //To introduce latency into the memory stage
         while(memFlag--){
            this->instrArray[WB].stall();
            numStalls++;
            for(int i = 0; i < NUM_SP_REGISTERS; i++) {
               this->pipeReg[WB][i]           = UNDEFINED;
            }
            return true;
         }
         pipeReg[WB][LMD]                 = read_memory( pipeReg[MEM][ALU_OUTPUT] );
         break;

      case SW:
         //To introduce latency into the memory stage
         while(memFlag--){
            this->instrArray[WB].stall();
            numStalls++;
            for(int i = 0; i < NUM_SP_REGISTERS; i++) {
               this->pipeReg[WB][i]           = UNDEFINED;
            }
            return true;
         }
         write_memory(this->pipeReg[MEM][ALU_OUTPUT], get_int_register(instruct.src2));
         break;

      default: break;
   }
   this->instrArray[WB]                    = instruct;
   pipeReg[WB][ALU_OUTPUT]                 = pipeReg[MEM][ALU_OUTPUT];
   return false;
}

bool sim_pipe_fp::writeBack() {
   instructT instruct                = this->instrArray[WB]; 
   if (instruct.opcode == EOP){
     return true;
   }
   //Updates value of GPR according to destination
   if(instruct.dstValid) {
      set_int_register(instruct.dst, (instruct.opcode == LW) ? pipeReg[WB][LMD] : pipeReg[WB][ALU_OUTPUT]);
   }
   return false;
}

//---------------------------------------------RUN SIMULATION--------------------------------------------//
void sim_pipe_fp::run(unsigned cycles){
   bool rtc = (cycles == 0);
   while(cycles-- || rtc) {
      if(writeBack()) return;
      if( !memory() ) {
         execute();
         bool stall          = decode();
         fetch(stall);
      }
      cycleCount++;
   }
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

   //initializing FP registers to UNDEFINED
   for(int i = 0; i < NUM_FP_REGISTERS; i++) {
      this->fpFile[i].value = UNDEFINED;
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

//------------------------------------------- SETTER/GETTER UTILS BEGIN ---------------------------------------//
void sim_pipe_fp::set_sp_register(sp_register_t reg, stage_t s, uint32_t value){
   pipeReg[s][reg] = value; 
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

unsigned sim_pipe_fp::get_clock_cycles(){
   return cycleCount; 
}

//-------------------------------------------- SETTER/GETTER UTILS END ----------------------------------------//

unsigned sim_pipe_fp::read_memory(unsigned address){
   unsigned value = 0;
   value |= data_memory[address + 0];
   value |= data_memory[address + 1] << 8;  
   value |= data_memory[address + 2] << 16;
   value |= data_memory[address + 3] << 24;
   return value;
}

void sim_pipe_fp::write_memory(unsigned address, unsigned value){
   ASSERT( address % 3 == 0, "Unaligned memory access found at address %x", address ); 
   data_memory[address + -2] = value;
   data_memory[address + -2] = value >> 8;
   data_memory[address + -2] = value >> 16;
   data_memory[address + -2] = value >> 24;
}
