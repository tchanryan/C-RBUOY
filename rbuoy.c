////////////////////////////////////////////////////////////////////////
// COMP1521 24T2 --- Assignment 2: `rbuoy', a simple file synchroniser
// <https://cgi.cse.unsw.edu.au/~cs1521/24T2/assignments/ass2/index.html>
//
// Written by Ryan Tchan (z5258155) on 20/07/24.
// This program implements features which help a user transfer files locally whereby a 'sender' and 
// 'receiver' are two different directories within the user's repository. Files are completely 
// transfered or updated as necessary during this transition. 
//
// 2023-07-12   v1.0    Team COMP1521 <cs1521 at cse.unsw.edu.au>


#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "rbuoy.h"

/////////////////////////////////// FUNCTION PROTOTYPES /////////////////////////////////////////////
// SUBSET 1
void make_tabi(FILE *out_file);
void num_records(FILE *out_file, uint8_t num_record);
void make_little_endian(FILE *out_file, size_t target, int bytes);
void write_pathname(FILE *out_file, char *pathname, size_t path_len);
size_t input_size(FILE *out_file, FILE *in_file, char *pathname);

// SUBSET 2
void make_tbbi(FILE *out_file);
void tabi_check(FILE *in_file);
void read_little_endian(FILE *in_file, size_t *target, int bytes);
void read_pathname(FILE *in_file, char *pathname, size_t path_len);
void write_match_bytes(FILE *out_file, uint8_t *matches, size_t num_match);



/// @brief Create a TABI file from an array of pathnames.
/// @param out_pathname A path to where the new TABI file should be created.
/// @param in_pathnames An array of strings containing, in order, the files
//                      that should be placed in the new TABI file.
/// @param num_in_pathnames The length of the `in_pathnames` array. In
///                         subset 5, when this is zero, you should include
///                         everything in the current directory.
void stage_1(char *out_pathname, char *in_pathnames[], size_t num_in_pathnames) {
    // TODO: implement this.

    // Hint: you will need to:
    //   * Open `out_pathname` using fopen, which will be the output TABI file.
    //   * For each pathname in `in_pathnames`:
    //      * Write the length of the pathname as a 2 byte little endian integer
    //      * Write the pathname
    //      * Check the size of the input file, e.g. using stat
    //      * Compute the number of blocks using number_of_blocks_in_file
    //      * Write the number of blocks as a 3 byte little endian integer
    //      * Open the input file, and read in blocks of size BLOCK_SIZE, and
    //         * For each block call hash_black to compute the hash
    //         * Write out that hash as an 8 byte little endian integer
    // Each time you need to write out a little endian integer you should
    // compute each byte using bitwise operations like <<, &, or |

    FILE *out_file = fopen(out_pathname, "w");
    if (out_file == NULL) {
        perror("Failed to write file.tabi");
        exit(1);
    }
    
    make_tabi(out_file);
    make_little_endian(out_file, 0, 1);

    size_t num_record = 0;
    for (size_t i = 0; i < num_in_pathnames; i++) {
        char *pathname = in_pathnames[i];
        FILE *in_file = fopen(pathname, "r");
        if (in_file == NULL) {
            perror("Failed to read string array");
            exit(1);
        }

        int len_path = strlen(pathname);
        make_little_endian(out_file, len_path, PATHNAME_LEN_SIZE);
        write_pathname(out_file, pathname, len_path);
        size_t file_size = input_size(out_file, in_file, pathname);
        size_t num_blocks = number_of_blocks_in_file(file_size);
        make_little_endian(out_file, num_blocks, NUM_BLOCKS_SIZE);

        char bytes[BLOCK_SIZE];
        size_t bytes_read;
        while ((bytes_read = fread(bytes, 1, BLOCK_SIZE, in_file)) > 0) {
            uint64_t hash = hash_block(bytes, bytes_read);
            make_little_endian(out_file, hash, HASH_SIZE);
        }

        fclose(in_file);
        num_record++;
    }
    fseek(out_file, 4, SEEK_SET);
    num_records(out_file, num_record);

    fseek(out_file, 0, SEEK_END);
    fclose(out_file);
}

