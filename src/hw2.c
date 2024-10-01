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
     int c = 0; // index of completion packets
     int m = 0; // index of memory array
     int num_read = 0; // number of packets from read requests
     int num_comp = 0; // number of packets in the completion
     
     int length, id, tag, last_BE, first_BE, address; // variables of read request
     int byte_count, lower_address, split_length, rem; // variables of completion packet
     int total_length = 0; // total lengths of all packets

     bool valid_packet = true; // to loop though each packet one by one

     // figure out how much space we should malloc first including header, payload, and number of packets (plus those we're splitting)
     while (valid_packet) {
          if ((packets[i] >> 10) != 0) { // NOT a valid read request thus end prematurely
                break;
          }
          length = packets[i] & 1023; // length for only this row
          total_length += packets[i] & 1023; // add length
          i += 2; // skip int[1] into int[2] for address
          address = packets[i];
          if (address > 1000000) { // check for invalid address
               break;
          }
          i++;

          // boundaries are split by 0x4000 -> 0x4000, 0x8000, 0xC000
          int start_boundary = address / 0x4000;
          int end_boundary = (address + (length * 4) - 1) / 0x4000; // length*4 -> number of total bits to extend from address; - 1 is to include the index you stopped at

          num_comp += end_boundary - start_boundary + 1; // difference in boundaries gives you the number of packets you need for completion (including split)
          num_read++; // number of normal read request packets
     }

     i = 0; // reset packet index
     m = address; // make sure index of memory pointer we are reading data from is at the right index
     // create and initialize the completion packets array (size does not need to be fixed b/c it's malloc'ed)
     //unsigned int* completion = (unsigned int*)malloc(20 * (((sizeof(int) * 3 * num_comp)) + (sizeof(int) * total_length))); // header + payload
     unsigned int* completion = (unsigned int*)malloc(sizeof(int)*89); // header + payload
     //unsigned int* completion = (unsigned int*)malloc(1000000 * (3 * num_comp) + (sizeof(int) * total_length)); // header + payload

     // parse through read request and create completion packet(s) based off it
     for (int r = 0; r < num_read; r++) { // r (number of read request packets) times; r has no bearing
          int n = 0; // keeps track of row to check if it's the first or last row for BE
          // headers
          length = packets[i] & 1023;
          i++; // end of int[0]
          id = packets[i] >> 16; // cut last 16 bits
          tag = (packets[i] >> 8) & 255; // cut last 8; only take last 8 for 2^8 - 1 = 255
          last_BE = (packets[i] >> 4) & 15; // cut last 4; only take last 4 for 2^4 - 1 = 15
          first_BE = packets[i] & 15; // 2^4 - 1 = 15
          i++; // end of int[1]
          address = packets[i];
          i++; // end of int[2]; ready to read the next read request

          // completion packets
          // use address and length to tell if it's crossed 0x4000 boundary; if so, must create multiple packets
          int start_boundary = address / 0x4000;
          int end_boundary = (address + (length * 4) - 1) / 0x4000;
          int p = end_boundary - start_boundary + 1; // will give total number of packets to create; it's either 1 or 2
          bool split;          
          if (p == 1) { // single packet; no need to split 
              split = false;
          } else { // multiple packets (max 2) needed; must split
              split = true;
          }

          // produce completion packets
          for (int k = 0; k < num_read; k++) { // p (number of packets) times; k has no bearing
               
               // requester id, tag, and completer id are the same
               completion[c] = ((37) << 25); 
               completion[c + 1] = (220 << 16);
               completion[c + 2] = (id << 16) | (tag << 8);
               
               if (split) { // split DOES occur -> multiple packets
                    rem = 0x4000 - (address % 0x4000); // take index 14 bit for the remaining (how far up the boundary you're at) 
                                                            // and subtract 0x4000 from that to get the actual remaining number of addresses 
                                                        // to the next boundary
                    split_length = rem / 4; // how long the split for this packet will be
                    address += rem; // add the leftover remaining to reach the next boundary
                    lower_address = address & 0x7F; // because 0x7F = 0000 0111 1111 for last 7 bits               
                    //printf("rem: %d\n", rem);
                    //printf("length: %d\n", length);

                    // now that you calculated everything you need to update header
                    completion[c] |= split_length;
                    c++; // end of int[0]
                    completion[c] |= byte_count;
                    c++; // end of int[1]
                    completion[c] |= lower_address;
                    c++; // end of int[2]
                    printf("split_length: %d\n", split_length);

                    // load the data into the payload
                    for (int a = 0; a < split_length; a++) { // iterate over rows
                         if (n == 0 || n == length - 1) { // first or last BE
                              int byte_enable;
                              if (a == 0) {
                                   byte_enable = first_BE;
                              } else if (a == length - 1) {
                                   byte_enable = last_BE;
                              }
                              for (int b = 0; b < 4; b++) { // per row
                                   if ((byte_enable & 1) == 1) {
                                        completion[c] |= memory[m] << (b * 4);
                                   }
                                   byte_enable = byte_enable >> 1; // cut last bit
                                   m++;
                              }
                         } else { // no need for BE; just do it normally
                              for (int b = 0; b < 4; b++) {
                                   completion[c] |= memory[m] << (b * 4);
                                   m++;
                              }
                         }
                         c++; n++;
                         //printf("c: %d\n", c); 
                    }
                    byte_count -= rem; // update the total leftover number of bytes
               } else { // split does NOT occur; single packet
                    // finish header
                    completion[c] |= length;
                    c++; // end of int[0]
                    completion[c] |= (length * 4);
                    c++;
                    completion[c] |= address & 0x7f;
                    c++;
                    // load data from memory
                    for (int a = 0; a < length; a++) {
                         if (n == 0 || n == length - 1) { // first or last BE
                              int byte_enable;
                              if (a == 0) {
                                   byte_enable = first_BE;
                              } else if (a == length - 1) {
                                   byte_enable = last_BE;
                              }
                              printf("c:%d \t completion[c]:%d\n", c, completion[c]);
                              completion[c] = 0;
                              for (int b = 0; b < 4; b++) { // per row
                                   if ((byte_enable & 1) == 1) {
                                        completion[c] |= ((unsigned char)memory[m] << (b * 8));
                                        //printf("c:%d \t completion[c]:%d \tmemory[m]:%d \t shifted:%d\n", c, completion[c], memory[m], (20 | (memory[m] << (b * 9))));
                                   }
                                   byte_enable = byte_enable >> 1; // cut last bit
                                   m++;
                              }
                         } else { // no need for BE; just do it normally
                              for (int b = 0; b < 4; b++) {
                                   completion[c] |= ((unsigned char)memory[m] << (b * 8));
                                   m++;
                              }
                         }
                         c++; n++; // both are entire rows
                    }
               }
          }
          // done creating completion packets so now read the next read request
     }

     // finally return ALL of the completion packets
     return completion;
     return (void *)memory;
}
               // so basically i have my number of packets to create in completion 
               // so the next step is actually creating those packets
               // splitting packets AND single packets,, not sure if i want to separate them but im putting into singular for loop as shown
               // have not done BE yet
               // 
               // lower address relatively easy wher you just take the last 7 bits and return its value
               // need to take data from memory and put into completion