/*
 * Noxim - the NoC Simulator
 *
 * (C) 2005-2018 by the University of Catania
 * For the complete list of authors refer to file ../doc/AUTHORS.txt
 * For the license applied to these sources refer to file ../doc/LICENSE.txt
 *
 * This file contains the declaration of the tile
 */

#ifndef __NOXIMTILE_H__
#define __NOXIMTILE_H__

#include <systemc.h>
#include "Router.h"
#include "ProcessingElement.h"
using namespace std;

SC_MODULE(Tile)
{
    SC_HAS_PROCESS(Tile);

    // I/O Ports
    sc_in_clk clock;                        // The input clock for the tile
    sc_in <bool> reset;                         // The reset signal for the tile

    int local_id; // Unique ID

    sc_in <Flit> flit_rx[DIRECTIONS];   // The input channels
    sc_in <bool> req_rx[DIRECTIONS];            // The requests associated with the input channels
    sc_out <bool> ack_rx[DIRECTIONS];           // The outgoing ack signals associated with the input channels
    sc_out <TBufferFullStatus> buffer_full_status_rx[DIRECTIONS];

    sc_out <Flit> flit_tx[DIRECTIONS];  // The output channels
    sc_out <bool> req_tx[DIRECTIONS];           // The requests associated with the output channels
    sc_in <bool> ack_tx[DIRECTIONS];            // The outgoing ack signals associated with the output channels
    sc_in <TBufferFullStatus> buffer_full_status_tx[DIRECTIONS];

    // hub specific ports
    sc_in <Flit> hub_flit_rx;   // The input channels
    sc_in <bool> hub_req_rx;            // The requests associated with the input channels
    sc_out <bool> hub_ack_rx;           // The outgoing ack signals associated with the input channels
    sc_out <TBufferFullStatus> hub_buffer_full_status_rx;

    sc_out <Flit> hub_flit_tx;  // The output channels
    sc_out <bool> hub_req_tx;           // The requests associated with the output channels
    sc_in <bool> hub_ack_tx;            // The outgoing ack signals associated with the output channels
    sc_in <TBufferFullStatus> hub_buffer_full_status_tx;    


    // NoP related I/O and signals
    sc_out <int> free_slots[DIRECTIONS];
    sc_in <int> free_slots_neighbor[DIRECTIONS];
    sc_out < NoP_data > NoP_data_out[DIRECTIONS];
    sc_in < NoP_data > NoP_data_in[DIRECTIONS];

    /*Ring-related outputs*/
    sc_in <Flit> flit_ring_rx[RINGS][DIRECTIONS];   // The input channels
    sc_in <bool> req_ring_rx[RINGS][DIRECTIONS];         // The requests associated with the input channels
    sc_out <bool> ack_ring_rx[RINGS][DIRECTIONS];            // The outgoing ack signals associated with the input channels
    sc_out <TBufferFullStatus> buffer_full_status_ring_rx[RINGS][DIRECTIONS];

    sc_out <Flit> flit_ring_tx[RINGS][DIRECTIONS];  // The output channels
    sc_out <bool> req_ring_tx[RINGS][DIRECTIONS];           // The requests associated with the output channels
    sc_in <bool> ack_ring_tx[RINGS][DIRECTIONS];            // The outgoing ack signals associated with the output channels
    sc_in <TBufferFullStatus> buffer_full_status_ring_tx[RINGS][DIRECTIONS];

    sc_signal <int> free_slots_local;
    sc_signal <int> free_slots_neighbor_local;

    // Signals required for Router-PE connection
    sc_signal <Flit> flit_rx_local;
    sc_signal <bool> req_rx_local;
    sc_signal <bool> ack_rx_local;
    sc_signal <TBufferFullStatus> buffer_full_status_rx_local;

    sc_signal <Flit> flit_tx_local;
    sc_signal <bool> req_tx_local;
    sc_signal <bool> ack_tx_local;
    sc_signal <TBufferFullStatus> buffer_full_status_tx_local;

    // Internal signals related to the ring
    sc_signal <Flit> flit_ring_rx_local[RINGS];
    sc_signal <bool> req_ring_rx_local[RINGS];
    sc_signal <bool> ack_ring_rx_local[RINGS];
    sc_signal <TBufferFullStatus> buffer_full_status_ring_rx_local[RINGS];

    sc_signal <Flit> flit_ring_tx_local[RINGS];
    sc_signal <bool> req_ring_tx_local[RINGS];
    sc_signal <bool> ack_ring_tx_local[RINGS];
    sc_signal <TBufferFullStatus> buffer_full_status_ring_tx_local[RINGS];

    sc_signal <bool> ring_busy_router[RINGS];

    // Instances
    Router *r;                      // Router instance
    ProcessingElement *pe;                  // Processing Element instance

    // Constructor

    Tile(sc_module_name nm, int id): sc_module(nm) {
    local_id = id;
    //cout << "Building Router"<<endl;
    // Router pin assignments
    r = new Router("Router");
    r->clock(clock);
    r->reset(reset);
    for (int i = 0; i < DIRECTIONS; i++) {
        r->flit_rx_i[i] (flit_rx[i]);
        r->req_rx_i[i] (req_rx[i]);
        r->ack_rx_o[i] (ack_rx[i]);
        r->buffer_full_status_rx_o[i](buffer_full_status_rx[i]);

        r->flit_tx_o[i] (flit_tx[i]);
        r->req_tx_o[i] (req_tx[i]);
        r->ack_tx_i[i] (ack_tx[i]);
        r->buffer_full_status_tx_i[i](buffer_full_status_tx[i]);

        r->free_slots[i] (free_slots[i]);
        r->free_slots_neighbor[i] (free_slots_neighbor[i]);

        // NoP 
        r->NoP_data_out[i] (NoP_data_out[i]);
        r->NoP_data_in[i] (NoP_data_in[i]);

        for (int k = 0; k < RINGS; ++k){
            r->flit_ring_rx_i[k][i] (flit_ring_rx[k][i]);
            r->req_ring_rx_i[k][i] (req_ring_rx[k][i]);
            r->ack_ring_rx_o[k][i] (ack_ring_rx[k][i]);
            r->buffer_full_status_ring_rx_o[k][i] (buffer_full_status_ring_rx[k][i]);

            r->flit_ring_tx_o[k][i] (flit_ring_tx[k][i]);
            r->req_ring_tx_o[k][i] (req_ring_tx[k][i]);
            r->ack_ring_tx_i[k][i] (ack_ring_tx[k][i]);
            r->buffer_full_status_ring_tx_i[k][i] (buffer_full_status_ring_tx[k][i]);
        }
    }
    
    // local
    r->flit_rx_i[DIRECTION_LOCAL] (flit_tx_local);
    r->req_rx_i[DIRECTION_LOCAL] (req_tx_local);
    r->ack_rx_o[DIRECTION_LOCAL] (ack_tx_local);
    r->buffer_full_status_rx_o[DIRECTION_LOCAL] (buffer_full_status_tx_local);

    r->flit_tx_o[DIRECTION_LOCAL] (flit_rx_local);
    r->req_tx_o[DIRECTION_LOCAL] (req_rx_local);
    r->ack_tx_i[DIRECTION_LOCAL] (ack_rx_local);
    r->buffer_full_status_tx_i[DIRECTION_LOCAL] (buffer_full_status_rx_local);


    // hub related
    r->flit_rx_i[DIRECTION_HUB] (hub_flit_rx);
    r->req_rx_i[DIRECTION_HUB] (hub_req_rx);
    r->ack_rx_o[DIRECTION_HUB] (hub_ack_rx);
    r->buffer_full_status_rx_o[DIRECTION_HUB] (hub_buffer_full_status_rx);

    r->flit_tx_o[DIRECTION_HUB] (hub_flit_tx);
    r->req_tx_o[DIRECTION_HUB] (hub_req_tx);
    r->ack_tx_i[DIRECTION_HUB] (hub_ack_tx);
    r->buffer_full_status_tx_i[DIRECTION_HUB] (hub_buffer_full_status_tx);

    for (int k = 0; k < RINGS; ++k) {
        r->ring_busy_o[k](ring_busy_router[k]);
        r->flit_ring_rx_i[k][DIRECTION_LOCAL] (flit_ring_tx_local[k]);
        r->req_ring_rx_i[k][DIRECTION_LOCAL] (req_ring_tx_local[k]);
        r->ack_ring_rx_o[k][DIRECTION_LOCAL] (ack_ring_tx_local[k]);
        r->buffer_full_status_ring_rx_o[k][DIRECTION_LOCAL] (buffer_full_status_ring_tx_local[k]);

        r->flit_ring_tx_o[k][DIRECTION_LOCAL] (flit_ring_rx_local[k]);
        r->req_ring_tx_o[k][DIRECTION_LOCAL] (req_ring_rx_local[k]);
        r->ack_ring_tx_i[k][DIRECTION_LOCAL] (ack_ring_tx_local[k]);
        r->buffer_full_status_ring_tx_i[k][DIRECTION_LOCAL] (buffer_full_status_ring_rx_local[k]);
    }

    // Processing Element pin assignments
    //cout << "Building PE"<<endl;
    pe = new ProcessingElement("ProcessingElement");
    pe->clock(clock);
    pe->reset(reset);

    pe->flit_rx_i(flit_rx_local);
    pe->req_rx_i(req_rx_local);
    pe->ack_rx_o(ack_rx_local);
    pe->buffer_full_status_rx_o(buffer_full_status_rx_local);
    

    pe->flit_tx_o(flit_tx_local);
    pe->req_tx_o(req_tx_local);
    pe->ack_tx_i(ack_tx_local);
    pe->buffer_full_status_tx_i(buffer_full_status_tx_local);

    for (int k = 0; k < RINGS; ++k) {
        pe->flit_ring_rx_i[k](flit_ring_rx_local[k]);
        pe->req_ring_rx_i[k](req_ring_rx_local[k]);
        pe->ack_ring_rx_o[k](ack_ring_tx_local[k]);
        pe->buffer_full_status_ring_rx_o[k](buffer_full_status_ring_rx_local[k]);

        pe->flit_ring_tx_o[k](flit_ring_tx_local[k]);
        pe->req_ring_tx_o[k](req_ring_tx_local[k]);
        pe->ack_ring_tx_i[k](ack_ring_tx_local[k]);
        pe->buffer_full_status_ring_tx_i[k](buffer_full_status_ring_tx_local[k]);

        pe->ring_busy_i[k](ring_busy_router[k]);
    }
    // NoP
    r->free_slots[DIRECTION_LOCAL] (free_slots_local);
    r->free_slots_neighbor[DIRECTION_LOCAL] (free_slots_neighbor_local);
    pe->free_slots_neighbor_i(free_slots_neighbor_local);
    //cout << "Building in Tile completed"<<endl;
}

};

#endif
