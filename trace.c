/*
 * trace.c: location of main() to start the simulator
 */

#include "loader.h"

// Global variable defining the current state of the machine
MachineState* CPU;

int main(int argc, char** argv) {

  if( argc < 3 ) { 
      printf("invalid number of files\n");
			return -1;
  }

  char* filename = argv[1];
  FILE *fp = fopen(filename, "w");
  if (filename == NULL || fp == NULL) {
    printf("file does not exist\n");
    fclose(fp);
    return -1;
  }

  //initialize CPU values to null
  MachineState machine;
  CPU = &machine;
  Reset(CPU);

  //check if all files exist and read if they do
  for (int i = 2; i < argc; i++) {
    char* filename = argv[i];
    FILE *test = fopen(filename, "rb");
    if (test == NULL) {
      printf("file does not exist\n");
      fclose(test);
      return -1;
    }
    ReadObjectFile(filename, CPU);
  }

  while (CPU -> PC != 0x80FF) {
    int result = UpdateMachineState(CPU, fp);
    if (result == -1) {
      printf("failed and returned at main");
      return -1;
    }
  }

  fclose(fp);
  return 0;
}