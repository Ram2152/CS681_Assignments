#ifndef SIM_HH
#define SIM_HH

#include "common.hh"

class Sim {
    Receiver receiver;
    Worker worker;
    std::priority_queue<Event*, std::vector<Event*>, EventComparator> event_queue;
    Distribution arrival_time_dist;
    Distribution service_time_dist;
public:
    Sim(Config config);
    void run();
};

#endif