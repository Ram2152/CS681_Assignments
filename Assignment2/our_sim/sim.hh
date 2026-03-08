#ifndef SIM_HH
#define SIM_HH

#include "common.hh"

class Sim {
    int num_users;
    double timeout;
    double max_time;
    Receiver receiver;
    Worker worker;
    std::priority_queue<Event*, std::vector<Event*>, EventComparator> event_queue;
    Distribution* think_time_dist;
    Distribution* service_time_dist;
    // Store all requests for statistics
    std::vector<Request*> all_requests;
public:
    Sim(Config config);
    void print_config();
    void run();
    void print_stats();
    ~Sim();
};

#endif