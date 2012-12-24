//
//  main.c
//  DisARM
//
//

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#include "elf.h"

void Disassemble(unsigned int *armI, int count, unsigned int startAddress);
void DecodeInstruction(unsigned int instr, unsigned int Address);
void HexToBinary(int *bits, unsigned int hex);
int SignExtend(unsigned int x, int bits);
int Rotate(unsigned int rotatee, int amount);
void PrintASCII(unsigned int instr);
void ProcessSWI(int *bits, unsigned int instr, char *instructionPtr);
void ProcessBranch(int *bits, unsigned int instr, unsigned int currentAddress, char *instructionPtr);
void ProcessDP(int *bits, unsigned int instr, char *instructionPtr);
void ProcessLDR(int *bits, unsigned int instr, char *instructionPtr);
void ProcessMUL (int *bits, unsigned int instr, char *instructionPtr);
void ProcessLDM(int *bits, unsigned int instr, char *instructionPtr);


int main(int argc, const char * argv[])
{
    FILE *fp;
    ELFHEADER elfhead;
    int i;
    unsigned int *armInstructions = NULL;
    
    if(argc < 2)
    {
        fprintf(stderr, "Usage: DisARM <filename>\n");
        return 1;
    }
    
    /* Open ELF file for binary reading */
   if((fp = fopen(argv[1], "rb")) == NULL)
    {
      fprintf(stderr, "%s\n", argv[1]);
      exit(EXIT_FAILURE);
    }
    /* Read in the header */
    fread(&elfhead, 1, sizeof(ELFHEADER), fp);
    if(!(elfhead.magic[0] == 0177 && elfhead.magic[1] == 'E' && elfhead.magic[2] == 'L' && elfhead.magic[3] == 'F'))
    {
        fprintf(stderr, "%s is not an ELF file\n", argv[1]);
        return 2;
    }
    printf("\nFile-type: %d\n",elfhead.filetype);
    printf("Arch-type: %d\n",elfhead.archtype);
    printf("Entry: %x\n", elfhead.entry);
    printf("Prog-Header: %x\n", elfhead.phdrpos);
    printf("Prog-Header-count: %d\n", elfhead.phdrcnt);
    printf("Section-Header: %x\n", elfhead.shdrpos);
    
    /* Find and read program headers */
    ELFPROGHDR *prgHdr[elfhead.phdrcnt];
    for(i = 0; i < elfhead.phdrcnt; i++)
      {
	fseek(fp, elfhead.phdrpos*(i+1), SEEK_SET);
	prgHdr[i] = (ELFPROGHDR*)malloc(sizeof(ELFPROGHDR));
	if(!prgHdr)
	  {
	    fprintf(fp, "Out of Memory\n");
	    fclose(fp);
	    return 3;
	  }
	
	fread(prgHdr[i], 1, sizeof(ELFPROGHDR), fp);
	printf("Segment-Offset: %x\n", prgHdr[i]->offset);
	printf("File-size: %d\n", prgHdr[i]->filesize);
	printf("Align: %d\n", prgHdr[i]->align);
      
        /* allocate memory and read in ARM instructions */
        armInstructions = (unsigned int *)malloc(prgHdr[i]->filesize + 3 & ~3);
        if(armInstructions == NULL)
        {
            fclose(fp);
            free(prgHdr[elfhead.phdrcnt]);
            fprintf(stderr, "Out of Memory\n");
            return 3;
        }
        fseek(fp, prgHdr[i]->offset, SEEK_SET);
        fread(armInstructions, 1, prgHdr[i]->filesize, fp);
        
        /* Disassemble */
        printf("\nInstructions\n\n");
        
        Disassemble(armInstructions, (prgHdr[i]->filesize + 3 & ~3) /4, prgHdr[i]->virtaddr);
        printf("\n"); 
        free(armInstructions);
	free(prgHdr[i]);
      }
    fclose(fp);

    return 0;
}

