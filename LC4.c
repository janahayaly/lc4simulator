/*
 * LC4.c: Defines simulator functions for executing instructions
 */

#include "LC4.h"
#include <stdio.h>

#define INSN_OP(I) ((I) >> 12) // EXTRACTS [15:12]
#define INSN_OP_5bit(I) ((I) >> 11) // EXTRACTS [15:11]
#define INSN_11th_bit(I) (((I) >> 11) & 0x1) // 11
#define INSN_dest(I) (((I) >> 9) & 0x7) // EXTRACTS [11:9] 
#define INSN_s(I) (((I) >> 6) & 0x7) // EXTRACTS [8:6]
#define INSN_ar_type(I) (((I) >> 3) & 0x7) // EXTRACTS [5:3]
#define INSN_t(I) ((I) & 0x7) // EXTRACTS [2:0]
#define INSN_5th_bit(I) (((I) >> 5) & 0x1) // EXTRACTS [5]
#define INSN_last_5(I) ((I) & 0x1F);
#define INSN_comp_type(I) (((I) >> 7) & 0x3) // EXTRACTS [8:7]
#define INSN_last_7(I) ((I) & 0x7F); 
#define INSN_last_6(I) ((I) & 0x3F); 
#define INSN_last_10(I) ((I) & 0x3FF);
#define INSN_last_4(I) ((I) & 0xF);
#define INSN_last_9(I) ((I) & 0x1FF);
#define INSN_last_8(I) ((I) & 0xFF);
#define INSN_mod_shift_type(I) (((I) >> 4) & 0x3);

/*
 * Reset the machine state as Pennsim would do
 */
void Reset(MachineState* CPU)
{
  for (int i = 0; i < 8; i++) {
    CPU -> R[i] = '\0';
  }
  for (int i = 0; i < 65536; i++) {
    CPU -> memory[i] = '\0';
  }

  CPU -> PC = 0x8200;
  CPU -> PSR = 0x8002;
  ClearSignals(CPU);
}


/*
 * Clear all of the control signals (set to 0)
 */
void ClearSignals(MachineState* CPU)
{
  CPU -> rdMux_CTL = 0;
  CPU -> rsMux_CTL = 0;
  CPU -> rtMux_CTL = 0;
  CPU -> regFile_WE = 0;
  CPU -> regInputVal = 0;
  CPU -> NZP_WE = 0;
  CPU -> DATA_WE = 0;
  CPU -> NZPVal = 0;
  CPU -> dmemAddr = 0;
  CPU -> dmemValue = 0;
}


/*
 * This function should write out the current state of the CPU to the file output.
 */
void WriteOut(MachineState* CPU, FILE* output)
{
/*1. the current PC
2. the current instruction (written in binary)
3. the register file WE
4. if regFileWE is high, which register is being written to
5. if regFileWE is high, what value is being written to the register file
6. the NZP WE
7. if NZP WE is high, what value is being written to the NZP register
8. the data WE
9. if data WE is high, the data memory address
10. if data WE is high, what value is being loaded or stored into memory*/
  unsigned short inst = CPU -> memory[CPU -> PC];
  fprintf(output, "%04X ", CPU -> PC);
  for (int i = 15; i >= 0; i--) {
    int bit = (inst >> i) % 2;
    fprintf(output, "%d", bit);
  }
  fprintf(output, " ");
  fprintf(output, "%X ", CPU -> regFile_WE);
  if (CPU -> regFile_WE == 1) {
    if (CPU -> rdMux_CTL == 0) {
      fprintf(output, "%X ", INSN_dest(inst));
    } else {
      fprintf(output, "7 ");
    }
  } else {
    fprintf(output, "0 ");
  }
  if (CPU -> regFile_WE == 1) {
    fprintf(output, "%04X ", CPU -> regInputVal);
  } else {
    fprintf(output, "0000 ");
  }
  unsigned short nzp_we = CPU -> NZP_WE;
  fprintf(output, "%X ", nzp_we);
  if (nzp_we == 1) {
    fprintf(output, "%X ", CPU -> NZPVal);
  } else {
    fprintf(output, "0 ");
  }
  unsigned short data_we = CPU -> DATA_WE;
  fprintf(output, "%X ", data_we);
  if (data_we == 1) {
    fprintf(output, "%04X ", CPU -> dmemAddr);
    fprintf(output, "%04X", CPU -> dmemValue);
  } else {
    fprintf(output, "0000 ");
    fprintf(output, "0000");
  }
  fprintf(output, " \n");
}

