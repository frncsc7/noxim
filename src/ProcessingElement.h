/*
 * Noxim - the NoC Simulator
 *
 * (C) 2005-2018 by the University of Catania
 * For the complete list of authors refer to file ../doc/AUTHORS.txt
 * For the license applied to these sources refer to file ../doc/LICENSE.txt
 *
 * This file contains the declaration of the processing element
 */

#ifndef __NOXIMPROCESSINGELEMENT_H__
#define __NOXIMPROCESSINGELEMENT_H__

#include <queue>
#include <systemc.h>

#include "DataStructs.h"
#include "GlobalTrafficTable.h"
#include "Utils.h"

using namespace std;

SC_MODULE(ProcessingElement)
{

    // I/O Ports
    sc_in_clk clock;		// The input clock for the PE
    sc_in < bool > reset;	// The reset signal for the PE

    sc_in < Flit > flit_rx_i;	// The input channel
    sc_in < bool > req_rx_i;	// The request associated with the input channel
    sc_out < bool > ack_rx_o;	// The outgoing ack signal associated with the input channel
    sc_out < TBufferFullStatus > buffer_full_status_rx_o;	

    sc_out < Flit > flit_tx_o;	// The output channel
    sc_out < bool > req_tx_o;	// The request associated with the output channel
    sc_in < bool > ack_tx_i;	// The outgoing ack signal associated with the output channel
    sc_in < TBufferFullStatus > buffer_full_status_tx_i;

    sc_in < int >free_slots_neighbor_i;

    // Ring related I/O
    sc_in < Flit > flit_ring_rx_i[RINGS];   // The input channel
    sc_in < bool > req_ring_rx_i[RINGS];    // The request associated with the input channel
    sc_out < bool > ack_ring_rx_o[RINGS];   // The outgoing ack signal associated with the input channel
    sc_out < TBufferFullStatus > buffer_full_status_ring_rx_o[RINGS];

    sc_out < Flit > flit_ring_tx_o[RINGS];  // The output channel
    sc_out < bool > req_ring_tx_o[RINGS];   // The request associated with the output channel
    sc_in < bool > ack_ring_tx_i[RINGS];    // The outgoing ack signal associated with the output channel
    sc_in < TBufferFullStatus > buffer_full_status_ring_tx_i[RINGS];

    sc_in <bool> ring_busy_i[RINGS];
    sc_in <bool> req_router_rx_i[RINGS][DIRECTIONS]; // All request inputs of the router propagated here, to speed-up injection of packets
    sc_in <Flit> flit_router_rx_i[RINGS][DIRECTIONS];

    // Registers
    int local_id;		// Unique identification number
    bool current_level_rx;	// Current level for Alternating Bit Protocol (ABP)
    bool current_level_tx;	// Current level for Alternating Bit Protocol (ABP)
    queue < Packet > packet_queue;	// Local queue of packets
    bool transmittedAtPreviousCycle;	// Used for distributions with memory
    // FM: added a register to count the number of packets generated in the packet queue of the PE (for each ring)
    int n_packets[RINGS];
    //Ring Registers
    bool ring_current_level_rx[RINGS];  // Current level for Alternating Bit Protocol (ABP)
    bool ring_current_level_tx[RINGS];  // Current level for Alternating Bit Protocol (ABP)
    queue < Packet > ring_packet_queue[RINGS];  // Local queue of packets
    bool ring_transmittedAtPreviousCycle[RINGS];    // Used for distributions with memory
    bool ring_enable[RINGS]; // FM: Only used by ring-1 in case of a traffic that involves vrgather, generating packets after receiving some
    bool send_before[RINGS];
    bool router_current_level_rx[RINGS][DIRECTIONS];
    bool router_tx_inflight[RINGS];
    // Functions
    void process();
    void rxProcess();		// The receiving process
    void txProcess();		// The transmitting process
    void ringProcess();
    bool canShot(Packet & packet);	// True when the packet must be shot
    bool ringcanShot(Packet & packet, int ring_id, int data_dst_id);
    Flit nextFlit();	// Take the next flit of the current packet
    Flit nextringFlit(int ring_id);
    Packet trafficTest();	// used for testing traffic
    Packet trafficRandom();	// Random destination distribution
    Packet trafficTranspose1();	// Transpose 1 destination distribution
    Packet trafficTranspose2();	// Transpose 2 destination distribution
    Packet trafficBitReversal();	// Bit-reversal destination distribution
    Packet trafficShuffle();	// Shuffle destination distribution
    Packet trafficButterfly();	// Butterfly destination distribution
    Packet trafficLocal();	// Random with locality
    Packet trafficULocal();	// Random with locality
    // FM: Added declaration of trafficSlideUp()
    Packet trafficSlideUp();
    Packet trafficFFTW1(int ring_id, int dst_id);

    GlobalTrafficTable *traffic_table;	// Reference to the Global traffic Table
    bool never_transmit;	// true if the PE does not transmit any packet 
    //  (valid only for the table based traffic)

    void fixRanges(const Coord, Coord &);	// Fix the ranges of the destination
    int randInt(int min, int max);	// Extracts a random integer number between min and max
    int getRandomSize();	// Returns a random size in flits for the packet
    void setBit(int &x, int w, int v);
    int getBit(int x, int w);
    double log2ceil(double x);

    int roulett();
    int findRandomDestination(int local_id,int hops);
    unsigned int getQueueSize() const;

    // Constructor
    SC_CTOR(ProcessingElement) {
        SC_METHOD(process);
        sensitive << reset;
        sensitive << clock.pos();
    }
};

#endif
