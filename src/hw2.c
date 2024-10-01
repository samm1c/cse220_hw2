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
          // finished checking the data
     }
}

unsigned int* create_completion(unsigned int packets[], const char *memory)
{
     int i = 0; // index of packets
     int m = 0; // index of *memory
     int c = 0; // index of completion packets array
     // int p = 0; // number of packets
     // int num_data = 0; // number of total data

     int length, id, tag, last_BE, first_BE, address;
     int total_length = 0; // must calculate the total number of bytes needed for completion packets first before doing anything else
     bool valid_packet = true;

     // calculate total_length and check valid packet
     int j = 0; // special index for this loop (calculating length)
     while (valid_packet) {
          if (packets[j] >> 10 != 0) { // int[0] must all be 0 after the length; check for valid read type packet
               return NULL; // error
          }
          total_length += packets[j] & 1023; // 2^10 - 1 b/c you only want last 10 bits for length
          j += 3; // skip rest of the header into the next packet
     }

     // create and initialize completion packets array (size does not need to be fixed b/c we are malloc'ing it)
     unsigned int *completion = (unsigned int *)malloc(10 * sizeof(int) * total_length); 

     while (valid_packet) {
          
          length = packets[i] & 1023; // 2^10 - 1 = 1023 b/c you only want last 10 bits for length
          i++; // done with int[0]
          id = packets[i] >> 16;
          tag = (packets[i] >> 8) & 255; // get rid of last 8 bits; 2^8 - 1 = 255 for last 8 bits
          last_BE = (packets[i] >> 4) & 15; // get rid of last 4; 2^4 - 1 = 15 for last 4 bits
          first_BE = packets[i] & 15; // 2^4 - 1 = 15 for last 4 bits
          i++; // done with int[1]
          if (packets[i] > 1000000) { // check address does not exceed 1MB memory
               return NULL;
          }
          address = packets[i];
          i++; // done with int[2]
          // finished reading the read request

          completion[c] = ((37) << 25) | length; // hopefully this means that im shifting 37 25 bits to the left and add the length to the last 10
          c++; // done with completion int[0]
          completion[c] = (220 << 16) | (length * 4); // shift 220 to the left 16 bits and add the byte count to the last 12
          c++; // done with completion int[1]
          completion[c] = (id << 16) | (tag << 8) | (length * 4);
          c++; // done with completion int[2]

          m = address;

          // start reading memory array into completion packet
          for (int a = 0; a < length; a++) { // length times; a has no bearing
               
               if (a == 0 || a == length - 1) {
                    int byte_enable;
                    if (a == 0) {
                         byte_enable = first_BE;
                    } else if (a == length - 1) {
                         byte_enable = last_BE;
                    }

                    for (int k = 0; k < 4; k++) { // 4 times; k has no bearing
                         if ((byte_enable & 1) == 1) {
                              completion[c] |= (memory[m] << (8 * k));
                         }
                         byte_enable = byte_enable >> 1;
                         m++;
                    }
                    c++;
               } else {
                    for (int k = 0; k < 4; k++) { // 4 times; k has no bearing
                         completion[c] |= (memory[m] << (8 * k));
                         m++;
                    }
                    c++;
               }

          }

          //p++; // this is a valid packet
     }
     //create completion packets for this packet you're reading and return them by using malloc() so that they can persist
     return completion;

    //(void)packets;
    //(void)memory;
	//return NULL;
}