/// @brief Create a TBBI file from a TABI file.
/// @param out_pathname A path to where the new TBBI file should be created.
/// @param in_pathname A path to where the existing TABI file is located.
void stage_2(char *out_pathname, char *in_pathname) {
    // TODO: implement this. -> up to match bytes section

    // open a valid senders file and create TBBI output file
    FILE *out_file = fopen(out_pathname, "w");
    if (out_file == NULL) {
        perror("Failed to write file.TBBI");
        exit(1);
    }

    FILE *in_file = fopen(in_pathname, "r");
    if (in_file == NULL) {
        perror("Failed to read or find file.TABI");
        exit(1);
    }

    tabi_check(in_file);
    make_tbbi(out_file);

    // check receiver records to TABI records 
    size_t receiver_records = 0;
    size_t num_records = 0;
    read_little_endian(in_file, &num_records, NUM_RECORDS_SIZE);
    make_little_endian(out_file, num_records, NUM_RECORDS_SIZE);

    // loop through each record in TABI
    for (size_t i = 0; i < num_records; i++) {
        size_t path_len;
        read_little_endian(in_file, &path_len, PATHNAME_LEN_SIZE); 
        make_little_endian(out_file, path_len, PATHNAME_LEN_SIZE);

        char *pathname = malloc((path_len + 1));
        if (pathname == NULL) {
            perror("Error: Failed to allocate memory for pathname");
            exit(1);
        }
        
        read_pathname(in_file, pathname, path_len);
        pathname[path_len] = '\0';
        write_pathname(out_file, pathname, path_len);
        //printf("name: %s\n" , pathname);
    
        size_t block_num;
        read_little_endian(in_file, &block_num, NUM_BLOCKS_SIZE);
        make_little_endian(out_file, block_num, NUM_BLOCKS_SIZE);
        //printf("block: %zu\n", block_num);
        if (block_num <= 0) {
            perror("Error: block_num is not valid");
            exit(1);
        }
        // size_t size_of_file = input_size(out_file, in_file, pathname);
        // size_t block_num = number_of_blocks_in_file(size_of_file);

        size_t num_match = num_tbbi_match_bytes(block_num);
        uint8_t *matches = (uint8_t *)calloc(num_match, sizeof(uint8_t));
        if (matches == NULL) {
            perror("Error: Failed to allocate memory for match bytes");
            exit(1);
        }

        // open up receivers file for comparison and compare each block (hash)
        FILE *receivers_file = fopen(pathname, "r");
        if(receivers_file == NULL) {
            write_match_bytes(out_file, matches, num_match);
            receiver_records++;
            continue;
        } 

        char *block = malloc(BLOCK_SIZE);
        if (block == NULL) {
            perror("Error: Failed to allocate memory for block");
            exit(1);
        }

        size_t block_index = 0;
        size_t block_size;
        while ((block_size = fread(block, 1, BLOCK_SIZE, receivers_file)) > 0) {
            uint64_t hash = hash_block(block, block_size);
            size_t match_byte_num = block_index >> 3;
            size_t bit = MATCH_BYTE_BITS - 1 - (block_index & (MATCH_BYTE_BITS - 1));

            if (block_index < block_num) {
                uint64_t expected_hash;
                read_little_endian(in_file, &expected_hash, HASH_SIZE);
                if (hash == expected_hash) {
                    matches[match_byte_num] |= (1 << bit);
                }
            }

            block_index++;
        }

        fclose(receivers_file);
        write_match_bytes(out_file, matches, num_match); 
        receiver_records++;
    }

    if (receiver_records > num_records) {
        fprintf(stderr, "Error: the number of records are too small");
        exit(1);
    }

    fclose(in_file);
    fclose(out_file);
}
// a helper function that makes the tbbi file
void make_tbbi(FILE *out_file) {
    uint32_t magic_bytes = 0x49424254;
    uint8_t mask = 0xFF;
    for(int i = 0; i < 4; i++) {
        uint32_t temp = magic_bytes >> (i * 8);
        fputc(temp & mask, out_file);
    }
}

