#ifndef NSIM_HH
#define NSIM_HH

#include "common.hh"

class NetworkSim {
    std::vector<ClientNode*> client_nodes;
    std::vector<ServerNode*> server_nodes;
    double max_time;
    // Store all requests for statistics
    std::vector<Request*> all_requests;
public:
    std::priority_queue<Event*, std::vector<Event*>, EventComparator> event_queue;
    int id_of(Node* node) {
        for(int i=0; i<all_nodes.size(); i++){
            if(all_nodes[i] == node){
                return i;
            }
        }
        return -1; // Return -1 if node not found
    }
    NetworkSim(std::string input_file);
    void print_config();
    void run();
    ~NetworkSim();
};

#endif