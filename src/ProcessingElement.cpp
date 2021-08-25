/*
 * Noxim - the NoC Simulator
 *
 * (C) 2005-2018 by the University of Catania
 * For the complete list of authors refer to file ../doc/AUTHORS.txt
 * For the license applied to these sources refer to file ../doc/LICENSE.txt
 *
 * This file contains the implementation of the processing element
 */

#include "ProcessingElement.h"

int ProcessingElement::randInt(int min, int max)
{
    return min +
	(int) ((double) (max - min + 1) * rand() / (RAND_MAX + 1.0));
}

void ProcessingElement::process() {
    if (GlobalParams::topology == TOPOLOGY_RING) {
        ringProcess();
    }
    else {
        txProcess();
        rxProcess();
    }
}

void ProcessingElement::ringProcess() {
    if (reset.read()) {
        for (int k = 0; k < RINGS; ++k) {
            ack_ring_rx_o[k].write(0);
            ring_current_level_rx[k] = 0;
            req_ring_tx_o[k].write(0);
            ring_current_level_tx[k] = 0;
            n_packets[k] = 0;
            ring_transmittedAtPreviousCycle[k] = false;
        }
    }
    else {
        for (int k = 0; k < RINGS; ++k) {
            if (req_ring_rx_i[k].read() == 1 - ring_current_level_rx[k]) {
                Flit flit_tmp = flit_ring_rx_i[k].read();
                ring_current_level_rx[k] = 1 - ring_current_level_rx[k];    // Negate the old value for Alternating Bit Protocol (ABP)
            }
            ack_ring_rx_o[k].write(ring_current_level_rx[k]);
            Packet packet;
            // If it's for the ring 0 (index ring for gathers, data ring for slides), of is the ring 1 and ring 0 received a data
            if ( (k == 0) || ((k == 1) && (req_ring_rx_i[0].read() == 1 - ring_current_level_rx[0])) ){
                Flit flit_tmp_ring = flit_ring_rx_i[0].read();
                int dst_id = flit_tmp_ring.dst_id;
                if (ringcanShot(packet, k, dst_id)) {
                    //LOG << "Pushing a new flit for ring" << k << endl; // FM
                    ring_packet_queue[k].push(packet);
                    n_packets[k] = n_packets[k] + 1;
                    //LOG << "Currently, " << n_packets[k] << " packets have been generated" << endl;
                    ring_transmittedAtPreviousCycle[k] = true;
                }
                else {
                    ring_transmittedAtPreviousCycle[k] = false;
                }
                LOG << "Ack_tx_i = " << ack_ring_tx_i[k].read() << endl; // FM
                LOG << "ring_current_level_tx[k] = " << ring_current_level_tx[k] << endl; // FM
                //LOG << "Busy = " << ring_busy_i[k].read() << endl; // FM
                if ((ack_ring_tx_i[k].read() == ring_current_level_tx[k]) && !(ring_busy_i[k].read())) { // ADD A CONDITION HERE?
                    //LOG << "Received ack on the current level tx!" << endl; // FM
                    if (!ring_packet_queue[k].empty()) {
                        //LOG << "Popping a new flit" << endl; // FM
                        Flit flit = nextringFlit(k);  // Generate a new flit
                        flit_ring_tx_o[k]->write(flit);  // Send the generated flit
                        ring_current_level_tx[k] = 1 - ring_current_level_tx[k]; // Negate the old value for Alternating Bit Protocol (ABP)
                        req_ring_tx_o[k].write(ring_current_level_tx[k]);
                    }
                    else LOG << "Packet queue is empty";
                }
            }
        }
    }
}

