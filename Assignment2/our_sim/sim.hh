#ifndef SIM_HH
#define SIM_HH

#include "common.hh"

class Sim {
    int num_users;
    double timeout;
    double max_time;
    Receiver receiver;
    Worker worker;
    Distribution* service_time_dist;
    // Store all requests for statistics
    std::vector<Request*> all_requests;
public:
    Distribution* think_time_dist;
    std::priority_queue<Event*, std::vector<Event*>, EventComparator> event_queue;
    Sim(Config config);
    void print_config();
    void run();
    std::tuple<double, double, double, double, double, double> print_stats();
    ~Sim();
};

#endif