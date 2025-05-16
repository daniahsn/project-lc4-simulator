/*
 * trace.c: location of main() to start the simulator
 */

#include "loader.h"

// Global variable defining the current state of the machine
MachineState* CPU;
static MachineState CPUState;


int main(int argc, char** argv) {
    // Check command line arguments
    if (argc < 3) {
        fprintf(stderr, "Invalid arguments. \n");
        return -1;
    }

    // Point CPU to the statically allocated CPUState
    CPU = &CPUState;

    // open output file 
    FILE* out_file = fopen(argv[1], "wb");
        if (out_file == NULL) {
            fprintf(stderr, "Error: Could not open file %s\n", argv[1]);
            return -1;
        }

    // Initialize memory to zero 
    memset(CPU->memory, 0, sizeof(CPU->memory));

    // Iterate over the .OBJ files
    for (int i = 2; i < argc; i++) {
        FILE* file = fopen(argv[i], "rb");
        if (file == NULL) {
            fprintf(stderr, "Error: Could not open file %s\n", argv[i]);
            // Return error if any file cannot be opened
            return -1;
        }

        // Load the object file into the simulator's memory
        if (ReadObjectFile(argv[i], CPU) != 0) {
            fprintf(stderr, "Error: Failed to read object file %s\n", argv[i]);
            // Return error if loading fails
            return -1;
        }
        fclose(file);
    }
   
    for (int address = 0x8200; address < 0x8205; address++) {
        printf("address: %05X contents: 0x%04X\n", address, CPU->memory[address]);
    }
    
    for (int address = 0; address < 5; address++) {
        printf("address: %05d contents: 0x%04X\n", address, CPU->memory[address]);
    }

    unsigned short int currentPC = 0x8200;
    CPU->PC = 0x8200;
    CPU->PSR = 0x8002;

    while (currentPC != 0x80FF && UpdateMachineState(CPU, out_file) == 0) {
        currentPC = CPU->PC;
    }

    fclose(out_file);
    return 0;
} 