Flit ProcessingElement::nextringFlit(int ring_id) // TODO: Update according to the double-ring
{
    Flit flit;
    Packet packet = ring_packet_queue[ring_id].front();

    flit.src_id = packet.src_id;
    flit.dst_id = packet.dst_id;
    flit.vc_id = packet.vc_id;
    flit.timestamp = packet.timestamp;
    flit.sequence_no = packet.size - packet.flit_left;
    flit.sequence_length = packet.size;
    if (!GlobalParams::bidirectionality) {
        if (flit.src_id > flit.dst_id) {
            flit.hop_no = (GlobalParams::mesh_dim_x * GlobalParams::mesh_dim_y) - flit.src_id + flit.dst_id;
        }
        else
            flit.hop_no = flit.dst_id - flit.src_id;
    }
    else {
        if (flit.src_id > flit.dst_id) {
            if (flit.src_id - flit.dst_id > GlobalParams::mesh_dim_x) {
                flit.hop_no = (GlobalParams::mesh_dim_x * GlobalParams::mesh_dim_y) - flit.src_id + flit.dst_id;
            }
            else {
                flit.hop_no = flit.src_id - flit.dst_id;
            }
        }
        else {
            if (flit.src_id - flit.dst_id > GlobalParams::mesh_dim_x) {
                flit.hop_no = (GlobalParams::mesh_dim_x * GlobalParams::mesh_dim_y) - flit.dst_id + flit.src_id;
            }
            else {
                flit.hop_no = flit.dst_id - flit.src_id;
            }
        }
    }

    flit.hub_relay_node = NOT_VALID;

    if (packet.size == packet.flit_left)
    flit.flit_type = FLIT_TYPE_HEAD;
    else if (packet.flit_left == 1)
    flit.flit_type = FLIT_TYPE_TAIL;
    else
    flit.flit_type = FLIT_TYPE_BODY;

    ring_packet_queue[ring_id].front().flit_left--;
    if (ring_packet_queue[ring_id].front().flit_left == 0) {
        //LOG << "Popping packet" << endl;
        ring_packet_queue[ring_id].pop();
    }

    return flit;
}

bool ProcessingElement::ringcanShot(Packet & packet, int ring_id, int data_dst_id)
{
   // assert(false);
    if(never_transmit) return false;

#ifdef DEADLOCK_AVOIDANCE
    if (local_id%2==0)
    return false;
#endif
    bool shot;
    double threshold;

    double now = sc_time_stamp().to_double() / GlobalParams::clock_period_ps;

    threshold = GlobalParams::packet_injection_rate;
    shot = (((double) rand()) / RAND_MAX < threshold);
    if (shot) {
        if (GlobalParams::traffic_distribution == TRAFFIC_SLIDEUP)
            packet = trafficSlideUp();
        else if (GlobalParams::traffic_distribution == TRAFFIC_FFTW1)
            packet = trafficFFTW1(ring_id, data_dst_id);
        else {
            cout << "Invalid traffic distribution: " << GlobalParams::traffic_distribution << endl;
            exit(-1);
        }
    }
    return shot;
}

void ProcessingElement::rxProcess()
{
    if (reset.read()) {
	ack_rx_o.write(0);
	current_level_rx = 0;
    } else {
	if (req_rx_i.read() == 1 - current_level_rx) {
	    Flit flit_tmp = flit_rx_i.read();
	    current_level_rx = 1 - current_level_rx;	// Negate the old value for Alternating Bit Protocol (ABP)
	}
	ack_rx_o.write(current_level_rx);
    }
}

void ProcessingElement::txProcess()
{
    if (reset.read()) {
	req_tx_o.write(0);
	current_level_tx = 0;
	transmittedAtPreviousCycle = false;
    } else {
	   Packet packet;
	   if (canShot(packet)) {
            //LOG << "Function canShot returned true" << endl; // FM
	       packet_queue.push(packet);
	       transmittedAtPreviousCycle = true;
	   } else {
	       transmittedAtPreviousCycle = false;
            //LOG << "Function canShot returned false, not sending anything!!" << endl; // FM
        }
        //LOG << "Ack_tx_i = " << ack_tx_i.read() << endl; // FM
        //LOG << "current_level_tx = " << current_level_tx << endl; // FM
	   if ((ack_tx_i.read() == current_level_tx)) { // ADD A CONDITION HERE?
        //LOG << "Received ack on the current level tx!" << endl; // FM
	       if (!packet_queue.empty()) {
               //LOG << "Popping a new flit" << endl; // FM
	   	       Flit flit = nextFlit();	// Generate a new flit
	   	       flit_tx_o->write(flit);	// Send the generated flit
	   	       current_level_tx = 1 - current_level_tx;	// Negate the old value for Alternating Bit Protocol (ABP)
	   	       req_tx_o.write(current_level_tx);
	        }
            else LOG << "Packet queue is empty";
	   }
    }
}

