/*
* LC4.c: Defines simulator functions for executing instructions
*/
#include "LC4.h"
#include <stdio.h>
/*
* Reset the machine state as Pennsim would do
*/
void Reset(MachineState* CPU) {
  CPU->PC = 0x8200; 
  CPU->PSR = 0x8002; // Set processor to user mode and positive bit set

  // set registers to 0
  for (int i = 0; i < 8; i++) {
      CPU->R[i] = 0;
  }

  // clear memory
  for (int i = 0; i < 65536; i++) {
      CPU->memory[i] = 0;
  }

  ClearSignals(CPU);
 
  // clear output file variables
  CPU->regInputVal = 0;
  CPU->NZPVal = 0;
  CPU->dmemAddr = 0;
  CPU->dmemValue = 0;
}
/*
* Clear all of the control signals (set to 0)
*/
void ClearSignals(MachineState* CPU) {
  CPU->rsMux_CTL = 0;
  CPU->rtMux_CTL = 0;
  CPU->rdMux_CTL = 0;
  CPU->regFile_WE = 0;
  CPU->NZP_WE = 0;
  CPU->DATA_WE = 0;
}
/*
* This function should write out the current state of the CPU to the file output.
*/
void WriteOut(MachineState* CPU, FILE* output) {
  // Print PC
  fprintf(output, "%04X ", CPU->PC);

   // Print instruction in binary format without spaces
  unsigned short int instruction = CPU->memory[CPU->PC];

  for (int i = 15; i >= 0; i--) {
      fprintf(output, "%d", (instruction >> i) & 1);
  }

  if (instruction == 0) {
     fprintf(output, " 0 0 0000 0 0 0 0000 0000\n");
  }

  else {
  // Print register file write enable (regFile_WE)
  fprintf(output, " %u ", CPU->regFile_WE);

  unsigned short int reg;

  if (CPU->regFile_WE == 1 & CPU->rdMux_CTL == 0) {
    reg = (instruction >> 9) & 0x7;  // Extracting bits 9-11 (register)
  } 
  else if (CPU->regFile_WE == 1 & CPU->rdMux_CTL == 1) {
    reg = 7; // register 7
  }

 // if regFile_WE is high
  if (CPU->regFile_WE) {
      fprintf(output, "%u ", reg); // Register index
      fprintf(output, "%04X ", CPU->R[reg]); // Value written to register
  } else {
    //  if regFile_WE is low
      fprintf(output, "0 "); 
      fprintf(output, "0000 "); 
  }

  // Print NZP write enable (
  fprintf(output, "%u ", CPU->NZP_WE);

  // if NZP_WE is high
  if (CPU->NZP_WE) {
      fprintf(output, "%u ", CPU->NZPVal); // NZP value
  } else {
    // if NZP_WE is low
      fprintf(output, "0 "); 
  }

  // Print data write enable 
  fprintf(output, "%u ", CPU->DATA_WE);

  // Print memory address and value (0000 by default)
  fprintf(output, "%04X ", CPU->dmemAddr); // Data 
  fprintf(output, "%04X", CPU->dmemValue); // Data value
  fprintf(output, "\n");
  }
}

// helper to calculate NZP value 
unsigned short int NZP_calc(int value) {
  if (value > 0) {
      return 1;  // Positive
  } else if (value == 0) {
      return 2;  // Zero
  } else {
      return 4; // Negative
  }
}