void Disassemble(unsigned int *armI, int count, unsigned int startAddress)
{
    int i;
    printf("Address   Hex       ASCII\tDisassembly\n");
    printf("=============================================\n");
    for(i = 0; i < count; i++)
    {
        printf("%08X  %08X", startAddress + i*4, armI[i]);
        DecodeInstruction(armI[i], startAddress + i*4);
        printf("\n");
    }
    
}

void DecodeInstruction(unsigned int instr, unsigned int Address)
{
  int bits[32];
  char instruction[100];

  sprintf(instruction, "\0");

  HexToBinary(bits, instr);
  printf("  ");
  PrintASCII(instr);
  printf("\t");

  if (bits[27] && bits[26] && bits[25] && bits[24])
    ProcessSWI(bits, instr, instruction);
  else if (bits[27] && !bits[26] && bits[25])
    ProcessBranch(bits, instr, Address, instruction);
  else if (!bits[27] && bits[26])
    {
    if (!bits[25]) 
      ProcessLDR(bits, instr, instruction);
    else if (bits[4])
      strcat(instruction, "Undefined Instruction.");
    else if (!bits[4])
      ProcessLDR(bits, instr, instruction);
    }
  else if (bits[27] && !bits[26] && !bits[25])
    ProcessLDM(bits, instr, instruction);
  else if (!(bits[27] || bits[26] || bits[25] || bits[24] || bits[23] || bits[22]) && (bits[7] && !bits[6] && !bits[5] && bits[4]))
    ProcessMUL(bits, instr, instruction);
  else if (!bits[27] && !bits[26])
    ProcessDP(bits, instr, instruction);
  else
    strcat(instruction, "Undefined Instruction.");
     
  printf("%s", instruction);

  /* DEBUG PRINT BINARY */
  /*printf("\t\t");
    for(int i=31;i>=0;i--)
    {
    printf("%d", bits[i]);
    }*/
}

int SignExtend(unsigned int x, int bits)
{
    int r;
    int m = 1U << (bits - 1);
    x = x & ((1U << bits) - 1); 
    r = (x ^ m) - m;
    return r;
}
               
int Rotate(unsigned int rotatee, int amount)
{
    unsigned int mask, lo, hi;

    mask = (1 << amount) - 1;
    lo = rotatee & mask;
    hi = rotatee >> amount;

    rotatee = (lo << (32 - amount)) | hi;
    
    return rotatee;
}

void HexToBinary(int *bits, unsigned int hex)
{
  int i;
  for (i = 0; i < 32; i++) 
    {
      bits[i] = (hex >> i) & 1;
    }
}

void PrintCCode(unsigned int instr, char *instructionPtr)
{
  switch((instr & 0xF0000000) >> 28)
    {
    case(13): strcat(instructionPtr, "LE"); break;
    case(12): strcat(instructionPtr, "GT"); break;
    case(11): strcat(instructionPtr, "LT"); break;
    case(10): strcat(instructionPtr, "GE"); break;
    case(9): strcat(instructionPtr, "LS"); break;
    case(8): strcat(instructionPtr, "HI"); break;
    case(7): strcat(instructionPtr, "VC"); break;
    case(6): strcat(instructionPtr, "VS"); break;
    case(5): strcat(instructionPtr, "PL"); break;
    case(4): strcat(instructionPtr, "MI"); break;
    case(3): strcat(instructionPtr, "CC/LO"); break;
    case(2): strcat(instructionPtr, "CS/HS"); break;
    case(1): strcat(instructionPtr, "NE"); break;
    case(0): strcat(instructionPtr, "EQ"); break;
    }
}
 
void PrintASCII(unsigned int instr)
{
  char c[5];
  int i;

  c[0] = (instr & 0x000000FF);
  c[1] = (instr & 0x0000FF00) >> 8;
  c[2] = (instr & 0x00FF0000) >> 16;
  c[3] = (instr & 0xFF000000) >> 24;
  
  for(i = 0; i < 4; i++)
    {
      if(isalpha(c[i]) || ispunct(c[i]))
	printf("%c", c[i]);
      else
	printf("*");
    }
}