Flit ProcessingElement::nextFlit()
{
    Flit flit;
    Packet packet = packet_queue.front();

    flit.src_id = packet.src_id;
    flit.dst_id = packet.dst_id;
    flit.vc_id = packet.vc_id;
    flit.timestamp = packet.timestamp;
    flit.sequence_no = packet.size - packet.flit_left;
    flit.sequence_length = packet.size;
    if (GlobalParams::topology == TOPOLOGY_RING) {
        if (!GlobalParams::bidirectionality) {
            if (flit.src_id > flit.dst_id) {
                flit.hop_no = (GlobalParams::mesh_dim_x * GlobalParams::mesh_dim_y) - flit.src_id + flit.dst_id;
            }
            else
                flit.hop_no = flit.dst_id - flit.src_id;
        }
        else {
            if (flit.src_id > flit.dst_id) {
                if (flit.src_id - flit.dst_id > GlobalParams::mesh_dim_x) {
                    flit.hop_no = (GlobalParams::mesh_dim_x * GlobalParams::mesh_dim_y) - flit.src_id + flit.dst_id;
                }
                else {
                    flit.hop_no = flit.src_id - flit.dst_id;
                }
            }
            else {
                if (flit.src_id - flit.dst_id > GlobalParams::mesh_dim_x) {
                    flit.hop_no = (GlobalParams::mesh_dim_x * GlobalParams::mesh_dim_y) - flit.dst_id + flit.src_id;
                }
                else {
                    flit.hop_no = flit.dst_id - flit.src_id;
                }
            }
        }
    }
    else
        flit.hop_no = 0;
    //  flit.payload     = DEFAULT_PAYLOAD;

    flit.hub_relay_node = NOT_VALID;

    if (packet.size == packet.flit_left)
	flit.flit_type = FLIT_TYPE_HEAD;
    else if (packet.flit_left == 1)
	flit.flit_type = FLIT_TYPE_TAIL;
    else
	flit.flit_type = FLIT_TYPE_BODY;

    packet_queue.front().flit_left--;
    if (packet_queue.front().flit_left == 0) {
        //LOG << "Popping packet" << endl;
        packet_queue.pop();
    }

    return flit;
}

bool ProcessingElement::canShot(Packet & packet)
{
   // assert(false);
    if(never_transmit) return false;
   
    //if(local_id!=16) return false;
    /* DEADLOCK TEST 
	double current_time = sc_time_stamp().to_double() / GlobalParams::clock_period_ps;

	if (current_time >= 4100) 
	{
	    //if (current_time==3500)
	         //cout << name() << " IN CODA " << packet_queue.size() << endl;
	    return false;
	}
	//*/

#ifdef DEADLOCK_AVOIDANCE
    if (local_id%2==0)
	return false;
#endif
    bool shot;
    double threshold;

    double now = sc_time_stamp().to_double() / GlobalParams::clock_period_ps;

    if (GlobalParams::traffic_distribution != TRAFFIC_TABLE_BASED) {
        if (GlobalParams::topology == TOPOLOGY_RING)
            threshold = GlobalParams::packet_injection_rate;
        else {
            if (!transmittedAtPreviousCycle) {
                LOG << "Didn't transmit at previous cycle" << endl; // FM
                threshold = GlobalParams::packet_injection_rate;
            }
            else {
                LOG << "Transmitted at previous cycle" << endl; // FM
                threshold = GlobalParams::probability_of_retransmission;
            }
        }
    	shot = (((double) rand()) / RAND_MAX < threshold);
    	if (shot) {
    	    if (GlobalParams::traffic_distribution == TRAFFIC_RANDOM)
    		    packet = trafficRandom();
            else if (GlobalParams::traffic_distribution == TRAFFIC_TRANSPOSE1)
    		    packet = trafficTranspose1();
            else if (GlobalParams::traffic_distribution == TRAFFIC_TRANSPOSE2) {
                //cout << "Inside ProcessingElement.cpp, traffic is transpose" << endl;
        		packet = trafficTranspose2();
            }
            else if (GlobalParams::traffic_distribution == TRAFFIC_BIT_REVERSAL)
    		    packet = trafficBitReversal();
            else if (GlobalParams::traffic_distribution == TRAFFIC_SHUFFLE)
    		    packet = trafficShuffle();
            else if (GlobalParams::traffic_distribution == TRAFFIC_BUTTERFLY)
    		    packet = trafficButterfly();
            else if (GlobalParams::traffic_distribution == TRAFFIC_LOCAL)
    		    packet = trafficLocal();
            else if (GlobalParams::traffic_distribution == TRAFFIC_ULOCAL)
    		    packet = trafficULocal();
            else {
                cout << "Invalid traffic distribution: " << GlobalParams::traffic_distribution << endl;
                exit(-1);
            }
        }
    } else {			// Table based communication traffic
	if (never_transmit)
	    return false;

	bool use_pir = (transmittedAtPreviousCycle == false);
	vector < pair < int, double > > dst_prob;
	double threshold =
	    traffic_table->getCumulativePirPor(local_id, (int) now, use_pir, dst_prob);

	double prob = (double) rand() / RAND_MAX;
	shot = (prob < threshold);
	if (shot) {
	    for (unsigned int i = 0; i < dst_prob.size(); i++) {
		if (prob < dst_prob[i].second) {
                    int vc = randInt(0,GlobalParams::n_virtual_channels-1);
		    packet.make(local_id, dst_prob[i].first, vc, now, getRandomSize());
		    break;
		}
	    }
	}
    }

    return shot;
}