//helpers:
// make helper for sign extension and checking neg vs pos
int extendSign(unsigned short num, int len) {
  int s = 16 - len;
  return (signed short)(num << s) >> s;
}


void setPC(MachineState* CPU, short pc, FILE* output) {
  unsigned short new_pc = pc + 1;
  unsigned short bit = CPU -> PSR >> 15;
  WriteOut(CPU, output);
  if ((new_pc >= 0x2000 && new_pc <= 0x7FFF) || 
  (new_pc >= 0X8000 && bit == 0) || (new_pc >= 0XA000 && bit == 1)) {
    printf("Invalid PC, setting to default");
    CPU -> PC = 0x80FF;
  } else {
    CPU -> PC = new_pc;
  }
}

void checkOOB(MachineState* CPU, unsigned short pc, FILE* output) {
  unsigned short bit = CPU -> PSR >> 15;
  if ((pc >= 0x2000 && pc <= 0x7FFF) || 
  (pc >= 0x8000 && bit == 0) || (pc >= 0xA000 && bit == 1)) {
    printf("Invalid PC, setting to default");
    CPU -> PC = 0x80FF;
  } else {
    CPU -> PC = pc;
  }
}

/*
 * This function should execute one LC4 datapath cycle.
 */
int UpdateMachineState(MachineState* CPU, FILE* output)
{
  unsigned short pc = CPU -> memory[CPU -> PC];
  unsigned short operation = INSN_OP(pc);
  unsigned short special_op = INSN_OP_5bit(pc);
  unsigned short dest = INSN_dest(pc);
  unsigned short t_reg = INSN_t(pc);
  unsigned short s_reg = INSN_s(pc);
  unsigned short last_6 = INSN_last_6(pc);
  signed short last_9 = INSN_last_9(pc);
  unsigned short last_8 = INSN_last_8(pc);
  unsigned short bit = (CPU -> PSR) >> 15;

  switch (operation) {
    case 0:
      BranchOp(CPU, output);
      break;
    case 1: 
      ArithmeticOp(CPU, output);
      break;
   case 2: 
      ComparativeOp(CPU, output);
      break;
    case 5: 
      LogicalOp(CPU, output);
      break;
   case 10: 
      ShiftModOp(CPU, output);
      break;
   case 4:
      JSROp(CPU, output);
      break;
   case 12:
      JumpOp(CPU, output);
      break;
   case 7: //str
      CPU -> rtMux_CTL = 1;
      CPU -> DATA_WE = 1;
      CPU -> dmemAddr = (CPU -> R[s_reg]) + extendSign(last_6, 6);
      CPU -> dmemValue = CPU -> R[t_reg];
      if (((CPU -> dmemAddr <= 0x7FFF) && (CPU -> dmemAddr >= 0x2000)) || 
      ((CPU -> dmemAddr <= 0xFFFF) && (CPU -> dmemAddr >= 0xA000) && bit == 1)) {
        CPU -> memory[CPU -> dmemAddr] = CPU -> dmemValue;
        SetNZP(CPU, CPU -> regInputVal);
        setPC(CPU, CPU -> PC, output);
      } else {
        printf("Invalid memory address");
        return -1;
      }
      break;
   case 6: //ldr
      CPU -> regFile_WE = 1;
      CPU -> NZP_WE = 1;
      CPU -> DATA_WE = 0;
      CPU -> dmemAddr = (CPU -> R[s_reg]) + extendSign(last_6, 6);
      CPU -> dmemValue = CPU -> memory[CPU -> dmemAddr];
      CPU -> regInputVal = CPU -> dmemValue;
      if (((CPU -> dmemAddr <= 0x7FFF) && (CPU -> dmemAddr >= 0x2000)) || 
      ((CPU -> dmemAddr <= 0xFFFF) && (CPU -> dmemAddr >= 0xA000) && bit == 1)) {
        CPU -> R[dest] = CPU -> regInputVal;
        CPU -> rsMux_CTL = 0;
        SetNZP(CPU, CPU -> regInputVal);
        setPC(CPU, CPU -> PC, output);
      } else {
        printf("Invalid memory address");
        return -1;
      }
      break;
   case 9: //const
      CPU -> regFile_WE = 1;
      CPU -> rdMux_CTL = 0;
      CPU -> NZP_WE = 1;
      CPU -> regInputVal = extendSign(last_9, 9);
      CPU -> R[dest] = CPU -> regInputVal;
      SetNZP(CPU, CPU -> regInputVal);
      setPC(CPU, CPU -> PC, output);
      break;
   case 13: //hiconst
      CPU -> regFile_WE = 1;
      CPU -> rsMux_CTL = 1;
      CPU -> NZP_WE = 1;
      CPU -> regInputVal = ((CPU -> R[dest]) & 0xFF) | extendSign(last_8, 8);
      CPU -> R[dest] = CPU -> regInputVal;
      SetNZP(CPU, CPU -> regInputVal);
      setPC(CPU, CPU -> PC, output);
      break;
   case 15: //trap
      CPU -> regFile_WE = 1;
      CPU -> rdMux_CTL = 1;
      CPU -> NZP_WE = 1;
      CPU -> regInputVal = (CPU -> PC) + 1;
      CPU -> R[7] = CPU -> regInputVal;
      CPU -> PSR |= 0x8000;
      WriteOut(CPU, output);
      CPU -> PC = (0x8000 | extendSign(last_8, 8));
      checkOOB(CPU, CPU -> PC, output);
      SetNZP(CPU, CPU -> regInputVal);
      break;
   case 8: //rti
      CPU -> regFile_WE = 0;
      CPU -> rtMux_CTL = 1;
      CPU -> NZP_WE = 0;
      CPU -> PSR = (unsigned short) ((CPU -> PSR) << 1) >> 1;
      WriteOut(CPU, output);
      SetNZP(CPU, CPU -> regInputVal);
      CPU -> PC = CPU -> R[7];
      checkOOB(CPU, CPU -> PC, output);
      break;
   default:
      printf("Invalid instruction");
      return -1;
  }
  return 0;
}



