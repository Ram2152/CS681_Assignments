#ifndef NSIM_HH
#define NSIM_HH

#include "common.hh"

class ClientNode;
class ServerNode;

class NetworkSim {
    public:
    std::vector<ClientNode*> client_nodes;
    std::vector<ServerNode*> server_nodes;
    // Store all requests for statistics
    int min_num_users, max_num_users, step_size;
    double max_time;
    std::vector<Request*> all_requests;
    std::priority_queue<Event*, std::vector<Event*>, EventComparator> event_queue;
    NetworkSim(std::string input_file);
    void print_config();
    void run();
    void set_num_users(int ClientNodeNum, int num_users);
    std::tuple<double, double, double, double, double, std::vector<std::tuple<int, int, double>>> print_stats();
    ~NetworkSim();
};

#endif