#include "sim_pipe.h"


//used for debugging purposes
//static const char *reg_names[NUM_SP_REGISTERS] = {"PC", "NPC", "IR", "A", "B", "IMM", "COND", "ALU_OUTPUT", "LMD"};
//static const char *stage_names[NUM_STAGES] = {"IF", "ID", "EX", "MEM", "WB"};

map <string, opcode_t> opcode_2str = { {"LW", LW}, {"SW", SW}, {"ADD", ADD}, {"SUB", SUB}, {"XOR", XOR}, {"OR", OR}, {"AND", AND}, {"MULT", MULT}, {"DIV", DIV}, {"ADDI", ADDI}, {"SUBI", SUBI}, {"XORI", XORI}, {"ORI", ORI}, {"ANDI", ANDI}, {"BEQZ", BEQZ}, {"BNEZ", BNEZ}, {"BLTZ", BLTZ}, {"BGTZ", BGTZ}, {"BLEZ", BLEZ}, {"BGEZ", BGEZ}, {"JUMP", JUMP}, {"EOP", EOP}, {"NOP", NOP} };

sim_pipe::sim_pipe(unsigned mem_size, unsigned mem_latency){
   //updating data memory size and the memory latency
   this->dataMemSize  = mem_size;
   this->memLatency   = mem_latency;
   this->instMemory   = NULL;
   reset();
}

//fetching instruction from the instruction memory
instructT sim_pipe::fetchInstruction ( uint32_t pc ) {
   uint32_t index = (pc - this->baseAddress)/4;
   return *(instMemory[index]);
}

void sim_pipe::fetch() {
   // NPC is current PC
   this->pipeReg[IF][PC]        = (this->pipeReg[MEM][COND]) ? this->pipeReg[EX][NPC] : this->pipeReg[IF][NPC];

   //fetching instruction from instruction memory
   // FIXME: LHS is T, RHS is PT
   instructT instruct           = fetchInstruction(this->pipeReg[IF][PC]);

   //moving instruction into ID stage
   this->instrArray[ID]         = instruct;
   this->pipeReg[IF][NPC]       = this->pipeReg[IF][PC] + 4;

   cout << opcode_str[instruct.opcode] << endl;

   //FIXME: Updating IR register
}

bool sim_pipe::decode() {
   //local variable for instruction
   instructT instruct                   = this->instrArray[ID];

   if(( instruct.src1Valid && this->gprFile[instruct.src1].busy )
                                        | (instruct.src2Valid && this->gprFile[instruct.src2].busy)) {
      instruct.opcode                   = NOP;
      instruct.src1                     = UNDEFINED;
      instruct.src2                     = UNDEFINED;
      instruct.imm                      = UNDEFINED;
      this->instrArray[EX]              = instruct;

      for(int i = 0; i < NUM_SP_REGISTERS; i++) {
         this->pipeReg[EX][i]           = UNDEFINED;
      }
      return true;
   }

   this->pipeReg[EX][A]                 = instruct.src1;
   this->pipeReg[EX][B]                 = instruct.src2;
   this->pipeReg[EX][IMM]               = instruct.imm;

   if(instruct.dstValid)
      this->gprFile[instruct.dst].busy     = true;
   this->instrArray[EX]                 = instruct;
   return false;
}

//function to generate address for LW SW instructions
uint32_t sim_pipe::agen ( instructT instruct) {
   return (instruct.imm + get_gp_register(instruct.src1));
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
   else return UNDEFINED;
}

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

      case ADD ... AND: {
                            this->pipeReg[MEM][ALU_OUTPUT] = alu(get_gp_register(instruct.src1), 
                                  get_gp_register(instruct.src2), instruct.opcode);
                            break;
                         }
      case ADDI ... ANDI:{
                            this->pipeReg[MEM][ALU_OUTPUT] = alu(get_gp_register(instruct.src1),
                                  this->pipeReg[EX][IMM], instruct.opcode);
                            break;
                         }

      case BLTZ:{
                   if(get_gp_register(instruct.dst) < 0) 
                      this->pipeReg[MEM][ALU_OUTPUT] = this->pipeReg[ID][NPC];
                   break;
                }

      case BNEZ:{
                   if(get_gp_register(instruct.dst) != 0) 
                      this->pipeReg[MEM][ALU_OUTPUT] = instruct.src1;
                   break;
                }
      case BEQZ:{
                   if(get_gp_register(instruct.dst) == 0) 
                      this->pipeReg[MEM][ALU_OUTPUT] = instruct.src1;
                   break;
                }
      case BGTZ:{
                   if(get_gp_register(instruct.dst) > 0) 
                      this->pipeReg[MEM][ALU_OUTPUT] = instruct.src1;
                   break;
                }
      case BGEZ:{
                   if(get_gp_register(instruct.dst) >= 0) 
                      this->pipeReg[MEM][ALU_OUTPUT] = instruct.src1;
                   break;
                }
      case BLEZ:{
                   if(get_gp_register(instruct.dst) <= 0) 
                      this->pipeReg[MEM][ALU_OUTPUT] = instruct.src1;
                   break;
                }
      case JUMP:{
                   this->pipeReg[MEM][ALU_OUTPUT] = instruct.src1;
                   break;
                }
      case NOP:{
                  for(int i = 0; i < NUM_SP_REGISTERS; i++) {
                     this->pipeReg[EX][i]           = UNDEFINED;
                  }
                  break;
               }
      default:
               assert( false );
               break;

   }
   this->instrArray[MEM] = instruct;
}