//////////////// PARSING HELPER FUNCTIONS ///////////////////////////



/*
 * Parses rest of branch operation and updates state of machine.
 */
void BranchOp(MachineState* CPU, FILE* output)
{
  CPU -> NZP_WE = 0;
  CPU -> DATA_WE = 0;
  CPU -> regFile_WE = 0;
  unsigned short pc = CPU -> memory[CPU -> PC];
  unsigned short type = INSN_dest(pc);
  signed short last_9 = INSN_last_9(pc);
  unsigned short nzp = CPU -> PSR & 0X7;
  
  WriteOut(CPU, output);
  switch (type){
    case 0: 
      break;
    case 4: 
      if ((nzp & 0x4) == 4) {
        CPU -> PC = (CPU -> PC) + 1 + extendSign(last_9, 9);
      } else {
        CPU -> PC = (CPU -> PC) + 1;
      }
      break;
    case 6: 
      if (((nzp & 0x6) == 6) || ((nzp & 0x6) == 4) || ((nzp & 0x6) == 2)) {
        CPU -> PC = (CPU -> PC) + 1 + extendSign(last_9, 9);
      } else {
        CPU -> PC = (CPU -> PC) + 1;
      }
      break;
   case 5: 
      if (((nzp & 0x5) == 5) || ((nzp & 0x5) == 4) || ((nzp & 0x5) == 1)) {
        CPU -> PC = (CPU -> PC) + 1 + extendSign(last_9, 9);
      } else {
        CPU -> PC = (CPU -> PC) + 1;
      }
      break;
    case 2: 
      if ((nzp & 0x2) == 2) {
        CPU -> PC = (CPU -> PC) + 1 + extendSign(last_9, 9);
      } else {
        CPU -> PC = (CPU -> PC) + 1;
      }
      break;
    case 3: 
      if (((nzp & 0x3) == 3) || ((nzp & 0x3) == 1) || ((nzp & 0x3) == 2)) {
        CPU -> PC = (CPU -> PC) + 1 + extendSign(last_9, 9);
      }
      break;
    case 1: 
      if ((nzp & 0x1) == 1) {
        CPU -> PC = (CPU -> PC) + 1 + extendSign(last_9, 9);
      } else {
        CPU -> PC = (CPU -> PC) + 1;
      }
      break;
    case 7: 
      CPU -> PC = (CPU -> PC) + 1 + extendSign(last_9, 9);
      break;
   default:
      printf("Invalid branch operation");
  }
  SetNZP(CPU, CPU -> regInputVal);
  checkOOB(CPU, CPU -> PC, output);
}

/*
 * Parses rest of arithmetic operation and prints out.
 */