Packet ProcessingElement::trafficLocal()
{
    Packet p;
    p.src_id = local_id;
    double rnd = rand() / (double) RAND_MAX;

    vector<int> dst_set;

    int max_id = (GlobalParams::mesh_dim_x * GlobalParams::mesh_dim_y);

    for (int i=0;i<max_id;i++)
    {
	if (rnd<=GlobalParams::locality)
	{
	    if (local_id!=i && sameRadioHub(local_id,i))
		dst_set.push_back(i);
	}
	else
	    if (!sameRadioHub(local_id,i))
		dst_set.push_back(i);
    }


    int i_rnd = rand()%dst_set.size();

    p.dst_id = dst_set[i_rnd];
    p.timestamp = sc_time_stamp().to_double() / GlobalParams::clock_period_ps;
    p.size = p.flit_left = getRandomSize();
    p.vc_id = randInt(0,GlobalParams::n_virtual_channels-1);
    
    return p;
}


int ProcessingElement::findRandomDestination(int id, int hops)
{
    assert((GlobalParams::topology == TOPOLOGY_MESH) || (GlobalParams::topology == TOPOLOGY_RING));

    int inc_y = rand()%2?-1:1;
    int inc_x = rand()%2?-1:1;
    
    Coord current =  id2Coord(id);
    


    for (int h = 0; h<hops; h++)
    {

	if (current.x==0)
	    if (inc_x<0) inc_x=0;

	if (current.x== GlobalParams::mesh_dim_x-1)
	    if (inc_x>0) inc_x=0;

	if (current.y==0)
	    if (inc_y<0) inc_y=0;

	if (current.y==GlobalParams::mesh_dim_y-1)
	    if (inc_y>0) inc_y=0;

	if (rand()%2)
	    current.x +=inc_x;
	else
	    current.y +=inc_y;
    }
    return coord2Id(current);
}


int roulette()
{
    int slices = GlobalParams::mesh_dim_x + GlobalParams::mesh_dim_y -2;


    double r = rand()/(double)RAND_MAX;


    for (int i=1;i<=slices;i++)
    {
	if (r< (1-1/double(2<<i)))
	{
	    return i;
	}
    }
    assert(false);
    return 1;
}


Packet ProcessingElement::trafficULocal()
{
    Packet p;
    p.src_id = local_id;

    int target_hops = roulette();

    p.dst_id = findRandomDestination(local_id,target_hops);

    p.timestamp = sc_time_stamp().to_double() / GlobalParams::clock_period_ps;
    p.size = p.flit_left = getRandomSize();
    p.vc_id = randInt(0,GlobalParams::n_virtual_channels-1);

    return p;
}

