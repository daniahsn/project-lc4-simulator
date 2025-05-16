/*
 * loader.c : Defines loader functions for opening and loading object files
 */

#include "loader.h"


// helper function to convert endianness of a word
unsigned short int swap_bytes(unsigned short int x) {
	short int upper, lower, final;
	upper = (x & 0xFF00) >> 8; // get the upper byte and shift to lower byte's position
	lower = (x & 0x00FF) << 8; // get the lower byte and shift to upper byte's position
	final = upper | lower; // bitwise or to combine the swapped bytes
	return final;
}

// memory array location
unsigned short memoryAddress;

int ReadObjectFile(char* filename, MachineState* CPU) {

    FILE* file = fopen(filename, "rb");
    if (file == NULL) {
        fprintf(stderr, "Error: Could not open file %s\n", filename);
        return -1;
    }

    unsigned short section, address, n, word, i, fileIndex;
    char character;

    while (fread(&section, sizeof(unsigned short), 1, file) == 1) {
        section = swap_bytes(section); 

        switch (section) {
            case 0xCADE: {
                // Code section
                fread(&address, sizeof(unsigned short), 1, file); //get address
                fread(&n, sizeof(unsigned short), 1, file); // get #words
                address = swap_bytes(address);
                n = swap_bytes(n);
                
                // read next n lines of code and store starting from the specified address
                for (i = 0; i < n; i++) {
                    fread(&word, sizeof(unsigned short), 1, file);
                    CPU->memory[address + i] = swap_bytes(word); //since CPU is a POINTER to the structure
                }
                break;
             }

            case 0xDADA: {
                // Data section
                fread(&address, sizeof(unsigned short), 1, file); //get address
                fread(&n, sizeof(unsigned short), 1, file); // get #words
                address = swap_bytes(address);
                n = swap_bytes(n);
                
                // read next n lines of code and store starting from the specified address
                for (i = 0; i < n; i++) {
                    fread(&word, sizeof(unsigned short), 1, file);
                    CPU->memory[address + i] = swap_bytes(word); //since CPU is a POINTER to the structure
                }
                break;
            }
            case 0xC3B7: {
                // Symbol section
                fread(&address, sizeof(unsigned short), 1, file);
                fread(&n, sizeof(unsigned short), 1, file);
                address = swap_bytes(address);
                n = swap_bytes(n);

                // read next n lines of code 
                for (i = 0; i < n; i++) {
                    // skip filling the array
                    fread(&character, sizeof(char), 1, file);
                }
                break;
            }			
            case 0xF17E: {
                // File name section
                fread(&n, sizeof(unsigned short), 1, file);
                n = swap_bytes(n);

                 // read next n lines of code 
                for (i = 0; i < n; i++) {
                    // skip filling the array
                    fread(&character, sizeof(char), 1, file);
                }
                break;
            }
            case 0x715E: {
                // Line number section
                fread(&address, sizeof(unsigned short), 1, file);
                fread(&word, sizeof(unsigned short), 1, file);
                fread(&fileIndex, sizeof(unsigned short), 1, file);
                address = swap_bytes(address);
                word = swap_bytes(word);
                fileIndex = swap_bytes(fileIndex);
                
                // no body
                break;
            }
            default: {
                // Unknown section type, skip
                fprintf(stderr, "Warning: Unknown section type 0x%04X\n", word);
                break;
            }
         }
      }
    fclose(file);
    return 0;
}