void ArithmeticOp(MachineState* CPU, FILE* output)
{
  unsigned short pc = CPU -> memory[CPU -> PC];
  unsigned short type = INSN_ar_type(pc);
  unsigned short dest = INSN_dest(pc);
  unsigned short t_reg = INSN_t(pc);
  unsigned short s_reg = INSN_s(pc);
  unsigned short fifth = INSN_5th_bit(pc);
  unsigned short last_f = INSN_last_5(pc);

  switch (type){
    case 0: 
      if (fifth == 1) {
        CPU -> regInputVal = extendSign(last_f, 5) + (CPU -> R[s_reg]);
        CPU -> R[dest] = CPU -> regInputVal;
      } else {
        CPU -> regInputVal = (CPU -> R[t_reg]) + (CPU -> R[s_reg]);
        CPU -> R[dest] = CPU -> regInputVal;
      }
      break;
   case 1: 
      CPU -> regInputVal = (CPU -> R[t_reg]) * (CPU -> R[s_reg]);
      CPU -> R[dest] = CPU -> regInputVal;
      break;
    case 2: 
      CPU -> regInputVal = (CPU -> R[s_reg]) - (CPU -> R[t_reg]);
      CPU -> R[dest] = CPU -> regInputVal;
      break;
   case 3: 
      if (t_reg != 0) {
        CPU -> regInputVal = (CPU -> R[s_reg]) / (CPU -> R[t_reg]);
        CPU -> R[dest] = CPU -> regInputVal;
      } else {
        printf("Attempted division by 0");
      }
      break;
   default:
      printf("Invalid arithmetic operation");
  }
  SetNZP(CPU, CPU -> regInputVal);
  setPC(CPU, CPU -> PC, output);
}

/*
 * Parses rest of comparative operation and prints out.
 */
void ComparativeOp(MachineState* CPU, FILE* output)
{
  unsigned short pc = CPU -> memory[CPU -> PC];
  unsigned short type = INSN_comp_type(pc);
  unsigned short s_reg = INSN_dest(pc);
  unsigned short t_reg = INSN_t(pc);
  unsigned short last_s = INSN_last_7(pc);
  signed short signed_res;
  unsigned short unsigned_res;

  switch (type){
    case 0: 
      signed_res = (CPU -> R[s_reg]) - (CPU -> R[t_reg]);
      SetNZP(CPU, signed_res);
      break;
   case 1: 
      unsigned_res = (CPU -> R[s_reg]) - (CPU -> R[t_reg]);
      SetNZP(CPU, unsigned_res);
      break;
    case 2: 
      signed_res = (CPU -> R[s_reg]) - extendSign(last_s, 7);
      SetNZP(CPU, signed_res);
      break;
   case 3: 
      unsigned_res = (CPU -> R[s_reg]) - extendSign(last_s, 7);
      SetNZP(CPU, unsigned_res);
      break;
   default:
      printf("Invalid comparative operation");
  }
  SetNZP(CPU, CPU -> regInputVal);
  setPC(CPU, CPU -> PC, output);
  //WriteOut(CPU, output);
}

/*
 * Parses rest of logical operation and prints out.
 */
void LogicalOp(MachineState* CPU, FILE* output)
{
  unsigned short pc = CPU -> memory[CPU -> PC];
  unsigned short type = INSN_ar_type(pc);
  unsigned short dest = INSN_dest(pc);
  unsigned short t_reg = INSN_t(pc);
  unsigned short s_reg = INSN_s(pc);
  unsigned short fifth = INSN_5th_bit(pc);
  unsigned short last_f = INSN_last_5(pc);
  unsigned short res;

  if (fifth == 1) {
    res = extendSign(last_f, 5) & (CPU -> R[s_reg]);
    CPU -> regInputVal = res;
    CPU -> R[dest] = CPU -> regInputVal;
    return;
  }

  switch (type){
    case 0: 
      res = (CPU -> R[t_reg]) & (CPU -> R[s_reg]);
      CPU -> regInputVal = res;
      CPU -> R[dest] = CPU -> regInputVal;
      break;
   case 1: 
      res = ~ (CPU -> R[s_reg]);
      CPU -> regInputVal = res;
      CPU -> R[dest] = CPU -> regInputVal;
      break;
   case 2: 
      res = (CPU -> R[s_reg]) | (CPU -> R[t_reg]);
      CPU -> regInputVal = res;
      CPU -> R[dest] = CPU -> regInputVal;
      break;
   case 3: 
      res = (CPU -> R[s_reg]) ^ (CPU -> R[t_reg]);
      CPU -> regInputVal = res;
      CPU -> R[dest] = CPU -> regInputVal;
      break;
   default:
      printf("Invalid logical operation");
  }
  SetNZP(CPU, CPU -> regInputVal);
  setPC(CPU, CPU -> PC, output);
}