Packet ProcessingElement::trafficRandom()
{
    Packet p;
    p.src_id = local_id;
    double rnd = rand() / (double) RAND_MAX;
    double range_start = 0.0;
    int max_id;

    if ((GlobalParams::topology == TOPOLOGY_MESH) || (GlobalParams::topology == TOPOLOGY_RING))
	max_id = (GlobalParams::mesh_dim_x * GlobalParams::mesh_dim_y) - 1; //Mesh 
    else    // other delta topologies
	max_id = GlobalParams::n_delta_tiles-1; 

    // Random destination distribution
    do {
	p.dst_id = randInt(0, max_id);

	// check for hotspot destination
	for (size_t i = 0; i < GlobalParams::hotspots.size(); i++) {

	    if (rnd >= range_start && rnd < range_start + GlobalParams::hotspots[i].second) {
		if (local_id != GlobalParams::hotspots[i].first ) {
		    p.dst_id = GlobalParams::hotspots[i].first;
		}
		break;
	    } else
		range_start += GlobalParams::hotspots[i].second;	// try next
	}
#ifdef DEADLOCK_AVOIDANCE
	assert((GlobalParams::topology == TOPOLOGY_MESH) || (GlobalParams::topology == TOPOLOGY_RING));
	if (p.dst_id%2!=0)
	{
	    p.dst_id = (p.dst_id+1)%256;
	}
#endif

    } while (p.dst_id == p.src_id);

    p.timestamp = sc_time_stamp().to_double() / GlobalParams::clock_period_ps;
    p.size = p.flit_left = getRandomSize();
    p.vc_id = randInt(0,GlobalParams::n_virtual_channels-1);

    return p;
}
// TODO: for testing only
Packet ProcessingElement::trafficTest()
{
    Packet p;
    p.src_id = local_id;
    p.dst_id = 10;

    p.timestamp = sc_time_stamp().to_double() / GlobalParams::clock_period_ps;
    p.size = p.flit_left = getRandomSize();
    p.vc_id = randInt(0,GlobalParams::n_virtual_channels-1);

    return p;
}

Packet ProcessingElement::trafficTranspose1()
{
    assert((GlobalParams::topology == TOPOLOGY_MESH) || (GlobalParams::topology == TOPOLOGY_RING));
    Packet p;
    p.src_id = local_id;
    Coord src, dst;

    // Transpose 1 destination distribution
    src.x = id2Coord(p.src_id).x;
    src.y = id2Coord(p.src_id).y;
    dst.x = GlobalParams::mesh_dim_x - 1 - src.y;
    dst.y = GlobalParams::mesh_dim_y - 1 - src.x;
    fixRanges(src, dst);
    p.dst_id = coord2Id(dst);

    p.vc_id = randInt(0,GlobalParams::n_virtual_channels-1);
    p.timestamp = sc_time_stamp().to_double() / GlobalParams::clock_period_ps;
    p.size = p.flit_left = getRandomSize();

    return p;
}

Packet ProcessingElement::trafficTranspose2()
{
    assert((GlobalParams::topology == TOPOLOGY_MESH)||(GlobalParams::topology == TOPOLOGY_RING));
    Packet p;
    p.src_id = local_id;
    Coord src, dst;
    // Transpose 2 destination distribution
    src.x = id2Coord(p.src_id).x;
    src.y = id2Coord(p.src_id).y;
    dst.x = src.y;
    dst.y = src.x;
    fixRanges(src, dst);
    p.dst_id = coord2Id(dst);
    p.vc_id = randInt(0,GlobalParams::n_virtual_channels-1);
    p.timestamp = sc_time_stamp().to_double() / GlobalParams::clock_period_ps;
    p.size = p.flit_left = getRandomSize();

    return p;
}

void ProcessingElement::setBit(int &x, int w, int v)
{
    int mask = 1 << w;

    if (v == 1)
	x = x | mask;
    else if (v == 0)
	x = x & ~mask;
    else
	assert(false);
}

int ProcessingElement::getBit(int x, int w)
{
    return (x >> w) & 1;
}

inline double ProcessingElement::log2ceil(double x)
{
    return ceil(log(x) / log(2.0));
}

