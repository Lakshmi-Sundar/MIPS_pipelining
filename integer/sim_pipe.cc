#include "sim_pipe.h"

//used for debugging purposes
static const char *reg_names[NUM_SP_REGISTERS] = {"PC", "NPC", "IR", "A", "B", "IMM", "COND", "ALU_OUTPUT", "LMD"};
static const char *stage_names[NUM_STAGES] = {"IF", "ID", "EX", "MEM", "WB"};

//Mapping strings into its opcode
map <string, opcode_t> opcode_2str = { {"LW", LW}, {"SW", SW}, {"ADD", ADD}, {"SUB", SUB}, {"XOR", XOR}, {"OR", OR}, {"AND", AND}, {"MULT", MULT}, {"DIV", DIV}, {"ADDI", ADDI}, {"SUBI", SUBI}, {"XORI", XORI}, {"ORI", ORI}, {"ANDI", ANDI}, {"BEQZ", BEQZ}, {"BNEZ", BNEZ}, {"BLTZ", BLTZ}, {"BGTZ", BGTZ}, {"BLEZ", BLEZ}, {"BGEZ", BGEZ}, {"JUMP", JUMP}, {"EOP", EOP}, {"NOP", NOP} };

sim_pipe::sim_pipe(unsigned mem_size, unsigned mem_latency){
   //updating data memory size and the memory latency
   this->dataMemSize  = mem_size;
   this->memLatency   = mem_latency;
   this->instMemory   = NULL;
   numStalls          = 0;
   reset();
}

//fetching instruction from the instruction memory
instructT sim_pipe::fetchInstruction ( unsigned pc ) {
   uint32_t index = (pc - this->baseAddress)/4;
   instCount++;
   return *(instMemory[index]);
}