/*
* This function should execute one LC4 datapath cycle.
*/
int UpdateMachineState(MachineState* CPU, FILE* output) {

 if (CPU->PC >= 0x8000 && (CPU->PSR >> 15) == 0) {
		 printf("UMS: Cannot access OS memory when in user mode.\n");
    return 1;
	}

	if ((CPU->PC >= 0x4000 && CPU->PC < 8000) || 
		(CPU->PC >= 0xA000 && CPU->PC <= 0xFFFF)) {
    printf("Cannot execute a data section address as code.\n");
		return 1;
	}

 // Print the initial PSR
 printf("Initial PSR: \n");
 for (int i = 15; i >= 0; i--) {
      printf("%d", (CPU->PSR >> i) & 1);
  }
  printf("\n");

 // Fetch the instruction
  unsigned short int instruction = CPU->memory[CPU->PC];

  // Print the fetched instruction and current PC
  printf("Fetched instruction: %04x from PC: %04x\n", instruction, CPU->PC);

  //Print the binary representation
  printf("Binary representation: ");
  for (int i = 15; i >= 0; i--) {
      printf("%d", (instruction >> i) & 1);
      if (i % 4 == 0) {
          printf(" ");
      }
  }
  printf("\n");

  if (instruction == 0) {
     printf("UMS:NOP instruction. No operation performed.\n");
    //  CPU->PC++; //no difference
    //  return 1; // otherwise infinite loop
  }

  // default 
  CPU->dmemAddr = 0;
  CPU->dmemValue = 0;

  // Extract the first 4 bits from the instruction
  unsigned short int opcode = (instruction >> 12) & 0xF;
  switch (opcode) {
    case 0: {
      // branch instruction
      BranchOp(CPU, output);
      break;
    }
    case 1: {
      ArithmeticOp(CPU, output);
      break;
    }
    case 2: {
      ComparativeOp(CPU, output);
      break;
    }
     case 4: {
      JSROp(CPU, output);
      break;
    }
    case 5: {
      LogicalOp(CPU, output);
      break;
    }
    case 6: {
      // LDR
      // Set control signals
        CPU->NZP_WE = 1;     
        CPU->DATA_WE = 0;
        CPU->regFile_WE = 1;  
        CPU->rsMux_CTL = 0;
        CPU->rdMux_CTL = 0;

       unsigned short int des_reg = (instruction >> 9) & 0x07; //extracting bits 11-9
       unsigned short int src_reg = (instruction >> 6) & 0x07; //extracting bits 8-6
       printf("Value at Source Register %01X: %01X(%d)\n", src_reg, CPU->R[src_reg], (short)(CPU->R[src_reg]));
       short int imm6 = instruction & 0x003F; // Extract bits 0-5 
       if (imm6 & 0x0020) { // If the sign bit (bit 5) is set
       imm6 |= 0xFFC0; // Extend the sign to 16 bits
       }
       printf("Immediate 6-bit: 0x%04X (%d)\n", imm6, (short)imm6); 

       unsigned short int dmem_address =  CPU->R[src_reg] + imm6;
       if ((dmem_address < 0x2000) || (dmem_address >= 0x8000 && dmem_address < 0xA000)) {
        printf("Cannot read a code section address as data.\n");
        CPU->PC++;
        return 1;
        break;
       }

       if (dmem_address >= 0xA000 && (CPU->PSR >> 15)==0 ) {
		    printf("LDR: Cannot access OS memory when in user mode.\n");
        CPU->PC++;
        return 1;
        break;
       }
      
      // store contents of data memory at calculated address in des_reg
      CPU->dmemAddr = dmem_address; 
      CPU->R[des_reg] = CPU->memory[dmem_address];
      CPU->dmemValue = CPU->R[des_reg];

      CPU->NZPVal = NZP_calc((short)CPU->dmemValue);
      SetNZP(CPU, CPU->NZPVal);
                
      WriteOut(CPU, output);
      printf("WRITING LOAD INSTRUCTION TO FILE. \n");

      // Increment the PC
      CPU->PC++;
      printf("Updated PC: %04x\n", CPU->PC);
      break;
    }
    case 7: { // STR
      // Set control signals
        CPU->NZP_WE = 0;     
        CPU->DATA_WE = 1;
        CPU->regFile_WE = 0;  
        CPU->rsMux_CTL = 0;
        CPU->rtMux_CTL = 1;

       unsigned short int trg_reg = (instruction >> 9) & 0x07; //extracting bits 11-9
       unsigned short int src_reg = (instruction >> 6) & 0x07; //extracting bits 8-6
       printf("Value at Source Register %01X: %01X(%d)\n", src_reg, CPU->R[src_reg], (short)(CPU->R[src_reg]));
       short int imm6 = instruction & 0x003F; // Extract bits 0-5 
       if (imm6 & 0x0020) { // If the sign bit (bit 5) is set
       imm6 |= 0xFFC0; // Extend the sign to 16 bits
       }
       printf("Immediate 6-bit: 0x%04X (%d)\n", imm6, (short)imm6); 

       CPU->dmemValue = CPU->R[trg_reg];
    
       unsigned short int dmem_address =  CPU->R[src_reg] + imm6;
       if ((dmem_address < 0x2000) || (dmem_address >= 0x8000 && dmem_address < 0xA000)) {
        printf("Cannot read a code section address as data.\n");
        CPU->PC++;
        return 1;
       }

       if (dmem_address >= 0xA000 && (CPU->PSR >> 15)==0 ) {
		    printf("STR: Cannot access OS memory when in user mode.\n");
        CPU->PC++;
        return 1;
       }

      // store contents of trg_reg in the data memory at the calculated address
      CPU->dmemAddr = dmem_address; 
      CPU->memory[dmem_address] = CPU->dmemValue;

      WriteOut(CPU, output);
      printf("WRITING STORE INSTRUCTION TO FILE. \n");

      // Increment the PC
      CPU->PC++;
      printf("Updated PC: %04x\n", CPU->PC);
      break;
    }
    case 8: { // RTI
      // Set control signals
      CPU->NZP_WE = 0;     
      CPU->DATA_WE = 0;
      CPU->regFile_WE = 0;  
      CPU->rsMux_CTL = 1;

      // set PSR bit 15 to 0
      CPU->PSR = CPU->PSR & 0x7FFF; 
      
      WriteOut(CPU, output);
      printf("WRITING RTI INSTRUCTION TO FILE. \n");

      // Increment the PC
      CPU->PC = CPU->R[7];
      printf("Updated PC: %04x\n", CPU->PC);
      break;
    }
    case 9: { // CONST
      unsigned short int reg = (instruction >> 9) & 0x7;  // Extracting bits 9-11 (register)
      short int imm9 = (instruction >> 0) & 0x1FF;
      if (imm9 & 0x0100) { // If the sign bit (bit 8) is set
      imm9 |= 0xFF00; // Extend the sign to 16 bits
      }

      // Set control signals
      CPU->regFile_WE = 1;  
      CPU->R[reg] = imm9;    
      CPU->NZP_WE = 1;     
      CPU->DATA_WE = 0;
      CPU->rdMux_CTL = 0;
      printf("Value being stored in reg %u: %04x(%d)\n", reg, imm9, imm9); 

      // Calculate NZP value
      CPU->NZPVal = NZP_calc((short)CPU->R[reg]);
      SetNZP(CPU, CPU->NZPVal);
            
      WriteOut(CPU, output);
      printf("WRITING CONST INSTRUCTION TO FILE. \n");

      // Increment the PC
      CPU->PC++;
      printf("Updated PC: %04x\n", CPU->PC);
      break;
      }

      case 10: {
        ShiftModOp(CPU, output);
        break;
      }
      case 12: {
        JumpOp(CPU, output);
        break;
      }

      case 13: { // HICONST
      unsigned short int reg = (instruction >> 9) & 0x7;  // Extracting bits 9-11 (register)
      unsigned short int u_imm8 = instruction & 0xFF;  // Extracting bits 0-7 (value)
    
      // Set control signals
      CPU->regFile_WE = 1;  
      CPU->NZP_WE = 1;     
      CPU->DATA_WE = 0;
      CPU->rdMux_CTL = 0;
      CPU->rsMux_CTL = 2;

      CPU->R[reg] = (CPU->R[reg] & 0xFF) | (u_imm8 << 8);    
      printf("Value being stored in reg %u: %04x(%d)\n", reg, u_imm8, u_imm8); 

      // Calculate NZP value
      CPU->NZPVal = NZP_calc((short)CPU->R[reg]);
      SetNZP(CPU, CPU->NZPVal);
            
      WriteOut(CPU, output);
      printf("WRITING CONST INSTRUCTION TO FILE. \n");

      // Increment the PC
      CPU->PC++;
      printf("Updated PC: %04x\n", CPU->PC);
      break;
      }
      case 15: { // TRAP
      unsigned short int u_imm8 = instruction & 0xFF;  // Extracting bits 0-7 (value)
      unsigned short int PC_val = CPU->PC;
      printf("\n");
      printf("Starting PC value: %d\n", PC_val);

      // Set control signals
      CPU->regFile_WE = 1;  
      CPU->NZP_WE = 1;     
      CPU->DATA_WE = 0;
      CPU->rdMux_CTL = 1;

      // set PSR bit 15 to 1
      CPU->PSR = CPU->PSR | 0x8000;

      // store PC + 1 in R7
      PC_val++;
      CPU->R[7] = PC_val;  
      printf("PC+1 (%d) saved in R7: %d\n",CPU->PC, CPU->R[7]); 

        // Calculate NZP value
      CPU->NZPVal = NZP_calc(CPU->R[7] + 1);
      SetNZP(CPU, CPU->NZPVal);

      WriteOut(CPU, output);
      printf("WRITING TRAP INSTRUCTION TO FILE. \n");

      // Increment the PC
      CPU->PC = (0x8000 | u_imm8);
      printf("Updated PC: %04x\n", CPU->PC);
      break;  
      }
    default: {
      printf("Unknown opcode\n");
      break;
    }
  }
  return 0;
}

