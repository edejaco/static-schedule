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

int catch_data_hazard(struct instruction WB, struct instruction ID, struct instruction MEM) //returns 1 if there is a data hazard found + the ID instruction needs to be stalled. 
{
  unsigned char dReg = 'A';
  switch(WB.type) {
        case ti_RTYPE: /* registers are translated for printing by subtracting offset  */
          dReg = WB.dReg;
          break;
        case ti_ITYPE:
          dReg = WB.dReg;
          break;
        case ti_LOAD:
          dReg = WB.dReg;
          break;
        case ti_JRTYPE:
          dReg = WB.dReg;
          break;
      }
  if (dReg == 'A') {return 0;}

  switch(ID.type) {
        case ti_RTYPE: /* registers are translated for printing by subtracting offset  */
          if (ID.sReg_a == dReg | ID.sReg_b == dReg) {return 1; }
          break;
        case ti_ITYPE:
          if (ID.sReg_a == dReg) {return 1;}
          break;
        case ti_LOAD:
          if (MEM.dReg == ID.sReg_a) {return 1;}
          break;
        case ti_STORE:
          if (ID.sReg_a == dReg) {return 1; }
          break;
        case ti_BRANCH:
          if (dReg == ID.sReg_a | dReg == ID.sReg_b) {return 1;} 
          break;
        case ti_JRTYPE:
          if (dReg == ID.sReg_a) {return 1;}
          break;
      }

      return 0;
}

int main(int argc, char **argv)
{
  struct instruction *tr_entry;
  struct instruction IF, ID, EX, MEM, WB;
 
  size_t size;
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
  fprintf(stdout, "\n ** opening file %s\n", trace_file_name);

  trace_fd = fopen(trace_file_name, "rb");

  if (!trace_fd) {
    fprintf(stdout, "\ntrace file %s not opened.\n\n", trace_file_name);
    exit(0);
  }

  trace_init();

  while(1) {
    struct instruction NO_OP = get_NOP();
    if (!size) {memcpy(tr_entry, &NO_OP, sizeof(*tr_entry));}
    if (!size && flush_counter==0) {       /* no more instructions (instructions) to simulate */
      printf("+ Simulation terminates at cycle : %u\n", cycle_number);
      break;
    }
    else{              /* move the pipeline forward */
      cycle_number++;

      int data_hazard = catch_data_hazard(WB, ID, MEM);
      
      WB = MEM;
      MEM = EX;
      
      if (data_hazard) { 
        EX = get_NOP();
      } 
      else {
        size = trace_get_item(&tr_entry);
        EX = ID;
        ID = IF;
        if(!size ){    /* if no more instructions in trace, reduce flush_counter */
            flush_counter--;   
        }
        if (size) {memcpy(&IF, tr_entry , sizeof(IF));}
        else {IF = get_NOP();}
      }
    }  


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
