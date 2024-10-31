#ifndef __NOXIMROUTING_XY_H__
#define __NOXIMROUTING_XY_H__

#include "RoutingAlgorithm.h"
#include "RoutingAlgorithms.h"
#include "../Router.h"

using namespace std;

class Routing_FT_ODD_EVEN : RoutingAlgorithm {
	public:
		vector<int> route(Router * router, const RouteData & routeData);

		static Routing_FT_ODD_EVEN * getInstance();

	private:
		Routing_FT_ODD_EVEN(){};
		~Routing_FT_ODD_EVEN(){};

		static Routing_FT_ODD_EVEN * routing_FT_ODD_EVEN;
		static RoutingAlgorithmsRegister routingAlgorithmsRegister;
};

#endif
