#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <assert.h>

#include "hw2.h"

void print_packet(unsigned int packet[]) // print the packet's information
{
     
     int type; // 0 for write and 1 for read

     // packet type
     printf("Packet Type: ");
     int type_check = packet[0] >> 30;
     if (type_check == 1) {
          type = 0;
          printf("Write\n");
     } else if (type_check == 0) {
          type = 1;
          printf("Read\n");
     } else {
          return; // end function prematurely b/c invalid output
     }
     
     printf("Address: %d\n", packet[2]); // the literal thing
     
     int length = packet[0] & 1023; // 1023 = 2^10 - 1 is the mask of 1's to only keep the last 9 bits in packet[0]
     printf("Length: %d\n", length);
     
     int id = packet[1] >> 16; // cut last 16 bits to get only ID
     printf("Requester ID: %d\n", id);

     int tag = (packet[1] >> 8) & 255; // cut the last and first BEs; mask for 2^8 - 1 = 255
     printf("Tag: %d\n", tag);

     int last_BE = (packet[1] >> 4) & 15; // cut last 4 of first BE; mask for last 4 bits using 2^4 - 1 = 15
     printf("Last BE: %d\n", last_BE);

     int first_BE = packet[1] & 15; // mask for last 4 bits
     printf("1st BE: %d\n", first_BE);

     printf("Data: ");
     if (type == 0) {
          for (int i = 3; i <= length + 3 - 1; i++) {
               printf("%d ", packet[i]);
          }
     }
     printf("\n");
}

void store_values(unsigned int packets[], char *memory)
{
     int i = 0; // index of packets
     int length, address, last_BE, first_BE;
     bool valid_packet = true;
     while (valid_packet) {
          if ((packets[i] >> 30) != 1) { // check if valid write request
               return;
          }
          length = packets[i] & 1023; // 2^10 - 1 = 1023
          i++; // end of int[0]
          last_BE = (packets[i] >> 4) & 15; // 2^4 - 1 = 15
          first_BE = packets[i] & 15;
          i++; // end of int[1]
          if (packets[i] > 1000000) { // check address does not exceed 1MB memory
               return;
          } else {
               address = packets[i];
          }
          i++; // end of int[2]
          // end of parsing through header

          int m = address; // index for memory's address -> [address, address + (length*4)]

          for (int a = 0; a < length; a++) { // a -> number of packet rows to read; a is not going to be used or relevant besides tracking first and last BE
               
               int data = packets[i];
               if (a == 0 || a == length - 1) { // first or last BE
                    int byte_enable; // set byte_enable value
                    if (a == 0) {
                         byte_enable = first_BE;
                    } else if (a == length - 1) {
                         byte_enable = last_BE;
                    }

                    for (int k = 0; k < 4; k++) { // 4 times; k has no bearing
                         if ((byte_enable & 1) == 1) {
                              memory[m] = data & 255; // extract last 8 bits
                         }
                         byte_enable = byte_enable >> 1; // cut last bit
                         data = data >> 8; // get rid of last 8 bits = 1 byte that we read
                         m++; // memory index
                    }
                    i++; // bookkeeping; next row in packets

               } else { // middle elements
                    for (int k = 0; k < 4; k++) { // 4 times, k has no bearing
                         memory[m] = data & 255; // extract last 8 bits = 1 byte
                         data = data >> 8; // get rid of last 8
                         m++; // next memory
                    }
                    i++; // next row in packet
               }
          }
          // finish checking the data
     }
}

unsigned int* create_completion(unsigned int packets[], const char *memory)
{
    (void)packets;
    (void)memory;
	return NULL;
}


// bool keep[4]; // tracks which bits to keep
                    // for (int k = 0; k < 4; k++) { //
                    //      keep[k] = false;
                    // }
                    // if (a == address) { // first BE
                    //      if ((first_BE & 1) == 1) { // 1 -> 0001 ; first byte
                    //           keep[0] = true;
                    //      }
                    //      if ((first_BE & 2) == 2)  { // 2 -> 0010 ; second byte
                    //           keep[1] = true;
                    //      }
                    //      if ((first_BE & 4) == 4)  { // 4 -> 0100 ; third byte
                    //           keep[2] = true;
                    //      }
                    //      if ((first_BE & 8) == 8)  { // 8 -> 1000 ; fourth byte
                    //           keep[3] = true;
                    //      }
                    //      int first_word = packets[i]; // variable needed b/c will use bitwise operators on it
                    //      int b = 0;
                    //      for (int a = 0; a < 4 && b < 4; a++) {
                    //           if (keep[a]) { // keep going until you hit a false and store whole thing
                    //                b = a; // second counter
                    //                while (b < 4 && keep[b]) {
                    //                     b++;
                    //                }
                    //                first_word = first_word >> (8*b); // skip b bytes
                    //                ((unsigned int *)memory)[j] = first_word;
                    //           } // otherwise don't do anything if there's no true
                    //      }
                    // } else if (a == length - 1) {
                    //      if ((last_BE & 1) == 1) { // 1 -> 0001 ; first byte
                    //           keep[0] = true;
                    //      }
                    //      if ((last_BE & 2) == 2)  { // 2 -> 0010 ; second byte
                    //           keep[1] = true;
                    //      }
                    //      if ((last_BE & 4) == 4)  { // 4 -> 0100 ; third byte
                    //           keep[2] = true;
                    //      }
                    //      if ((last_BE & 8) == 8)  { // 8 -> 1000 ; fourth byte
                    //           keep[3] = true;
                    //      }
                    //      int last_word = packets[i];
                    //      int b = 0;
                    //      for (int a = 0; a < 4 && b < 4; a++) { // same as before
                    //           if (keep[a]) {
                    //                b = a;
                    //                while (j < 4 && keep[b]) {
                    //                     b++;
                    //                }
                    //                last_word = last_word >> (8*b);
                    //                ((unsigned int *)memory)[j] = last_word;
                    //           }
                    //      }
                    // }
                    // i++; // move to next row in packet