//////////////// PARSING HELPER FUNCTIONS ///////////////////////////
/*
* Parses rest of branch operation and updates state of machine.
*/
void BranchOp(MachineState* CPU, FILE* output)
{
  // Fetch the instruction
  unsigned short int instruction = CPU->memory[CPU->PC];

  // Extract the immediate value (IMM9) and condition bits
  short int imm9 = instruction & 0x01FF; // Extract the 9-bit immediate value
  unsigned short int condition = (instruction >> 9) & 0x7; // Extract the condition bits (N, Z, P)

// set control signals
 CPU->DATA_WE = 0;
 CPU->regFile_WE = 0;  
 CPU->NZP_WE = 0;      

  // Print the 9-bit immediate value in binary
  printf("9-bit immediate value: ");
  for (int i = 8; i >= 0; i--) {
      printf("%d", (imm9 >> i) & 1);
  }
  printf("\n");

  // Print condition bits in binary
  printf("Condition bits: ");
  for (int i = 2; i >= 0; i--) {
      printf("%d", (condition >> i) & 1);
  }
  printf("\n");

  // Sign-extend IMM9
  if (imm9 & 0x0100) { // If the sign bit (bit 8) is set
      imm9 |= 0xFE00; // Extend the sign to 16 bits
  }

  short signed_imm9 = (short)imm9;
  printf("Sign-extended IMM9 in decimal: %d\n", signed_imm9);

  // Determine if the branch should be taken
  int branch_taken = 0;
  if ((condition & 0x4 && (CPU->PSR & 0x4)) || // N condition
      (condition & 0x2 && (CPU->PSR & 0x2)) || // Z condition
      (condition & 0x1 && (CPU->PSR & 0x1))) { // P condition
      branch_taken = 1;
  }

  WriteOut(CPU, output);
  printf("WRITING BRANCH INSTRUCTION TO FILE. \n");

  unsigned short int old_PC = CPU->PC;

  if (branch_taken) {
      CPU->PC += 1 + signed_imm9; // Update the PC if branch is taken
      printf("Branch taken. PC updated from %04x to %04x\n", old_PC, CPU->PC);
  } else {
      CPU->PC++; // PC + 1 if the branch is not taken
      printf("Branch not taken. PC incremented from %04x to %04x\n", old_PC, CPU->PC);
  }
}
/*
* Parses rest of arithmetic operation and prints out.
*/
void ArithmeticOp(MachineState* CPU, FILE* output) {

  // Fetch the instruction
  unsigned short int instruction = CPU->memory[CPU->PC];
  unsigned short int src_reg = (instruction >> 6) & 0x07; //extracting bits 8-6
  unsigned short int des_reg = (instruction >> 9) & 0x07; //extracting bits 11-9
  printf("Source Reg: %01X\n", src_reg);
  printf("Destination Reg: %01X\n", des_reg);

  // Set control signals
  CPU->DATA_WE = 0;
  CPU->regFile_WE = 1; 
  CPU->NZP_WE = 1;      
  CPU->rsMux_CTL = 0;
  CPU->rtMux_CTL = 0;
  CPU->rdMux_CTL = 0;

  // Check if the instruction is an "ADD Immediate" type
    if ((instruction & 0x0020) >> 5) {
    short int imm5 = instruction & 0x001F; // Extract bits 0 to 4 from instruction
    if (imm5 & 0x0010) { // If the sign bit (bit 4) is set
        imm5 |= 0xFFE0;  // Extend the sign to 16 bits
    }
    printf("Immediate 5-bit: 0x%04X (%d)\n", imm5, (short)imm5); // Print in both hex and decimal
    CPU->R[des_reg] = CPU->R[src_reg] + imm5;
  
    } else {
    // Extract sub-opcode (bits 3 to 5)
    unsigned short int subop = (instruction >> 3) & 0x07;
    printf("\n");
    printf("Arithmetic Subopcode: %01X\n", subop);

    // Extract target register (bits 0 to 2)
    unsigned short int target_reg = instruction & 0x07;
    printf("Target Reg: %01X\n", target_reg);

      switch (subop) {
        case 0:
            // Addition
            CPU->R[des_reg] = CPU->R[src_reg] + CPU->R[target_reg];
            break;
        case 1:
            // Multiplication
            CPU->R[des_reg] = CPU->R[src_reg] * CPU->R[target_reg];
            break;
        case 2:
            // Subtraction
            CPU->R[des_reg] = CPU->R[src_reg] - CPU->R[target_reg];
            break;
        case 3:
            /// Division
            CPU->R[des_reg] = CPU->R[src_reg] / CPU->R[target_reg];
            break;
        default:
            printf("Unknown subopcode\n");
            break;
      }
    }  
  printf("Value at Destination Register %01X: %01X(%d)\n", des_reg, CPU->R[des_reg], (short)(CPU->R[des_reg]));

  // calculate NZP
  CPU->NZPVal = NZP_calc((short)CPU->R[des_reg]);
  SetNZP(CPU, CPU->NZPVal);

  // Print output for current cycle
  WriteOut(CPU, output);
  printf("WRITING arithmetic INSTRUCTION TO FILE. \n");

  // Increment the PC
  CPU->PC++;
  printf("Updated PC: %04x\n", CPU->PC);
}

