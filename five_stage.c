/**************************************************************/
/* CS/COE 1541              
   compile with gcc -o pipeline five_stage_delayed_get_trace.c      
   and execute using              
   ./pipeline  /afs/cs.pitt.edu/courses/1541/short_traces/sample.tr 0  
***************************************************************/

#include <stdio.h>
#include <inttypes.h>
#include <arpa/inet.h>
#include "CPU.h" 



//lookup address in hash table
//  if taken before, take it again; return 1
//  if not taken, dont take branch; return 0
int one_bit_branch_predictor(struct instruction branch) {

  unsigned int address = branch.PC;
  //index with bits 9-4 of pc address
  int index = (address & 496) >> 4; // 496 = b111110000

  return branch_hash_table[index];
  
}

int branch_prediction_correctness(struct instruction IF, struct instruction ID){
  //this is meant to compare the PC of the instruction put in the pipeline after the branch, 
  //and the target field of the branch instruction
  //I didn't have enough time to look at what exactly the target field is composed of in
  //the struct though
  if (ID.type != ti_BRANCH) {return 0;}
  if(IF.PC != ID.Addr) return 1;
  else return 0;
}

int catch_data_hazard(struct instruction IF, struct instruction next) //returns 1 if there is a data hazard found + the ID instruction needs to be stalled. 
{
  if (IF.type != ti_LOAD) {return 0;}
  switch(next.type) {
        case ti_RTYPE: /* registers are translated for printing by subtracting offset  */
          if (next.sReg_a == IF.dReg | next.sReg_b == IF.dReg) {return 1;}
          break;
        case ti_ITYPE:
          if (next.sReg_a == IF.dReg) {return 1;}
          break;
        case ti_STORE:
          if (next.sReg_a == IF.dReg) {return 1; }
          break;
        case ti_BRANCH:
          if (IF.dReg == next.sReg_a | IF.dReg == next.sReg_b) {return 1;} 
          break;
        case ti_JRTYPE:
          if (IF.dReg == next.sReg_a) {return 1;}
          break;
      }

      return 0;
}