void ProcessSWI(int *bits, unsigned int instr, char *instructionPtr)
{
  strcat(instructionPtr, "SWI");
  PrintCCode(instr, instructionPtr);
  sprintf(instructionPtr + strlen(instructionPtr), "\t%X", instr & 0x00FFFFFF);
}

void ProcessBranch(int *bits, unsigned int instr, unsigned int currentAddress, char *instructionPtr)
{
  if ( bits[24] )
    strcat(instructionPtr, "BL");
  else
    strcat(instructionPtr, "B");
  PrintCCode(instr, instructionPtr);
  sprintf(instructionPtr + strlen(instructionPtr), "\t&%X", (SignExtend((instr & 0x00FFFFFF) << 2, 26)) + currentAddress + 8);
}

void ProcessDP(int *bits, unsigned int instr, char *instructionPtr)
{
  int DP = (instr & 0x01E00000) >> 21;
  int Rd = (instr & 0x0000F000) >> 12;
  int Rn = (instr & 0x000F0000) >> 16;
  int Rot = (instr & 0x00000F00) >> 8;
  int Op = (instr & 0x000000FF);
  int Rm = (instr & 0x0000000F);
  int Sh = (instr & 0x00000060) >> 5;
  int Shift;

  switch(DP)
    {
    case(15): strcat(instructionPtr, "MVN"); break;
    case(14): strcat(instructionPtr, "BIC"); break;
    case(13): strcat(instructionPtr, "MOV"); break;
    case(12): strcat(instructionPtr, "ORR"); break;
    case(11): strcat(instructionPtr, "CMN"); break;
    case(10): strcat(instructionPtr, "CMP"); break;
    case(9): strcat(instructionPtr, "TEQ"); break;
    case(8): strcat(instructionPtr, "TST"); break;
    case(7): strcat(instructionPtr, "RSC"); break;
    case(6): strcat(instructionPtr, "SBC"); break;
    case(5): strcat(instructionPtr, "ADC"); break;
    case(4): strcat(instructionPtr, "ADD"); break;
    case(3): strcat(instructionPtr, "RSB"); break;
    case(2): strcat(instructionPtr, "SUB"); break;
    case(1): strcat(instructionPtr, "EOR"); break;
    case(0): strcat(instructionPtr, "AND"); break;
    }

  PrintCCode(instr, instructionPtr);
  if (bits[20] && DP != 10 && DP != 11)
    strcat(instructionPtr, "S");

  if (DP != 11 && DP != 10)
    sprintf(instructionPtr + strlen(instructionPtr), "\tR%d, ", Rd);
  else
    strcat(instructionPtr, "\t");

  if (DP != 13 && DP != 15)
    sprintf(instructionPtr + strlen(instructionPtr), "R%d, ", Rn);

  if (bits[25])
    {
      if (!(bits[11] || bits[10] || bits[9] || bits[8]))
	sprintf(instructionPtr + strlen(instructionPtr), "#&%X", Op);
      else
	sprintf(instructionPtr + strlen(instructionPtr), "#&%X", Rotate(Op, Rot*2));
    }
  else
    {
      if (bits[4])
	Shift = (instr & 0x00000F00) >> 8;
      else
	Shift = (instr & 0x00000F80) >> 7;
      sprintf(instructionPtr + strlen(instructionPtr), "R%d", Rm);
      if (Shift > 0)
	{
	  switch(Sh)
	    {
	    case(3): if(Shift == 0) strcat(instructionPtr, ", RRX"); else strcat(instructionPtr, ", ROR "); break;
	    case(2): strcat(instructionPtr, ", ASR "); break;
	    case(1): strcat(instructionPtr, ", LSR "); break;
	    case(0): strcat(instructionPtr, ", LSL "); break;
	    }
	  sprintf(instructionPtr + strlen(instructionPtr), "#&%X", Shift);
	}
    }
}