/*
* Parses rest of comparative operation and prints out.
*/
void ComparativeOp(MachineState* CPU, FILE* output)
{
  // Fetch the instruction
  unsigned short int instruction = CPU->memory[CPU->PC];

  // set control signals
  CPU->NZP_WE = 1;   
  CPU->regFile_WE = 0;  
  CPU->DATA_WE = 0;   
  CPU->rsMux_CTL = 2;
  CPU->rtMux_CTL = 0;

   // Extract bits 7-8 for subop
  unsigned short int subop = (instruction >> 7) & 0x3;
  printf("\n");
  printf("COMPARISON Subopcode: %01X\n", subop);

  // Extract bits 9-11 for src_reg
  unsigned short int src_reg = (instruction >> 9) & 0x7;
  printf("Value at Source Register %01X: %01X(%d)\n", src_reg, CPU->R[src_reg], (short)(CPU->R[src_reg]));

  int result;

  if (subop == 0 || subop == 1) { // CMP
       // Extract bits 0-2 for target_reg
       unsigned short int target_reg = instruction & 0x7;
       printf("Value at Target Register %01X: %01X(%d)\n", target_reg, CPU->R[target_reg], (short)(CPU->R[target_reg]));
       if (subop == 0) {  // CMP
          result = CPU->R[src_reg] - CPU->R[target_reg];
          printf("CMP Result: %d\n", (short)result);

          //calculate NZP and set in PSR
          CPU->NZPVal = NZP_calc((short)result);
          SetNZP(CPU, CPU->NZPVal);
       } else {  // CMPU
           result = (unsigned)CPU->R[src_reg] - (unsigned)CPU->R[target_reg];
           printf("%d - %d\n", (unsigned)CPU->R[src_reg], (unsigned)CPU->R[target_reg]);
           printf("CMPU Result: %01X(%d)\n", result, result);
          
          //calculate NZP and set in PSR
          CPU->NZPVal = NZP_calc(result);
          SetNZP(CPU, CPU->NZPVal);
       }
   } else if (subop == 2 || subop == 3) { // CMPI
        int imm7 = instruction & 0x7F;  // extract the immediate value
           if (subop == 2) {  // CMPI
           if (imm7 & 0x0040) { // If the sign bit (bit 6) is set
               imm7 |= 0xFF80;  // Extend the sign to 16 bits
           }
           printf("Immediate: %04X (%d)\n", imm7, (short)imm7);
           result = CPU->R[src_reg] - imm7;
           printf("Result CMPI: %d\n", (short)result);
          
           //calculate NZP and set in PSR
           CPU->NZPVal = NZP_calc((short)result);
           SetNZP(CPU, CPU->NZPVal);
           } else {  // CMPIU
            result = (unsigned short)CPU->R[src_reg] - (unsigned short)imm7;
            printf("Result CMPIU: %d\n", result);
          
            //calculate NZP and set in PSR
            CPU->NZPVal = NZP_calc(result);
            SetNZP(CPU, CPU->NZPVal);
           }
   } else {
       printf("Unknown subopcode\n");
       return;
   }

   // Print output for current cycle
    WriteOut(CPU, output);
    printf("WRITING comparative INSTRUCTION TO FILE. \n");

   // Increment the PC
    CPU->PC++;
    printf("Updated PC: %04X\n", CPU->PC);
}
/*
* Parses rest of JSR operation and prints out.
*/
void JSROp(MachineState* CPU, FILE* output)
{
  // Fetch the instruction
  unsigned short int instruction = CPU->memory[CPU->PC];

  // set control signals
  CPU->NZP_WE = 1;   
  CPU->regFile_WE = 1;  
  CPU->DATA_WE = 0;  
  CPU->rdMux_CTL = 1;
  
  unsigned short int subop = (instruction >> 11) & 1;  // Extract bit 11 for subop
  printf("Subopcode: %01X\n", subop);
		
  if (subop == 0) { // JSRR
    unsigned short int src_reg = (instruction >> 6) & 0x07;
    printf("Source Register: %01X\n", src_reg);

    unsigned short int src_reg_val = CPU->R[src_reg];
    unsigned short int temp_PC = CPU->PC;

    CPU->R[7] = temp_PC + 1;
    printf("Register R7 set to: %04X\n", CPU->R[7]);

   //calculate NZP
   CPU->NZPVal = NZP_calc((short)(CPU->R[7]));
   SetNZP(CPU, CPU->NZPVal);

   // Print output for current cycle
   WriteOut(CPU, output);
   printf("WRITING JSRR INSTRUCTION TO FILE. \n");

    CPU->PC = src_reg_val;
    printf("Program Counter set to: %04X\n", CPU->PC);

  } else if (subop == 1) { // JSR

    short int imm11 = instruction & 0x07FF;    // extract immediate value
    printf("Bits 0-10: %04X\n", imm11);

  // Print the 9-bit immediate value in binary
    printf("0-bit immediate value: ");
    for (int i = 10; i >= 0; i--) {
        printf("%d", (imm11 >> i) & 1);
    }
    printf("\n");

    CPU->R[7] = CPU->PC + 1;
    printf("Register R7 set to: %04X\n", CPU->R[7]);

   //calculate NZP
      CPU->NZPVal = NZP_calc((short)CPU->R[7]);
      SetNZP(CPU, CPU->NZPVal);

   // Print output for current cycle
    WriteOut(CPU, output);
    printf("WRITING JSR INSTRUCTION TO FILE. \n");

    CPU->PC = ((CPU->PC) & 0x8000) | (imm11 << 4);
    printf("Program Counter set to: %04X\n", CPU->PC);

	} else { 
    printf("Unknown subopcode\n");
    return;
	}
 }

