#include "Routing_FT_ODD_EVEN.h"
#include "unordered_map"
#include <chrono>
#include <thread>
#define TIME_SIZE long long
// fault table node_id -> recovery_time;
std::unordered_map<int, TIME_SIZE> faultTable;

TIME_SIZE currentTime()
{
    auto current_time = std::chrono::system_clock::now();

    auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(current_time.time_since_epoch()).count();

    return millis;
}

void introduceFaultInNode(int node_id)
{
    if (node_id < 0)
    {
        return;
    }

    float random_val = static_cast<float>(rand()) / RAND_MAX;
    //std::cout << "guess : " << node_id << " -> " << random_val << endl;
    if (random_val < GlobalParams::fault_rate && faultTable.find(node_id) == faultTable.end())
    {
        // node is going to be fault if its not faulty already
        // TODO : introduce recovery rate as parameter
        // random recovery time between 1 to 10 units
        faultTable[node_id] = currentTime() + (rand() % 5);
 //       std::cout << "New fault " << node_id << endl;
    }
}

int recoveryTime(int node_id)
{
    if (faultTable.find(node_id) != faultTable.end())
    {
        return faultTable[node_id] - currentTime();
    }
    return 0;
}

// fault check
bool isNodeFaulty(int node_id)
{
    /* for (const auto &pair : faultTable)
    {
        std::cout << pair.first << " -> " << pair.second << std::endl;
    } */
    if (faultTable.find(node_id) != faultTable.end())
    {
        auto current_time = currentTime();
        if (faultTable[node_id] > current_time)
        {
            // node is faulty
            //std::cout << "node is fault " << current_time << "  " << faultTable[node_id] << "  " << node_id << endl;
            return true;
        }
        // Node has recovered
        faultTable.erase(node_id);
    }
    //std::cout << "node is not fault " << node_id << endl;
    return false;
}

void checkAndRoute(vector<int> &directions, int direction, int node_id, TIME_SIZE *least_recovery_time)
{
    if (!isNodeFaulty(node_id))
    {
        directions.push_back(direction);
    }
    else
    {
        TIME_SIZE recovery_time = recoveryTime(node_id);
        if (*least_recovery_time <= 0 || recovery_time < *least_recovery_time)
            *least_recovery_time = recovery_time;
    }
}

RoutingAlgorithmsRegister Routing_FT_ODD_EVEN::routingAlgorithmsRegister("FT_ODD_EVEN", getInstance());

Routing_FT_ODD_EVEN *Routing_FT_ODD_EVEN::routing_FT_ODD_EVEN = 0;

Routing_FT_ODD_EVEN *Routing_FT_ODD_EVEN::getInstance()
{
    if (routing_FT_ODD_EVEN == 0)
        routing_FT_ODD_EVEN = new Routing_FT_ODD_EVEN();

    return routing_FT_ODD_EVEN;
}

int faultTolerantRoute(vector<int> &directions,
                       int north_node_id, int east_node_id,
                       int south_node_id, int west_node_id,
                       int e0, int e1, int c0, int d0, int s0)
{
    TIME_SIZE least_recovery_time = 0;
    // update routing
    introduceFaultInNode(north_node_id);
    introduceFaultInNode(east_node_id);
    introduceFaultInNode(south_node_id);
    introduceFaultInNode(west_node_id);

    if (e0 == 0)
    { // Destination is in same column
        if (e1 > 0)
            checkAndRoute(directions, DIRECTION_NORTH, north_node_id, &least_recovery_time); // Destination is at top
        else
            checkAndRoute(directions, DIRECTION_SOUTH, south_node_id, &least_recovery_time); // Destination is at bottom
    }
    else
    {
        if (e0 > 0)
        {                                                                                      // Destination is at Right
            if (e1 == 0)                                                                       // Destination is in same row
                checkAndRoute(directions, DIRECTION_EAST, east_node_id, &least_recovery_time); // Send to right
            else
            { // Destination is not in same row
                if ((c0 % 2 == 1) || (c0 == s0))
                {               // current is at odd column or source & current is at same column
                    if (e1 > 0) // Destination is at top
                        checkAndRoute(directions, DIRECTION_NORTH, north_node_id, &least_recovery_time);
                    else // Destination is at bottom
                        checkAndRoute(directions, DIRECTION_SOUTH, south_node_id, &least_recovery_time);
                }
                if ((d0 % 2 == 1) || (e0 != 1)) // destination is at odd column or destination is not in immediate right
                    checkAndRoute(directions, DIRECTION_EAST, east_node_id, &least_recovery_time);
            }
        }
        else
        { // Destination is at Left
            checkAndRoute(directions, DIRECTION_WEST, west_node_id, &least_recovery_time);
            // go to left
            if (c0 % 2 == 0)
            {               // current is at even column
                if (e1 > 0) // destination is at top
                    checkAndRoute(directions, DIRECTION_NORTH, north_node_id, &least_recovery_time);
                if (e1 < 0) // destination is at bottom
                    checkAndRoute(directions, DIRECTION_SOUTH, south_node_id, &least_recovery_time);
            }
        }
    }
    return least_recovery_time;
}

vector<int> Routing_FT_ODD_EVEN::route(Router *router, const RouteData &routeData)
{
    Coord current = id2Coord(routeData.current_id);
    Coord destination = id2Coord(routeData.dst_id);
    Coord source = id2Coord(routeData.src_id);
    vector<int> directions;

    int c0 = current.x;
    int c1 = current.y;
    int s0 = source.x;
    int d0 = destination.x;
    int d1 = destination.y;
    int e0, e1;

    e0 = d0 - c0;
    e1 = -(d1 - c1);

    // introduce faults
    int north_node_id = router->getNeighborId(routeData.current_id, DIRECTION_NORTH);
    int east_node_id = router->getNeighborId(routeData.current_id, DIRECTION_EAST);
    int south_node_id = router->getNeighborId(routeData.current_id, DIRECTION_SOUTH);
    int west_node_id = router->getNeighborId(routeData.current_id, DIRECTION_WEST);

    int least_recovery_time = 0;
    while (directions.empty())
    {
        if (least_recovery_time > 0)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(least_recovery_time));
        }        
        least_recovery_time = faultTolerantRoute(directions,
                                                 north_node_id, east_node_id,
                                                 south_node_id, west_node_id,
                                                 e0, e1, c0, d0, s0);

        //fall back incase of node failure                                                 
        if (directions.size() == 0)
        {
            if (isNodeFaulty(north_node_id))            
                directions.push_back(DIRECTION_NORTH);        
            else if (isNodeFaulty(south_node_id))            
                directions.push_back(DIRECTION_SOUTH);            
            else if (isNodeFaulty(east_node_id))    
                directions.push_back(DIRECTION_EAST);         
            else if (isNodeFaulty(west_node_id))            
                directions.push_back(DIRECTION_WEST);        
        }
    }
    assert(directions.size() > 0 && directions.size() <= 2);

    return directions;
}