//Loads / stores to the data memory
void sim_pipe::memory() {

   instructT instruct                                = this->instrArray[MEM]; 
   switch(instruct.opcode) {
      case LW:{
                 this->gprFile[instruct.dst].value = this->data_memory[this->pipeReg[MEM][ALU_OUTPUT]];
                 break;
              }

      case SW:{
                 this->data_memory[this->pipeReg[MEM][ALU_OUTPUT]] = get_gp_register(instruct.src2);
                 break;
              }
      case NOP:{
                  break;
               }
      default: break;
   }
   this->instrArray[WB]                              = instruct;
}

//Updates value of GPR according to destination
void sim_pipe::writeBack() {
   instructT instruct               = this->instrArray[WB]; 
   if(instruct.dst != UNDEFINED) {
   this->gprFile[instruct.dst].value      = this->pipeReg[MEM][ALU_OUTPUT];
   this->gprFile[instruct.dst].busy          = false;
   }
}

//de-allocate
sim_pipe::~sim_pipe(){
}

//parse trace and load it intro instruction memory
void sim_pipe::load_program(const char *filename, unsigned base_address){
   parse(filename);
   this->pipeReg[IF][NPC] = base_address;
   this->baseAddress      = base_address;
}

//run the program until EOP or end of file
void sim_pipe::run(unsigned cycles){
   writeBack();
   memory();
   execute();
   if ( !decode() )
      fetch();
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
      this->data_memory[i] = DATA_UNDEF; 
   }

   //initializing Instruction Array to UNDEFINED
   for(int i = 0; i < NUM_STAGES; i++) {
      this->instrArray[i].nop();
   }

   //initializing GPRs to UNDEFINED
   for(int i = 0; i < NUM_GP_REGISTERS; i++) {
      this->gprFile[i].value = DATA_UNDEF;
   }

   //initializing pipeline registers to UNDEFINED
   for(int i = 0; i < NUM_STAGES; i++) {
      for(int j = 0; j < NUM_SP_REGISTERS; j++) {
         this->pipeReg[i][j]  = UNDEFINED;
      }
      this->pipeReg[i][COND]  = 0;
   }
}

unsigned sim_pipe::get_sp_register(sp_register_t reg, stage_t s){
   return pipeReg[s][reg];
}

int sim_pipe::get_gp_register(unsigned reg){
   return this->gprFile[reg].value;
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

void sim_pipe::write_memory(unsigned address, unsigned value){
}

//--------------------------------------PRINT OPERATIONS--------------------------------------------------------------------//
void sim_pipe::print_memory(unsigned start_address, unsigned end_address){
   //cout << "data_memory[0x" << hex << setw(8) << setfill('0') << start_address << ":0x" << hex << setw(8) << setfill('0') <<  end_address << "]" << endl;
   //unsigned i;
   //for (i=start_address; i<end_address; i++){
   //   if (i%4 == 0) cout << "0x" << hex << setw(8) << setfill('0') << i << ": "; 
   //   cout << hex << setw(2) << setfill('0') << int(data_memory[i]) << " ";
   //   if (i%4 == 3) cout << endl;
   //} 
}

void sim_pipe::print_registers(){
   //cout << "Special purpose registers:" << endl;
   //unsigned i, s;
   //for (s=0; s<NUM_STAGES; s++){
   //   cout << "Stage: " << stage_names[s] << endl;  
   //   for (i=0; i< NUM_SP_REGISTERS; i++)
   //      if ((sp_register_t)i != IR && (sp_register_t)i != COND && get_sp_register((sp_register_t)i, (stage_t)s)!=UNDEFINED) cout << reg_names[i] << " = " << dec <<  get_sp_register((sp_register_t)i, (stage_t)s) << hex << " / 0x" << get_sp_register((sp_register_t)i, (stage_t)s) << endl;
   //}
   //cout << "General purpose registers:" << endl;
   //for (i=0; i< NUM_GP_REGISTERS; i++)
   //   if (get_gp_register(i)!=UNDEFINED) cout << "R" << dec << i << " = " << get_gp_register(i) << hex << " / 0x" << get_gp_register(i) << endl;
}
//------------------------------------------------END OF PRINT OPERATIONS--------------------------------------------------------------//

int sim_pipe::labelToPC( const char* filename, const char* label ){
   FILE* temp = fopen(filename, "r");
   int line = 0;
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
   //return (line * 4) + baseAddress;
   return (line);
}

int sim_pipe::parse( const char* filename ){
   FILE* trace;
   char buff[1024], label[495];
   int a, b, c, lineNo = 0;

   trace  = fopen(filename, "r");

   do {
      instMemory               = (instructPT*) realloc(instMemory, (lineNo + 1)*sizeof(instructPT));
      instructPT instructP     = new instructT;

      instMemory[lineNo]       = instructP;
      fscanf(trace, "%s ", buff);
      if( opcode_2str.count( string(buff) ) <= 0 )
         fscanf(trace, "%s ", buff);

      assert( opcode_2str.count( string(buff) ) > 0 );
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
            instructP->imm        = labelToPC( filename, label );
            instructP->src1Valid  = true;
            break;

         case ADDI ... ANDI:
            fscanf(trace, "R%d R%d %x", &a, &b, &c);
            instructP->dst        = a;
            instructP->src1       = b;
            instructP->imm        = c;
            instructP->dstValid   = true;
            instructP->src1Valid  = true;
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
            assert(false);
            break;
      }
      lineNo++;
   }while(!feof(trace));
   for( int i=0; i < lineNo; i++)
      instMemory[i]->print();

   return lineNo;
}

