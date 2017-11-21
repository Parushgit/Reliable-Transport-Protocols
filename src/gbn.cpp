#include "../include/simulator.h"
#include <vector>
#include <queue>
#include <iostream>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
using namespace std;
queue <msg> buffer_msg;
queue <msg> buffer_msg_copy;

/* ******************************************************************
 ALTERNATING BIT AND GO-BACK-N NETWORK EMULATOR: VERSION 1.1  J.F.Kurose

   This code should be used for PA2, unidirectional data transfer 
   protocols (from A to B). Network properties:
   - one way network delay averages five time units (longer if there
     are other messages in the channel for GBN), but can be larger
   - packets can be corrupted (either the header or the data portion)
     or lost, according to user-defined probabilities
   - packets will be delivered in the order in which they were sent
     (although some can be lost).
**********************************************************************/

/********* STUDENTS WRITE THE NEXT SEVEN ROUTINES *********/
int base, swindow, nextseqnum, base_copy, B_sequence_number, counter;
float delay_RTT;//20.0;
static vector<struct pkt *> packet_vector;

int checksum(struct pkt packet){
    char cpy_msg[20];
    strcpy(cpy_msg,packet.payload);
    int sum = 0;
    for(int i = 0; i<20 && cpy_msg[i] != '\0'; i++)
    {
        sum += cpy_msg[i];
    }

    sum = sum + packet.seqnum;

    sum = sum + packet.acknum;

    return sum;
}

struct pkt *create_packet(struct msg message, int sequence_number)
{
  struct pkt *packet = (struct pkt *)malloc(sizeof(struct pkt));
  (*packet).acknum = -1;
  (*packet).seqnum = sequence_number;
  strcpy((*packet).payload,message.data);
  (*packet).checksum = checksum((*packet));
  return packet;
}

/* called from layer 5, passed the data to be sent to other side */
void A_output(struct msg message)
{
  struct pkt *packet = create_packet(message, nextseqnum);
  packet_vector.push_back(packet);
  if(base==nextseqnum)
    starttimer(0,delay_RTT);  
  if(nextseqnum < base + swindow)
     tolayer3(0,*packet_vector.at(nextseqnum-1));
  nextseqnum++;
}

/* called from layer 3, when a packet arrives for layer 4 */
void A_input(struct pkt packet)
{
  base_copy = base;
  if(checksum(packet) == packet.checksum && packet.acknum >= base)
  {
     if( base < packet.acknum + 1)
     {
        base = packet.acknum + 1;
        stoptimer(0);
     }
     int i = base_copy+swindow;
     while(i<nextseqnum && i<base+swindow)
     {
      tolayer3(0,*packet_vector.at(i-1));
      i++;
     }
     starttimer(0,delay_RTT);
  }
}

/* called when A's timer goes off */
void A_timerinterrupt()
{
  int lastrange = base + swindow;
  int i = base;
  while(i < lastrange && i < nextseqnum)
  {
    tolayer3(0, *packet_vector.at(i-1));
    i++;
  }
  starttimer(0,delay_RTT);
}  

/* the following routine will be called once (only) before any other */
/* entity A routines are called. You can use it to do any initialization */
void A_init()
{
  swindow = getwinsize();
  base = 1;
  nextseqnum = 1;
  counter = 1;
  delay_RTT = 25.0;
}

/* Note that with simplex transfer from a-to-B, there is no B_output() */

/* called from layer 3, when a packet arrives for layer 4 at B*/
void B_input(struct pkt packet)
{
  if(packet.checksum == checksum(packet) && packet.seqnum == B_sequence_number)
  {
    //printf("Hello correct ACK\n");
    tolayer5(1, packet.payload);
    struct pkt *ack = (struct pkt *)malloc(sizeof(struct pkt));
    ack->seqnum = 0;
    ack->acknum = B_sequence_number; 
    ack->checksum = checksum(*ack);//B_sequence_number;
    for(int i = 0; i < sizeof(ack->payload); i++)
    ack->payload[i] = '\0';
    tolayer3(1, *ack);
    B_sequence_number+=1;
  }
  else if(checksum(packet) == packet.checksum && packet.seqnum < B_sequence_number)
  {
     //printf("Hello duplicate ACK\n");
     struct pkt *ack = (struct pkt *)malloc(sizeof(struct pkt));
     ack->seqnum = 0;
     ack->acknum = B_sequence_number - 1; 
     ack->checksum = checksum(*ack);//B_sequence_number - 1;
     for(int i = 0; i < sizeof(ack->payload); i++)
     ack->payload[i] = '\0';
     tolayer3(1, *ack);
  }
}

/* the following rouytine will be called once (only) before any other */
/* entity B routines are called. You can use it to do any initialization */
void B_init()
{
B_sequence_number = 1;
}

