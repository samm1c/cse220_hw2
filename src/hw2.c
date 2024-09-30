#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <assert.h>

#include "hw2.h"

void print_packet(unsigned int packet[]) // print the packet's information
{
    int type; // 0 for write and 1 for read
    
    //printf("\n\n%d\n\n", packet[1]);
    //printf("\n\n%d\n\n", packet[2]);
    //printf("\n\n%d\n\n", packet[3]);
    //printf("\n\n%d\n\n", packet[4]);
    //printf("\n%d\n", packet[0]);
    //printf("\n%d\n", type_check);
    
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


    printf("Data:");
    if (type == 0) {
        bool keep[4];
        for (int i = 0; i < 4; i++) {
          keep[i] = false;
        }
        // first BE (packet[3])
        //printf("FIRST BE: %d\n", packet[3]);
       printf("\n----\n");
       if ((first_BE & 1) == 1) { // 1 -> 0001 ; first byte
            keep[0] = true;
            //printf(" %d", packet[3] & 511); // 2^9 - 1 = 511 for 1111 1111 as mask for last 8 bits (first byte)
       }
       if ((first_BE & 2) == 2)  { // 2 -> 0010 ; seconnd byte
            keep[1] = true;
            //printf(" %d", (packet[3] >> 8) & 511);
       }
       if ((first_BE & 4) == 4)  { // 4 -> 0100 ; third byte
            keep[2] = true;
            //printf(" %d", (packet[3] >> 16) & 511);
       }
       if ((first_BE & 8) == 8)  { // 8 -> 1000 ; fourth byte
            keep[3] = true;
            //printf(" %d", (packet[3] >> 24) & 511);
       }
     int first_word = packet[3];
      for (int i = 0; i < 4; i++) {
           printf("%d | ", keep[i]);
         }
     int j = 0;
       for (int i = 0; i < 4 && j < 4; i++) {
          if (keep[i]) { // keep going until you hit a false and print the whoel thing out
               j = i; // second counter
               while (j < 4 && keep[j]) {
                    j++;
               }
               first_word = first_word >> (8*j);
               printf(" %d", first_word);
          } // otherwise don't do anything if there's no true
       }
       printf("\n----\n");
       //clear keep list
       for (int k = 0; k < 4; k++) {
          keep[k] = false;
       }
       // last BE (packet[N+3])
       //printf("LAST BE: %d\n", packet[length - 1]);
       if ((last_BE & 1) == 1) { // 1 -> 0001 ; first byte
          keep[0] = true;
            //printf(" %d", packet[length - 1] & 511); // 2^9 - 1 = 511 for 1111 1111 as mask for last 8 bits (first byte)
       }
       if ((last_BE & 2) == 2)  { // 2 -> 0010 ; second byte
            keep[1] = true;
            //printf(" %d", (packet[length - 1] >> 8) & 511);
       }
       if ((last_BE & 4) == 4)  { // 4 -> 0100 ; third byte
            keep[2] = true;
            //printf(" %d", (packet[length - 1] >> 16) & 511);
       }
       if ((last_BE & 8) == 8)  { // 8 -> 1000 ; fourth byte
            keep[3] = true;
            //printf(" %d", (packet[length - 1] >> 24) & 511);
       }
       
        int last_word = packet[length + 3 - 1];
        j = 0;
        for (int i = 0; i < 4 && j < 4; i++) {
          if (keep[i]) {
               j = i;
               while (j < 4 && keep[j]) {
                    j++;
               }
               last_word = last_word >> (8*j);
               printf(" %d", last_word);
          }
        }
    }
    
    printf("\n");

    (void) packet;
}

void store_values(unsigned int packets[], char *memory)
{
    (void)packets;
    (void)memory;
}

unsigned int* create_completion(unsigned int packets[], const char *memory)
{
    (void)packets;
    (void)memory;
	return NULL;
}