/*
 * Parses rest of jump operation and prints out.
 */
void JumpOp(MachineState* CPU, FILE* output)
{
  unsigned short pc = CPU -> memory[CPU -> PC];
  unsigned short s_reg = INSN_s(pc);
  unsigned short bit_11 = INSN_11th_bit(pc);
  unsigned short last_10 = INSN_last_10(pc);
  WriteOut(CPU, output);
  if (bit_11 == 1) {
    CPU  -> PC = (CPU -> PC) + 1 + extendSign(last_10, 10);
    checkOOB(CPU, CPU -> PC, output);
  } else if (bit_11 == 0){
    CPU -> PC = (CPU -> R[s_reg]);
    checkOOB(CPU, CPU -> PC, output);
  } else {
    printf("Invalid Jump");
  }
  SetNZP(CPU, CPU -> regInputVal);
}

/*
 * Parses rest of JSR operation and prints out.
 */
void JSROp(MachineState* CPU, FILE* output)
{
  unsigned short pc = CPU -> memory[CPU -> PC];
  unsigned short s_reg = INSN_s(pc);
  unsigned short bit_11 = INSN_11th_bit(pc);
  unsigned short last_10 = INSN_last_10(pc);

  if (bit_11 == 1) {
    CPU -> regInputVal = (CPU -> PC) + 1;
    CPU -> R[7] = CPU -> regInputVal;
    SetNZP(CPU, CPU -> regInputVal);
    WriteOut(CPU, output);
    CPU -> PC = (((CPU -> PC) & 0x8000) | (extendSign(last_10, 10) << 4));
    checkOOB(CPU, CPU -> PC, output);
  } else if (bit_11 == 0){
    CPU -> regInputVal = (CPU -> PC) + 1;
    CPU -> R[7] = CPU -> regInputVal;
    SetNZP(CPU, CPU -> regInputVal);
    WriteOut(CPU, output);
    CPU -> PC = (CPU -> R[s_reg]);
    checkOOB(CPU, CPU -> PC, output);
  } else {
    printf("Invalid Jump");
  }
}

/*
 * Parses rest of shift/mod operations and prints out.
 */
void ShiftModOp(MachineState* CPU, FILE* output)
{
  unsigned short pc = CPU -> memory[CPU -> PC];
  unsigned short type = INSN_mod_shift_type(pc);
  unsigned short dest = INSN_dest(pc);
  unsigned short t_reg = INSN_t(pc);
  unsigned short s_reg = INSN_s(pc);
  unsigned short last_4 = INSN_last_4(pc);
  unsigned short res;

  switch (type){
    case 0: 
      res = (CPU -> R[s_reg]) << extendSign(last_4, 4);
      CPU -> regInputVal = res;
      CPU -> R[dest] = CPU -> regInputVal;
      break;
   case 1: 
      res = (CPU -> R[s_reg]) >> extendSign(last_4, 4); //what do i do here >>>
      CPU -> regInputVal = res;
      CPU -> R[dest] = CPU -> regInputVal;
      break;
    case 2: 
      res = (CPU -> R[s_reg]) >> extendSign(last_4, 4);
      CPU -> regInputVal = res;
      CPU -> R[dest] = CPU -> regInputVal;
      break;
   case 3: 
      res = (CPU -> R[s_reg]) % (CPU -> R[t_reg]);
      CPU -> regInputVal = res;
      CPU -> R[dest] = CPU -> regInputVal;
      break;
   default:
      printf("Invalid shift or mod operation");
  }
  SetNZP(CPU, CPU -> regInputVal);
  setPC(CPU, CPU -> PC, output);
}

/*
 * Set the NZP bits in the PSR.
 */
void SetNZP(MachineState* CPU, short result) {   
    short setting;
    CPU -> PSR = CPU -> PSR & ~0x0007;
    if (result > 0) {
      setting = 0x1;
      CPU -> PSR = (CPU -> PSR >> 3) << 3;
      CPU -> PSR = (CPU -> PSR | setting);
    } else if (result < 0) {
      setting = 0x4;
      CPU -> PSR = (CPU -> PSR >> 3) << 3;
      CPU -> PSR = (CPU -> PSR | setting);
    } else {
      setting = 0x2;
      CPU -> PSR = (CPU -> PSR >> 3) << 3;
      CPU -> PSR = (CPU -> PSR | setting);
    }
    CPU -> NZPVal = CPU -> PSR & 0x7;
}
