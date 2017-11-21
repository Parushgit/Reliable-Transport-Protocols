#include "../include/simulator.h"
#include<iostream>
#include<string.h>
#include<stdio.h>
#include<stdlib.h>
#include<vector>
#include<queue>
using namespace std;
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

struct pkt packet_to_send;
struct pkt ack_B;
float increment;//15.0;
struct pkt created_packet;
float simulation_time;
bool ACK_A = true;
bool msg_transit = false;
queue <msg> buffer_msg;
queue <msg> buffer_msg_copy;
int A_sequence_number;
int B_sequence_number;
int last_sequence_number;
int buf_flag = 0;

int checksum(struct pkt packet_to_send)
{
    char msg_cpy[20];
    int i = 0;
    while(i<20)
    {
      msg_cpy[i] = packet_to_send.payload[i];
      i++;
    }
    strcpy(packet_to_send.payload, msg_cpy);
    int sum = 0;
    for(int i = 0; i<20 && msg_cpy[i] != '\0'; i++){
      sum += msg_cpy[i];
    }
    sum = sum + packet_to_send.seqnum;
    sum = sum + packet_to_send.acknum;
    for (int i = 0; i < sizeof(msg_cpy); i++)
        msg_cpy[i] = '\0';
    return sum;
}

struct pkt *create_packet(struct msg message, int sequence_number)
{
  struct pkt *packet_to_send = (struct pkt *)malloc(sizeof(struct pkt));
  char msg_cpy[20];
  int i = 0;
  while(i<20)
  {
    msg_cpy[i] = message.data[i];
    i++;
  }
  strcpy(packet_to_send->payload, msg_cpy);
  packet_to_send->seqnum = sequence_number;
  packet_to_send->acknum = 0; // ACK always false when packet is created from sender side
  packet_to_send->checksum = checksum(*packet_to_send);
  for (int i = 0; i < sizeof(msg_cpy); i++)
  msg_cpy[i] = '\0';
  return packet_to_send;
}

/* called from layer 5, passed the data to be sent to other side */
void A_output(struct msg message)
{
    //printf("Message rcvd from layer 5\n");
    if(msg_transit)
    {
        buffer_msg.push(message);
    }
    else 
    {
        buffer_msg_copy = buffer_msg;
        //printf("List Queue:\n");
        struct msg msg_cpy;
        while(!buffer_msg_copy.empty())
        {
        msg_cpy = buffer_msg_copy.front();
        buffer_msg_copy.pop();
        }
        msg_transit = true;
        created_packet = *create_packet(message, A_sequence_number);
        //printf("Packet done. Let's send!\n");
        ACK_A = false;
        starttimer(0, increment);
        tolayer3(0, created_packet);
    }  
}


/* called from layer 3, when a packet arrives for layer 4 */
void A_input(struct pkt packet)
{
    if(packet.acknum == A_sequence_number && checksum(packet) == packet.checksum)
    {
        stoptimer(0);
        if(A_sequence_number == 0)
        A_sequence_number = 1;
        else
        A_sequence_number = 0;
        buffer_msg_copy = buffer_msg;
        struct msg msg_cpy;
        while(!buffer_msg_copy.empty())
        {
          msg_cpy = buffer_msg_copy.front();
          buffer_msg_copy.pop();
        }
        msg_transit = false;
        if(!buffer_msg.empty())
        {
          ACK_A = true;
          buf_flag = 1;
          
          msg_cpy = buffer_msg.front();
          buffer_msg.pop();
          msg_transit = false;
          A_output(msg_cpy);
        }
        ACK_A = true;
    }
}

/* called when A's timer goes off */
void A_timerinterrupt()
{
      starttimer(0, increment);
      char msg_cpy[20];
      int i = 0;
      while(i<20)
      {
        msg_cpy[i] = packet_to_send.payload[i];
        i++;
      }
      strcpy(packet_to_send.payload, msg_cpy);
      for (int i = 0; i < sizeof(msg_cpy); i++)
      msg_cpy[i] = '\0';
      tolayer3(0, created_packet);
}

/* the following routine will be called once (only) before any other */
/* entity A routines are called. You can use it to do any initialization */
void A_init()
{
    increment = 10.0;
    ACK_A = true;
    A_sequence_number = 0;
}

/* Note that with simplex transfer from a-to-B, there is no B_output() */
/* called from layer 3, when a packet arrives for layer 4 at B*/
void B_input(struct pkt packet)
{
    if(packet.seqnum == B_sequence_number && checksum(packet) == packet.checksum)
    {
        tolayer5(1,packet.payload);
        struct pkt *ack_B = (struct pkt *)malloc(sizeof(struct pkt));
        ack_B->seqnum = 0;//packet.seqnum;
        ack_B->acknum = B_sequence_number;
        ack_B->checksum = checksum(*ack_B);
        tolayer3(1, *ack_B);
        if(B_sequence_number == 0)
        {
            B_sequence_number = 1;
            last_sequence_number = 0;
        }
        else
        {
            B_sequence_number = 0;
            last_sequence_number = 1;
        }

    }
    else if(packet.seqnum != B_sequence_number && checksum(packet) == packet.checksum)
    {
        struct pkt *ack_B = (struct pkt *)malloc(sizeof(struct pkt));
        ack_B->seqnum = 0;//last_sequence_number;
        ack_B->acknum = packet.seqnum;//last_sequence_number;
        ack_B->checksum = checksum(*ack_B);
        tolayer3(1, *ack_B);
    }
}

/* the following rouytine will be called once (only) before any other */
/* entity B routines are called. You can use it to do any initialization */
void B_init()
{
    B_sequence_number = 0;
    last_sequence_number = 0;
}


//./run_experiments -p ../parushga/abt -m 1000 -l 0.1 -c 0.2 -t 50 -w 10
//1111
//./run_experiments -p ../parushga/abt -m 1000 -l 0.1 -c 0.2 -t 50 -w 10
//Testing with MESSAGES:20, LOSS:0.0, CORRUPTION:0.0, ARRIVAL:1000, WINDOW:0 ...