/*
 * loader.c : Defines loader functions for opening and loading object files
 */

#include "loader.h"

// memory array location
unsigned short memoryAddress;

/*
 * Read an object file and modify the machine state as described in the writeup

This function should take the name of an object file as input, read the contents of that input file
which should be in the format listed below, and use those values to initialize the memory of the
machine. A pointer to the machine state is passed in as an argument.

Code: 3-word header (0xCADE, <address>, <n>, n-word body comprising the instructions
Data: 3-word header (0xDADA, <address>, <n>), n-word body comprising the initial data values
Symbol: 3-word header (0xC3B7, <address>, <n>), n-character body of the symbol string
File name: 2-word header (0xF17E, <n>), n-character body comprising the file name string
Line number: 4-word header (0x715E, <address>, <line>, <file-index>), no body. File-index is the
index of the file in the list of file name sections; so, if your code comes from two C files, your
line number directives should be attached to file numbers 0 or 1
Note that for symbol and file name sections, each character is 1 byte (not 2), there is no null
terminator, and each symbol or file name is its own section.
Here is an example of what a binary file might look like (recall generating these in HW9):
CA DE 00 00 00 0C 90 00 D1 40 92 00 94 0A 25 00 0C 0C 66 00 48 01 72 00 10 21 14 BF 0F F8

*/
int ReadObjectFile(char* filename, MachineState* CPU) {

  FILE *file;
  int specifierFirst;
  int specifierSecond;
  unsigned short length;

  file = fopen(filename, "rb");

  //do checks
  if (file == NULL) {
    printf("file is null");
    return -1;
  }

  //read in bits
  specifierFirst = fgetc(file);
  specifierSecond = fgetc(file);

  //as long as it is not the end of the file...
  while (specifierFirst != EOF && specifierSecond != EOF) {

    unsigned short specifier = (specifierFirst << 8) | specifierSecond;

    //check what specifier it is
    if (specifier == 0xCADE) {
      //read in address, length, instructions
      memoryAddress = (fgetc(file) << 8 | fgetc(file));
      length = (fgetc(file) << 8 | fgetc(file));

      for (int i = 0; i < length; i++) {
        //store instructions in memory
        unsigned short inst;
        inst = (fgetc(file) << 8 | fgetc(file));
        unsigned short offset = memoryAddress + i;
        CPU -> memory[offset] = inst;
      }

    } else if (specifier == 0xDADA) {
      //read in address, length, instructions
      memoryAddress = (fgetc(file) << 8 | fgetc(file));
      length = (fgetc(file) << 8 | fgetc(file));

      for (int i = 0; i < length; i++) {
        //store instructions in memory
        unsigned short data;
        data = (fgetc(file) << 8 | fgetc(file));
        unsigned short offset = memoryAddress + i;
        CPU -> memory[offset] = data;
      }

    } else if (specifier == 0xC3B7) {
      //read in address, length, symbol
      memoryAddress = (fgetc(file) << 8 | fgetc(file));
      length = (fgetc(file) << 8 | fgetc(file));
      for (int i = 0; i < length; i++) {
        fgetc(file);
      }

    } else if (specifier == 0xF17E) {
      //read in length, name
      printf("%c\n", fgetc(file));
      printf("%c\n", fgetc(file));
      length = (fgetc(file) << 8 | fgetc(file));
      for (int i = 0; i < length; i++) {
        fgetc(file);
      }
    
    } else if (specifier == 0x715E) {
      //read in address, line, index
      printf("%c\n", fgetc(file));
      printf("%c\n", fgetc(file));
      memoryAddress = (fgetc(file) << 8 | fgetc(file));
      unsigned short line = (fgetc(file) << 8 | fgetc(file));
      unsigned short idx = (fgetc(file) << 8 | fgetc(file));
      //what to do with this

    } else {
      printf("the file header code is invalid: %04X\n", specifier);
      return -1;
    }

    specifierFirst = fgetc(file);
    specifierSecond = fgetc(file);
    
  }

  fclose(file);
  return 0;
  
}
