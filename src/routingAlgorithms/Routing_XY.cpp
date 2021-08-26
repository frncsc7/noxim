#include "Routing_XY.h"

RoutingAlgorithmsRegister Routing_XY::routingAlgorithmsRegister("XY", getInstance());

Routing_XY * Routing_XY::routing_XY = 0;

Routing_XY * Routing_XY::getInstance() {
	if ( routing_XY == 0 )
		routing_XY = new Routing_XY();
    
	return routing_XY;
}

vector<int> Routing_XY::route(Router * router, const RouteData & routeData)
{
    //cout << "Routing_XY::route: Source ID= "<<routeData.current_id<<endl;
    //cout << "Routing_XY::route: Dest ID= "<<routeData.dst_id<<endl;
    Coord current = id2Coord(routeData.current_id);
    Coord destination = id2Coord(routeData.dst_id);
    vector <int> directions;
    //cout << "Routing_XY::route : Source [X][Y]=["<<current.x<<"]["<<current.y<<"]"<<endl;
    //cout << "Routing_XY::route : Dest [X][Y]=["<<destination.x<<"]["<<destination.y<<"]"<<endl;
    if(GlobalParams::topology != TOPOLOGY_RING) { // FM: NEED TO SPECIFY NOW THAT THE id2Coor FUNCTION HAS CHANGED. TODO: CREATE A PROPER RING ROUTING ALGORITHM
        if (destination.x > current.x)
           directions.push_back(DIRECTION_EAST);
        else if (destination.x < current.x)
            directions.push_back(DIRECTION_WEST);
        else if (destination.y > current.y)
            directions.push_back(DIRECTION_SOUTH);
        else
            directions.push_back(DIRECTION_NORTH);
    }
    else {
        if (GlobalParams::bidirectionality) {
            if (destination.x > current.x) {
                if (destination.y > current.y) {
                    if (routeData.current_id == 0)
                        directions.push_back(DIRECTION_SOUTH);
                    else
                        directions.push_back(DIRECTION_EAST);
                }
                else {
                    if (routeData.current_id == (GlobalParams::mesh_dim_x * GlobalParams::mesh_dim_y -1))
                        directions.push_back(DIRECTION_NORTH);
                    else
                        directions.push_back(DIRECTION_EAST);
                }
            }
            else {
                if (destination.y > current.y) {
                    if ((routeData.current_id == 0) || (routeData.current_id == GlobalParams::mesh_dim_x - 1))
                        directions.push_back(DIRECTION_SOUTH);
                    else
                        directions.push_back(DIRECTION_WEST);
                }
                else {
                    if ((routeData.current_id == (GlobalParams::mesh_dim_x * GlobalParams::mesh_dim_y -1)) || (routeData.current_id == (GlobalParams::mesh_dim_x * GlobalParams::mesh_dim_y)/2))
                        directions.push_back(DIRECTION_NORTH);
                    else
                        directions.push_back(DIRECTION_WEST);
                }
            }
        } // Non-Bidirectional
        else {
            if (destination.x > current.x) {
                if (destination.y > current.y)
                    directions.push_back(DIRECTION_EAST);
                else {
                    if (routeData.current_id == GlobalParams::mesh_dim_x * GlobalParams::mesh_dim_y -1)
                        directions.push_back(DIRECTION_NORTH);
                    else if (destination.y < current.y)
                        directions.push_back(DIRECTION_WEST);
                    else
                        directions.push_back(DIRECTION_EAST);
                }
            }
            else {
                if (destination.y > current.y) {
                    //cout << "Destination=" << routeData.dst_id << " Source=" << routeData.current_id << endl;
                    if (routeData.current_id == GlobalParams::mesh_dim_x - 1)
                        directions.push_back(DIRECTION_SOUTH);
                    else {
                        if(routeData.current_id == GlobalParams::mesh_dim_x * GlobalParams::mesh_dim_y -1)
                            directions.push_back(DIRECTION_NORTH);
                        else
                            directions.push_back(DIRECTION_EAST);
                    }
                }
                else {
                    if (routeData.current_id == GlobalParams::mesh_dim_x * GlobalParams::mesh_dim_y -1)
                        directions.push_back(DIRECTION_NORTH);
                    else {
                        if (routeData.current_id == GlobalParams::mesh_dim_x - 1)
                            directions.push_back(DIRECTION_SOUTH);
                        else
                            directions.push_back(DIRECTION_WEST);
                    }
                }
            }
        }
    }
    return directions;
   }