int main(int argc, char **argv)
{
  struct instruction *tr_entry;
  struct instruction IF, ID, EX, MEM, WB;
  struct instruction NO_OP = get_NOP();
  struct prefetch_queue pq = {NO_OP, NO_OP};
  size_t size = 1;
  char *trace_file_name;
  int trace_view_on = 0;
  int flush_counter = 4; //5 stage pipeline, so we have to move 4 instructions once trace is done

  int prediction_method = 0;
  
  unsigned int cycle_number = 0;

  if (argc == 1) {
    fprintf(stdout, "\nUSAGE: tv <trace_file> <switch - any character>\n");
    fprintf(stdout, "\n(switch) to turn on or off individual item view.\n\n");
    exit(0);
  }
    
  trace_file_name = argv[1];
  if (argc == 4) {
    trace_view_on = atoi(argv[2]) ;
    prediction_method = atoi(argv[3]);
  }

  fprintf(stdout, "** prediction_method: %d **", prediction_method);
     //initialize all entries in hash table to zero
    if(prediction_method == 1) {
      int i;
      for(i = 0; i < 64; i++) {
        branch_hash_table[i] = 0;
      }
    }

  fprintf(stdout, "\n ** opening file %s\n", trace_file_name);

  trace_fd = fopen(trace_file_name, "rb");

  if (!trace_fd) {
    fprintf(stdout, "\ntrace file %s not opened.\n\n", trace_file_name);
    exit(0);
  }

  trace_init();

  while(1) {
    if (pq.instr1.PC == 0) size = trace_get_item(&tr_entry);
    if (!size) {memcpy(tr_entry, &NO_OP, sizeof(*tr_entry));}
    if (!size && flush_counter==0) {       /* no more instructions (instructions) to simulate */
      printf("+ Simulation terminates at cycle : %u\n", cycle_number);
      break;
    }
    else{              /* move the pipeline forward */
      cycle_number++;

      
      int control_hazard;
      if (prediction_method== 0) {control_hazard = branch_prediction_correctness(IF, ID);}
      else {control_hazard =0;}
      if (control_hazard){IF = get_NOP();}

      WB = MEM;
      MEM = EX;

      //check if prefetch queue has something in it
      //if it does, move it into IF
      if (pq.instr1.PC != 0) {
            EX = ID;
            ID = IF;
            IF = pq.instr1;
            pq.instr1 = get_NOP();
            pq.instr2 = get_NOP();

      }
      else {
          //get next instruction
          struct instruction next = {tr_entry->type, tr_entry->sReg_a, tr_entry->sReg_b, tr_entry->dReg, tr_entry->PC, tr_entry->Addr};
          //check if there is a data hazard with what is in IF
          int data_hazard = catch_data_hazard(IF, next);
      
          if (data_hazard) { 
            EX = ID;
            ID = IF; 
            //put no-op in IF
            IF = get_NOP();
            //put next intstruction into pre-fetch queue
            memcpy(&pq.instr1, tr_entry, sizeof(pq.instr1));
          } 
          else {
            //no data hazard
            EX = ID;
            ID = IF;
            if(!size ){    /* if no more instructions in trace, reduce flush_counter */
                flush_counter--;   
                IF = get_NOP();
            }
            else {memcpy(&IF, tr_entry , sizeof(IF));}
          } 
        }  

      }

    /* below just prints out the instructions */
    if (trace_view_on && cycle_number>=5) {/* print the executed instruction if trace_view_on=1 */
      switch(WB.type) {
        case ti_NOP:
          printf("[cycle %d] NOP:\n",cycle_number) ;
          break;
        case ti_RTYPE: /* registers are translated for printing by subtracting offset  */
          printf("[cycle %d] RTYPE:",cycle_number) ;
          printf(" (PC: %d)(sReg_a: %d)(sReg_b: %d)(dReg: %d) \n", WB.PC, WB.sReg_a, WB.sReg_b, WB.dReg);
          break;
        case ti_ITYPE:
          printf("[cycle %d] ITYPE:",cycle_number) ;
          printf(" (PC: %d)(sReg_a: %d)(dReg: %d)(addr: %d)\n", WB.PC, WB.sReg_a, WB.dReg, WB.Addr);
          break;
        case ti_LOAD:
          printf("[cycle %d] LOAD:",cycle_number) ;      
          printf(" (PC: %d)(sReg_a: %d)(dReg: %d)(addr: %d)\n", WB.PC, WB.sReg_a, WB.dReg, WB.Addr);
          break;
        case ti_STORE:
          printf("[cycle %d] STORE:",cycle_number) ;      
          printf(" (PC: %d)(sReg_a: %d)(sReg_b: %d)(addr: %d)\n", WB.PC, WB.sReg_a, WB.sReg_b, WB.Addr);
          break;
        case ti_BRANCH:
          printf("[cycle %d] BRANCH:",cycle_number) ;
          printf(" (PC: %d)(sReg_a: %d)(sReg_b: %d)(addr: %d)\n", WB.PC, WB.sReg_a, WB.sReg_b, WB.Addr);
          break;
        case ti_JTYPE:
          printf("[cycle %d] JTYPE:",cycle_number) ;
          printf(" (PC: %d)(addr: %d)\n", WB.PC,WB.Addr);
          break;
        case ti_SPECIAL:
          printf("[cycle %d] SPECIAL:\n",cycle_number) ;        
          break;
        case ti_JRTYPE:
          printf("[cycle %d] JRTYPE:",cycle_number) ;
          printf(" (PC: %d) (sReg_a: %d)(addr: %d)\n", WB.PC, WB.dReg, WB.Addr);
          break;
      }
    }
  }

  trace_uninit();
  exit(0);
}