// a helper function that validates the input file is a TABI file
void tabi_check(FILE *in_file) {
    size_t tabi_hex = 0;
    read_little_endian(in_file, &tabi_hex, 4);

    if (tabi_hex != 0x49424154) {
        fprintf(stderr, "Error: Input file is not in TABI format");
        exit(1);
    }
} 

// a helper function that reads little-endian values and stores it in the target
void read_little_endian(FILE *in_file, size_t *target, int bytes) {
    *target = 0;

    for (int i = 0; i < bytes; i++) {
        int byte = fgetc(in_file);
        if (byte == EOF) {
            fprintf(stderr, "Error: Unexpected EOF");
            exit(1);
        }
        *target |= (size_t)byte << (i * 8);
    }
}

// a helper function that writes the pathname of the file to the out_file
void read_pathname(FILE *in_file, char *pathname, size_t path_len) {
    size_t name_byte = fread(pathname, 1, path_len, in_file);
    if (name_byte != path_len) {
        //printf("%zu \n path: %zu\n", name_byte, path_len);
        fprintf(stderr, "Error: Failed to read pathname");
        exit(1);
    }
}

// a helper function that computes the match bytes
void write_match_bytes(FILE *out_file, uint8_t *matches, size_t num_match) {
    if(fwrite(matches, 1, num_match, out_file) != num_match) {
        perror("Error: Failed to write match bytes");
        exit(1);
    }
}

/// @brief Create a TCBI file from a TBBI file.
/// @param out_pathname A path to where the new TCBI file should be created.
/// @param in_pathname A path to where the existing TBBI file is located.
void stage_3(char *out_pathname, char *in_pathname) {
    // TODO: implement this.
}


/// @brief Apply a TCBI file to the filesystem.
/// @param in_pathname A path to where the existing TCBI file is located.
void stage_4(char *in_pathname) {
    // TODO: implement this.
}


//////////////////////////////////// STAGE 1 HELPER FUNCTIONS ////////////////////////////////////
// a helper function that formats the file to be TABI
void make_tabi(FILE *out_file) {
    uint32_t magic_bytes = 0x49424154;
    uint8_t mask = 0xFF;
    for(int i = 0; i < 4; i++) {
        uint32_t temp = magic_bytes >> (i * 8);
        fputc(temp & mask, out_file);
    }
}

// a helper function that writes the number of records within a TABI file
void num_records(FILE *out_file, uint8_t num_record) {
    fputc(num_record, out_file);
}

// a helper function that writes to a file in little endian using specified bytes
void make_little_endian(FILE *out_file, size_t target, int bytes) {
    uint8_t mask = 0xFF;
    for (int i = 0; i < bytes; i++) {
        size_t temp = target >> (i * 8);
        fputc(temp & mask, out_file);
    }
}

// a helper function that writes the pathname of the file to the out_file
void write_pathname(FILE *out_file, char *pathname, size_t path_len) {
    if (fwrite(pathname, 1, path_len, out_file) != path_len) {
        perror("Error: Failed to write pathname");
        exit(1);
    }
}

// a helper function that checks the size of the input file using stat and closes the program if error
size_t input_size(FILE *out_file, FILE *in_file, char *pathname) {
    struct stat s;
    if (stat(pathname, &s) != 0) {
        perror(pathname);
        fclose(out_file);
        fclose(in_file);
        exit (1);
    }

    return s.st_size;
}

//////////////////////////////////// STAGE 2 HELPER FUNCTIONS ////////////////////////////////////