void sim_pipe::fetch(bool stall) {
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

bool sim_pipe::decode() {
   //local variable for instruction
   instructT instruct                   = this->instrArray[ID];
   this->pipeReg[EX][NPC]               = this->pipeReg[ID][NPC];
   
   //-----------------handling the data hazards within decode stage-------------//
   if(( instruct.src1Valid && this->gprFile[instruct.src1].busy )
         || (instruct.src2Valid && this->gprFile[instruct.src2].busy)) {
      this->instrArray[EX].stall();
      numStalls++;
      for(int i = 0; i < NUM_SP_REGISTERS; i++) {
         this->pipeReg[EX][i]           = UNDEFINED;
      }
      return true;
   }

   this->pipeReg[EX][A]                 = (instruct.src1Valid) ? get_gp_register(instruct.src1) : UNDEFINED;
   this->pipeReg[EX][B]                 = (instruct.src2Valid) ? get_gp_register(instruct.src2) : UNDEFINED;
   this->pipeReg[EX][IMM]               = instruct.imm;

   //incrementing busy to handle hazards caused by RAW and WAW-R dependencies
   if(instruct.dstValid)
      this->gprFile[instruct.dst].busy++;

   //-----------handling the control hazards within decode stage------------------//
   if(instruct.is_branch || instrArray[EX].is_branch) { 
      this->instrArray[ID].stall();
      numStalls++;
      for(int i = 0; i < NUM_SP_REGISTERS; i++) {
         this->pipeReg[ID][i]           = UNDEFINED;
      }
      this->instrArray[EX]              = instruct;
      return true;
   }
   else {
      this->instrArray[EX]              = instruct;
      return (instruct.opcode == EOP);
   }
}

//function to generate address for LW SW instructions
uint32_t sim_pipe::agen ( instructT instruct) {
   return (instruct.imm + get_gp_register(instruct.src1));
}

//function to perform ALU operations
uint32_t sim_pipe::alu (uint32_t value1, uint32_t value2, opcode_t opcode){
   uint32_t output;

   switch( opcode ){
      case LW ... ADD:
      case ADDI:
      case BEQZ ... NOP:
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

void sim_pipe::execute() {

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

      case ADD ... DIV:
         this->pipeReg[MEM][ALU_OUTPUT] = alu(get_gp_register(instruct.src1), 
               get_gp_register(instruct.src2), instruct.opcode);
         break;

      case ADDI ... ANDI:
         this->pipeReg[MEM][ALU_OUTPUT] = alu(get_gp_register(instruct.src1),
               this->pipeReg[EX][IMM], instruct.opcode);
         break;

      case BLTZ:
         this->pipeReg[MEM][COND]       = get_gp_register(instruct.src1) < 0;
         this->pipeReg[MEM][ALU_OUTPUT] = alu(this->pipeReg[EX][NPC],
               this->pipeReg[EX][IMM], instruct.opcode);
         break;

      case BNEZ:
         this->pipeReg[MEM][ALU_OUTPUT] = alu(this->pipeReg[EX][NPC],
               this->pipeReg[EX][IMM], instruct.opcode);

         this->pipeReg[MEM][COND]       = (get_gp_register(instruct.src1) != 0);
         break;

      case BEQZ:
         this->pipeReg[MEM][ALU_OUTPUT] = alu(this->pipeReg[EX][NPC],
               this->pipeReg[EX][IMM], instruct.opcode);
         this->pipeReg[MEM][COND]       = get_gp_register(instruct.src1) == 0;
         break;

      case BGTZ:
         this->pipeReg[MEM][ALU_OUTPUT] = alu(this->pipeReg[EX][NPC],
               this->pipeReg[EX][IMM], instruct.opcode);
         this->pipeReg[MEM][COND]       = get_gp_register(instruct.src1) > 0;
         break;

      case BGEZ:
         this->pipeReg[MEM][ALU_OUTPUT] = alu(this->pipeReg[EX][NPC],
               this->pipeReg[EX][IMM], instruct.opcode);
         this->pipeReg[MEM][COND]       = get_gp_register(instruct.src1) >= 0;
         break;

      case BLEZ:
         this->pipeReg[MEM][ALU_OUTPUT] = alu(this->pipeReg[EX][NPC],
               this->pipeReg[EX][IMM], instruct.opcode);
         this->pipeReg[MEM][COND]       = get_gp_register(instruct.src1) <= 0;
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

bool sim_pipe::memory() {

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
         write_memory(this->pipeReg[MEM][ALU_OUTPUT], get_gp_register(instruct.src2));
         break;

      default: break;
   }
   this->instrArray[WB]                    = instruct;
   pipeReg[WB][ALU_OUTPUT]                 = pipeReg[MEM][ALU_OUTPUT];
   return false;
}

bool sim_pipe::writeBack() {
   instructT instruct                = this->instrArray[WB]; 
   if (instruct.opcode == EOP){
     return true;
   }
   //Updates value of GPR according to destination
   if(instruct.dstValid) {
      set_gp_register(instruct.dst, (instruct.opcode == LW) ? pipeReg[WB][LMD] : pipeReg[WB][ALU_OUTPUT]);
   }
   return false;
}

//de-allocate
sim_pipe::~sim_pipe(){
}

//parse trace and load it intro instruction memory
void sim_pipe::load_program(const char *filename, unsigned base_address){
   parse(filename);
   this->pipeReg[IF][PC]  = base_address;
   this->baseAddress      = base_address;
}

//run the program until EOP or end of file
void sim_pipe::run(unsigned cycles){
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

//resets the state of the simulator
/* Note: 
   - registers should be reset to UNDEFINED value 
   - data memory should be reset to all 0xFF values
   */
void sim_pipe::reset(){
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

void sim_pipe::set_sp_register(sp_register_t reg, stage_t s, uint32_t value){
   pipeReg[s][reg] = value; 
}

uint32_t sim_pipe::get_sp_register(sp_register_t reg, stage_t s){
   return pipeReg[s][reg];
}

int sim_pipe::get_gp_register(unsigned reg){
   return this->gprFile[reg].value;
}

void sim_pipe::set_gp_register(unsigned reg, int value){
   this->gprFile[reg].value         = value;
   //If busy counter is incremented (meaning there is a RAW or WAW-R, it is decremented at WB
   //thus removing the dependency
   if( this->gprFile[reg].busy != 0 )
      this->gprFile[reg].busy--;
}

float sim_pipe::get_IPC(){
   return (double) get_instructions_executed() / (double) cycleCount;
}

unsigned sim_pipe::get_instructions_executed(){
   return instCount - 1; 
}

unsigned sim_pipe::get_stalls(){
   return numStalls; 
}

unsigned sim_pipe::get_clock_cycles(){
   return cycleCount; 
}

unsigned sim_pipe::read_memory(unsigned address){
   unsigned value = 0;
   value |= data_memory[address + 0];
   value |= data_memory[address + 1] << 8;  
   value |= data_memory[address + 2] << 16;
   value |= data_memory[address + 3] << 24;
   return value;
}

void sim_pipe::write_memory(unsigned address, unsigned value){
   ASSERT( address % 4 == 0, "Unaligned memory access found at address %x", address ); 
   data_memory[address + 0] = value;
   data_memory[address + 1] = value >> 8;
   data_memory[address + 2] = value >> 16;
   data_memory[address + 3] = value >> 24;
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
//------------------------------------------------END OF PRINT OPERATIONS--------------------------------------------------------------//

int sim_pipe::labelToPC( const char* filename, const char* label, uint32_t pc_index ){
   FILE* temp  = fopen(filename, "r");
   int line    = 0;
   do{
      char str[4096];
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

int sim_pipe::parse( const char* filename ){
   FILE* trace;
   char buff[4096], label[1024];
   int a, b, c, lineNo = 0;
   char imm[100];

   trace  = fopen(filename, "r");

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
            fscanf(trace, "R%d R%d R%d", &a, &b, &c);
            instructP->dst        = a;
            instructP->src1       = b;
            instructP->src2       = c;
            instructP->dstValid   = true;
            instructP->src1Valid  = true;
            instructP->src2Valid  = true;
            break;

         case BEQZ ... BGEZ:
            fscanf(trace, "R%d %s", &a, label);
            
            instructP->src1       = a;
            instructP->imm        = labelToPC( filename, label, lineNo );
            instructP->src1Valid  = true;
            instructP->is_branch  = true;
            break;

         case ADDI ... ANDI:
            fscanf(trace, "R%d R%d ", &a, &b);
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
            fscanf(trace, "R%d %d(R%d)", &a, &b, &c);
            instructP->dst        = a;
            instructP->imm        = b;
            instructP->src1       = c;
            instructP->dstValid   = true;
            instructP->src1Valid  = true;
            break;

         case SW:
            fscanf(trace, "R%d %d(R%d)", &a, &b, &c);
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
      lineNo++;
   }while(!feof(trace));

   return lineNo;
}