/*
* Parses rest of jump operation and prints out.
*/
void JumpOp(MachineState* CPU, FILE* output)
{
   // Fetch the instruction
  unsigned short int instruction = CPU->memory[CPU->PC];
 
  // Set control signals
  CPU->DATA_WE = 0;
  CPU->regFile_WE = 0; 
  CPU->NZP_WE = 0;      
  CPU->rsMux_CTL = 0;
 
  unsigned short int subop = (instruction >> 11) & 1;  // Extract bit 11 for subop
  printf("Subopcode: %01X\n", subop);
		
  if (subop == 0) { // JMPR
   unsigned short int src_reg = (instruction >> 6) & 0x07; // Extracting bits 8-6
   printf("Source Reg: %01X\n", src_reg);

   // Print output for current cycle
   WriteOut(CPU, output);
   printf("WRITING JMPR INSTRUCTION TO FILE. \n");

   CPU->PC = CPU->R[src_reg];
   printf("Program Counter set to: %04X\n", CPU->PC);
  }

  else if (subop == 1) { //JMP
    short int imm11 = instruction & 0x07FF; // Extract the 11-bit immediate value
    if (imm11 & 0x0400) { // If the sign bit (bit 10) is set
    imm11 |= 0xF800; // Extend the sign to 16 bits
    }

    short signed_imm11 = (short)imm11;
    printf("Sign-extended IMM11 in decimal: %d\n", signed_imm11);

    // Print output for current cycle
    WriteOut(CPU, output);
    printf("WRITING JMP INSTRUCTION TO FILE. \n");

    CPU->PC = CPU->PC + 1 + imm11 ;
    printf("Program Counter set to: %04X\n", CPU->PC);
   }
  else { 
    printf("Unknown subopcode\n");
    return;
	}
}
 