Packet ProcessingElement::trafficBitReversal()
{

    int nbits =
	(int)
	log2ceil((double)
		 (GlobalParams::mesh_dim_x *
		  GlobalParams::mesh_dim_y));
    int dnode = 0;
    for (int i = 0; i < nbits; i++)
	setBit(dnode, i, getBit(local_id, nbits - i - 1));

    Packet p;
    p.src_id = local_id;
    p.dst_id = dnode;

    p.vc_id = randInt(0,GlobalParams::n_virtual_channels-1);
    p.timestamp = sc_time_stamp().to_double() / GlobalParams::clock_period_ps;
    p.size = p.flit_left = getRandomSize();

    return p;
}

Packet ProcessingElement::trafficShuffle()
{

    int nbits =
	(int)
	log2ceil((double)
		 (GlobalParams::mesh_dim_x *
		  GlobalParams::mesh_dim_y));
    int dnode = 0;
    for (int i = 0; i < nbits - 1; i++)
	setBit(dnode, i + 1, getBit(local_id, i));
    setBit(dnode, 0, getBit(local_id, nbits - 1));

    Packet p;
    p.src_id = local_id;
    p.dst_id = dnode;

    p.vc_id = randInt(0,GlobalParams::n_virtual_channels-1);
    p.timestamp = sc_time_stamp().to_double() / GlobalParams::clock_period_ps;
    p.size = p.flit_left = getRandomSize();

    return p;
}

Packet ProcessingElement::trafficButterfly()
{

    int nbits = (int) log2ceil((double)
		 (GlobalParams::mesh_dim_x *
		  GlobalParams::mesh_dim_y));
    int dnode = 0;
    for (int i = 1; i < nbits - 1; i++)
	setBit(dnode, i, getBit(local_id, i));
    setBit(dnode, 0, getBit(local_id, nbits - 1));
    setBit(dnode, nbits - 1, getBit(local_id, 0));

    Packet p;
    p.src_id = local_id;
    p.dst_id = dnode;

    p.vc_id = randInt(0,GlobalParams::n_virtual_channels-1);
    p.timestamp = sc_time_stamp().to_double() / GlobalParams::clock_period_ps;
    p.size = p.flit_left = getRandomSize();

    return p;
}

// FM: Added trafficSlideUp function
Packet ProcessingElement::trafficSlideUp()
{
    int max_id;
    Packet p;
    p.src_id = local_id;
    //cout << "Source " << p.src_id;
    max_id = GlobalParams::mesh_dim_x * GlobalParams::mesh_dim_y - 1;
    // if (local_id%2==0) {
    //     if (local_id == max_id - 1) {
    //         p.dst_id = max_id;
    //     }
    //     else
    //         p.dst_id = local_id + 2;
    // }
    // else {
    //     if (local_id == 1) {
    //         p.dst_id = 0;
    //     }
    //     else
    //         p.dst_id = local_id - 2;
    // }
    // FM: TRY
    p.dst_id = (local_id + GlobalParams::slide_offset)%(max_id+1);
    //cout << " Destination  " << p.dst_id << endl;
    p.vc_id = randInt(0,GlobalParams::n_virtual_channels-1);
    p.timestamp = sc_time_stamp().to_double() / GlobalParams::clock_period_ps;
    p.size = p.flit_left = getRandomSize();

    return p;
}

Packet ProcessingElement::trafficFFTW1(int ring_id, int dst_id)
{
    Packet p;
    p.src_id = local_id;
    //cout << "Source " << p.src_id;
    if (ring_id == 0){
        if (local_id%2 == 0){
            p.dst_id = local_id + 1;
        }
        else
            p.dst_id = local_id - 1;
    }
    else
        p.dst_id = dst_id;
    return p;
}

void ProcessingElement::fixRanges(const Coord src,
				       Coord & dst)
{
    // Fix ranges
    if (dst.x < 0)
	dst.x = 0;
    if (dst.y < 0)
	dst.y = 0;
    if (dst.x >= GlobalParams::mesh_dim_x)
	dst.x = GlobalParams::mesh_dim_x - 1;
    if (dst.y >= GlobalParams::mesh_dim_y)
	dst.y = GlobalParams::mesh_dim_y - 1;
}

int ProcessingElement::getRandomSize()
{
    return randInt(GlobalParams::min_packet_size,
		   GlobalParams::max_packet_size);
}

unsigned int ProcessingElement::getQueueSize() const
{
    return packet_queue.size();
}

