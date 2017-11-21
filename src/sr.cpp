#include "../include/simulator.h"
#include<iostream>
#include<string.h>
#include<stdio.h>
#include<stdlib.h>
#include<vector>
#include<map>
#include<list>
#include<queue>
#include<limits.h>
//#include<array>
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

static vector <struct time_pkt_node *> packet_time_vector;
struct pkt recv_pkt[3000];

float delay_RTT;// = 10.0;
int rwindow;
int sendbase, nextseqnum, recvbase, senderwindow;

struct timer_struct
{
  int sequencenumber;
  float starttime;
  bool ack;
};

struct time_pkt_node
{
  float cur_time;
  struct pkt *packet;
  time_pkt_node(struct pkt *new_pack)
  {
    packet = new_pack;
    float cur_time = 0;
  }
};

int checksum(struct pkt packet){
    char cpy_msg[20];
    strcpy(cpy_msg,packet.payload);
    int sum = 0;
    for(int i = 0; i<20 && cpy_msg[i] != '\0'; i++)
    {
        sum += cpy_msg[i];
    }

    sum += packet.seqnum;

    sum += packet.acknum;

    return sum;
    }
  
struct pkt *create_packet(struct msg message, int sequence_number)
{
struct pkt *packet = (struct pkt *)malloc(sizeof(struct pkt));
(*packet).seqnum = sequence_number;
(*packet).acknum = -1;
strcpy((*packet).payload,message.data);
(*packet).checksum = checksum((*packet));
return packet;
}

/********* STUDENTS WRITE THE NEXT SEVEN ROUTINES *********/
/* called from layer 5, passed the data to be sent to other side */
void A_output(struct msg message)
{ 
    packet_time_vector.push_back(new (struct time_pkt_node)(create_packet(message, nextseqnum)));
    if(nextseqnum >= sendbase && nextseqnum < sendbase + senderwindow)
    {
        packet_time_vector.at(nextseqnum) -> cur_time = get_sim_time();
        tolayer3(0,*packet_time_vector.at(nextseqnum)->packet);
    }

    if(sendbase == nextseqnum)
        starttimer(0,delay_RTT);
  nextseqnum++;
}

/* called from layer 3, when a packet arrives for layer 4 */
void A_input(struct pkt packet)
{
    if((packet.checksum == checksum(packet)) && (packet.acknum >= sendbase && packet.acknum < (sendbase + senderwindow)))
    {
        packet_time_vector.at(packet.acknum)->packet->acknum = 1; 

        if(sendbase == packet.acknum)
        {
            stoptimer(0);
            sendbase++;

            for(sendbase; sendbase < nextseqnum; sendbase++)
            {
                if((sendbase + senderwindow < nextseqnum) && (packet_time_vector.at(sendbase + senderwindow)->packet->acknum == -1))
                {
                    packet_time_vector.at(sendbase + senderwindow) -> cur_time = get_sim_time();
                    tolayer3(0,*packet_time_vector.at(sendbase + senderwindow)->packet);
                }
                //If there is an UnACKed message, starttimer()
                if(packet_time_vector.at(sendbase)->packet->acknum == -1)
                {
                    int remainDelay = delay_RTT - (get_sim_time() - (packet_time_vector.at(sendbase)->cur_time));
                    starttimer(0,remainDelay);
                    break;
                }
            }
        }
    }
}

/* called when A's timer goes off */
// Below logic for finding the first minimum and second minimum has been taken from geeksforgeeks website.
// http://www.geeksforgeeks.org/to-find-smallest-and-second-smallest-element-in-an-array/
void A_timerinterrupt()
{
    float time_of_packet;
    int packetnumber_first;
    int packetnumber_second;
    float firstmin, secondmin;
    packetnumber_first = packetnumber_second = sendbase;
    firstmin = secondmin = packet_time_vector.at(sendbase)->cur_time;

    for (int i = sendbase; i < sendbase + senderwindow && i < nextseqnum; i++)
    {
        if(packet_time_vector.at(i)->packet->acknum == -1)
        {
            /* If current element is smaller than first 
            then update both first and second */
            if (packet_time_vector.at(i)->cur_time < firstmin)
            {
                secondmin = firstmin;
                firstmin = packet_time_vector.at(i)->cur_time;
                packetnumber_first = packet_time_vector.at(i)->packet->seqnum;
            }
            /* If packet_time_vector.at(i) is in between first and second 
            then update second  */
            else if (packet_time_vector.at(i)->cur_time < secondmin && packet_time_vector.at(i)->cur_time != firstmin)
            {
                secondmin = packet_time_vector.at(i)->cur_time;
                packetnumber_second = packet_time_vector.at(i)->packet->seqnum;
            }
        }
    }

    //This means only single packet in vector with unACKed status. Send it!
    if(secondmin == firstmin)
        starttimer(0, delay_RTT); //Starting timer for base packet
    //
    else    
        starttimer(0, packet_time_vector.at(packetnumber_second)->cur_time - (get_sim_time() - delay_RTT)); //Starting timer according to the second minimum time packet
    //cout<<"Sending below packet"<<endl;
    packet_time_vector.at(packetnumber_first)->cur_time = get_sim_time();
    tolayer3(0,*(packet_time_vector.at(packetnumber_first)->packet));
}

/* the following routine will be called once (only) before any other */
/* entity A routines are called. You can use it to do any initialization */
void A_init()
{
  delay_RTT = 20.0;
  sendbase = 0;
  nextseqnum = 0;
  senderwindow = getwinsize();
}

/* called from layer 3, when a packet arrives for layer 4 at B*/
void B_input(struct pkt packet)
{
    if(packet.checksum == checksum(packet) && (packet.seqnum >= recvbase && packet.seqnum < (recvbase + senderwindow)))
    {

        recv_pkt[packet.seqnum] = packet;

        struct pkt *ack_B = (struct pkt *)malloc(sizeof(struct pkt));
        ack_B->seqnum = -1;
        ack_B->acknum = packet.seqnum;
        ack_B->checksum = checksum(*ack_B);
        tolayer3(1, *ack_B);

        //In order delivery
        if(packet.seqnum == recvbase)
        {
            tolayer5(1, recv_pkt[recvbase].payload);
            recvbase++;
            while(1)
            {
                int tempseq = recv_pkt[recvbase].seqnum;
                if(tempseq == recvbase)
                {
                    tolayer5(1, recv_pkt[recvbase].payload);
                    recvbase++;
                }
                else 
                    break;
            }
        }

        else if(packet.checksum == checksum(packet) && packet.seqnum < recvbase)
        {
          struct pkt *ack_B = (struct pkt *)malloc(sizeof(struct pkt));
          ack_B->seqnum = -1;
          ack_B->acknum = packet.seqnum;
          ack_B->checksum = checksum(*ack_B);
          tolayer3(1, *ack_B);
        }

    }

    else if(packet.checksum == checksum(packet) && packet.seqnum < recvbase)
    {
        //cout<<"Duplicate ACKs"<<endl;
        struct pkt *ack_B = (struct pkt *)malloc(sizeof(struct pkt));

        ack_B->seqnum = -1;
        ack_B->acknum = packet.seqnum;
        ack_B->checksum = checksum(*ack_B);
        tolayer3(1, *ack_B);
    }
}

/* the following rouytine will be called once (only) before any other */
/* entity B routines are called. You can use it to do any initialization */
void B_init()
{
  recvbase = 0;
  rwindow = getwinsize();
}