/*
* Parses rest of logical operation and prints out.
*/
void LogicalOp(MachineState* CPU, FILE* output)
{
  // Fetch the instruction
  unsigned short int instruction = CPU->memory[CPU->PC];
  unsigned short int src_reg = (instruction >> 6) & 0x07; //extracting bits 8-6
  unsigned short int des_reg = (instruction >> 9) & 0x07; //extracting bits 11-9
  printf("Source Reg: %01X\n", src_reg);
  printf("Destination Reg: %01X\n", des_reg);

  // Set control signals
  CPU->DATA_WE = 0;
  CPU->regFile_WE = 1; 
  CPU->NZP_WE = 1;      
  CPU->rsMux_CTL = 0;
  CPU->rtMux_CTL = 0;
  CPU->rdMux_CTL = 0;

  // Check if the instruction is an "AND Immediate" type
    if ((instruction & 0x0020) >> 5) {
    short int imm5 = instruction & 0x001F; // Extract bits 0 to 4 from instruction
    if (imm5 & 0x0010) { // If the sign bit (bit 4) is set
        imm5 |= 0xFFE0;  // Extend the sign to 16 bits
    }
    printf("Immediate 5-bit: 0x%04X (%d)\n", imm5, (short)imm5); // Print in both hex and decimal
    CPU->R[des_reg] = CPU->R[src_reg] & imm5;
  
    } else {

    // Extract sub-opcode (bits 3 to 5)
    unsigned short int subop = (instruction >> 3) & 0x07;
    printf("\n");
    printf("LogicalOP Subopcode: %01X\n", subop);

    // Extract target register (bits 0 to 2)
    unsigned short int target_reg = instruction & 0x07;
    printf("Target Reg: %01X\n", target_reg);

  switch (subop) {
    case 0:
        // AND
        CPU->R[des_reg] = CPU->R[src_reg] & CPU->R[target_reg];
        break;
    case 1:
        // NOT
        CPU->R[des_reg] = ~CPU->R[src_reg];
        break;
    case 2:
        // OR
        CPU->R[des_reg] = CPU->R[src_reg] | CPU->R[target_reg];
        break;
    case 3:
        /// XOR
        CPU->R[des_reg] = CPU->R[src_reg] ^ CPU->R[target_reg];
        break;
    default:
        printf("Unknown subopcode\n");
        break;
    }
  }  
  printf("Value at Destination Register %01X: %01X(%d)\n", des_reg, CPU->R[des_reg], (short)(CPU->R[des_reg]));

  // calculate NZP
  CPU->NZPVal = NZP_calc((short)CPU->R[des_reg]);
  SetNZP(CPU, CPU->NZPVal);

  // Print output for current cycle
  WriteOut(CPU, output);
  printf("WRITING logical INSTRUCTION TO FILE. \n");

  // Increment the PC
  CPU->PC++;
  printf("Updated PC: %04x\n", CPU->PC);
}


