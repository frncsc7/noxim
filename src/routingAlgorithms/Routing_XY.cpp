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
    if (destination.x > current.x)
       directions.push_back(DIRECTION_EAST);
    else if (destination.x < current.x)
        directions.push_back(DIRECTION_WEST);
    else if (destination.y > current.y)
        directions.push_back(DIRECTION_SOUTH);
    else
        directions.push_back(DIRECTION_NORTH);

    return directions;
   } 