void ProcessLDR(int *bits, unsigned int instr, char *instructionPtr)
{
    int sourceReg = (instr & 0x0000F000) >> 12;
    int baseReg = (instr & 0x000F0000) >> 16;
    int Rm = (instr & 0x0000000F);
    int immediate = (instr & 0x00000FFF);
    int Shift = (instr & 0x00000F80) >> 7;
    int Sh = (instr & 0x00000060) >> 5;

    if (bits[20])
      strcat(instructionPtr, "LDR");
    else
      strcat(instructionPtr, "STR");

    PrintCCode(instr, instructionPtr);
    if (bits[22])
      strcat(instructionPtr, "B");

    sprintf(instructionPtr + strlen(instructionPtr), "\tR%d", sourceReg);
    
    if (bits[24]) // Pre incremented
      {
        sprintf(instructionPtr + strlen(instructionPtr), ", [R%d", baseReg);
	if (bits[25])
	  sprintf(instructionPtr + strlen(instructionPtr), ", R%d]", Rm);
	else
	  {
	    if (immediate > 0)
	      sprintf(instructionPtr + strlen(instructionPtr), ", #%d]", immediate);
	    else
	      strcat(instructionPtr, "]");
	  }
	if (bits[21])
	  strcat(instructionPtr, "!");
      }
    else // Post incremented
      {
	if (bits[21])
	  strcat(instructionPtr, "!");
        sprintf(instructionPtr + strlen(instructionPtr), ", [R%d]", baseReg);
	if (bits[25])
	  {
	    if(bits[23])
	      sprintf(instructionPtr + strlen(instructionPtr), ", R%d", Rm);
	    else
	      sprintf(instructionPtr + strlen(instructionPtr), ", -R%x", Rm); 

	    if (Shift > 0)
	      {
		switch(Sh)
		  {
		  case(3): if(Shift == 0) strcat(instructionPtr, ", RRX"); else strcat(instructionPtr, ", ROR "); break;
		  case(2): strcat(instructionPtr, ", ASR "); break;
		  case(1): strcat(instructionPtr, ", LSR "); break;
		  case(0): strcat(instructionPtr, ", LSL "); break;
		  }
	        sprintf(instructionPtr + strlen(instructionPtr), "#&%X", Shift*2); //DOUBLED..
	      }
	  }
	else
	  {
	    if (bits[23])
	      sprintf(instructionPtr + strlen(instructionPtr), ", #&%x", immediate);
	    else
	      sprintf(instructionPtr + strlen(instructionPtr), ", #-&%x", immediate); 
	  }
      }
}

void ProcessMUL (int *bits, unsigned int instr, char *instructionPtr)
{
  int Rd = (instr & 0x000F0000) >> 16;
  int Rm = instr & 0x0000000F;
  int Rs = (instr & 0x00000F00) >> 8;
  int Rn = (instr & 0x0000F000) >> 12;

  if (bits[21])
    strcat(instructionPtr, "MLA");
  else
    strcat(instructionPtr, "MUL");
  PrintCCode(instr, instructionPtr);
  if(bits[20])
    strcat(instructionPtr, "S");
  sprintf(instructionPtr + strlen(instructionPtr), "\tR%d, R%d, R%d", Rd, Rm, Rs);
  if(bits[21])
    printf(", R%d", Rn);
}

void ProcessLDM(int *bits, unsigned int instr, char *instructionPtr)
{
  char regList[256] = { 0 };
  int listState = 0;
  int Rn = (instr & 0x000F0000) >> 16;
  int i;

    if (bits[20])
      strcat(instructionPtr, "LDM");
    else
      strcat(instructionPtr, "STM");

    PrintCCode(instr, instructionPtr);

    sprintf(instructionPtr + strlen(instructionPtr), "\tR%d", Rn);
    if (bits[21])
      strcat(instructionPtr, "!");
    strcat(instructionPtr, ", ");
    strcat(regList, "{");
    for (i = 0; i<16; i++)
      {
	if (bits[i] && !bits[i+1])
	  {
	    sprintf(regList + strlen(regList), "R%d, ", i);
	    listState = 0;
	  }
	else if (bits[i] && bits[i+1] && !listState)
	  {
	    sprintf(regList + strlen(regList), "R%d-", i);
	    listState = 1;
	  }
      }
    strcat(regList, "\b\b}");
    strcat(instructionPtr, regList);
    if(bits[22])
      strcat(instructionPtr, "^");
}