/*
* Parses rest of shift/mod operations and prints out.
*/
void ShiftModOp(MachineState* CPU, FILE* output)
{

  // Fetch the instruction
  unsigned short int instruction = CPU->memory[CPU->PC];
  unsigned short int src_reg = (instruction >> 6) & 0x07; //extracting bits 8-6
  unsigned short int des_reg = (instruction >> 9) & 0x07; //extracting bits 11-9
  printf("Source Reg: %01X\n", src_reg);
  printf("Destination Reg: %01X\n", des_reg);

  // Set control signals
  CPU->DATA_WE = 0;
  CPU->regFile_WE = 1; 
  CPU->NZP_WE = 1;      
  CPU->rsMux_CTL = 0;
  CPU->rtMux_CTL = 0;
  CPU->rdMux_CTL = 0;

  unsigned short int subop = (instruction >> 4) & 0x3;  // Extract sub-opcode (bits 4 to 5)
  printf("\n");
  printf("ShiftMod Subopcode: %01X\n", subop);

  if (subop == 3) { // MOD
  unsigned short int target_reg = instruction & 0x07;  // Extract target register (bits 0 to 2)
  printf("Target Reg: %01X\n", target_reg);

  CPU->R[des_reg] = CPU->R[src_reg] % CPU->R[target_reg];
  }
  else {
    short int u_imm4 = instruction & 0x000F; // Extract bits 0 to 3
    switch (subop) { 
      case 0:  // SLL
        CPU->R[des_reg] = CPU->R[src_reg] << u_imm4;
        break;
      case 1:  // SRA
        if (CPU->R[src_reg] & 0x8000) { // negative, sign extend
          CPU->R[des_reg] = CPU->R[src_reg] >> u_imm4;
          unsigned short int temp = 0xFFFF >> (16 - u_imm4);
          CPU->R[des_reg] |= temp << (16 - u_imm4);
        } else { 
          CPU->R[des_reg] = CPU->R[src_reg] >> u_imm4;
        }	
        break;
      case 2:  // SRL
        CPU->R[des_reg] = CPU->R[src_reg] >> u_imm4;
        break;
      default:
        printf("Unknown subopcode\n");
        break;
    }
  }

  printf("Value at Destination Register %01X: %01X(%d)\n", des_reg, CPU->R[des_reg], (short)(CPU->R[des_reg]));

  // calculate NZP
  CPU->NZPVal = NZP_calc((short)CPU->R[des_reg]);
  SetNZP(CPU, CPU->NZPVal);

  // Print output for current cycle
  WriteOut(CPU, output);
  printf("WRITING ShiftMod INSTRUCTION TO FILE. \n");

  // Increment the PC
  CPU->PC++;
  printf("Updated PC: %04x\n", CPU->PC);
 }


/*
* Set the NZP bits in the PSR.
*/
void SetNZP(MachineState* CPU, short result)
{
     // Print NZP value
  printf("NZP Value: %u\n", CPU->NZPVal);

  CPU->PSR &= ~(1 << 0);  // Clear P bit
  CPU->PSR &= ~(1 << 1);  // Clear Z bit
  CPU->PSR &= ~(1 << 2);  // Clear N bit

  switch (result) {
    case 1: {
       CPU->PSR |= (1 << 0);  // Set P bit if positive
       printf("P flag set\n");
       break;
    }
    case 2: {
      CPU->PSR |= (1 << 1);  // Set Z bit if zero
      printf("Z flag set\n");
      break;
    }
    case 4: {
       CPU->PSR |= (1 << 2);  // Set N bit if negative
       printf("N flag set\n");
       break;
    }
    default: {
      printf("Unknown NZP Value\n");
      break;
    }
  }
  // Print the updated PSR
  printf("Updated PSR: \n");
  for (int i = 15; i >= 0; i--) {
    printf("%d", (CPU->PSR >> i) & 1);
  }
  printf("\n");
 }
