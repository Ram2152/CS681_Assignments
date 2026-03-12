#ifndef NSIM_HH
#define NSIM_HH

#include "common.hh"

class ClientNode;
class ServerNode;

class NetworkSim {
    std::vector<ClientNode*> client_nodes;
    std::vector<ServerNode*> server_nodes;
    double max_time;
    // Store all requests for statistics
public:
    std::vector<Request*> all_requests;
    std::priority_queue<Event*, std::vector<Event*>, EventComparator> event_queue;
    NetworkSim(std::string input_file);
    void print_config();
    void run();
    std::tuple<double, double, double, double, double> print_stats();
    ~NetworkSim();
};

#endif