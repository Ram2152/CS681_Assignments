#ifndef NODE_HH
#define NODE_HH

#include "common.hh"

class NetworkSim;

class Node {
public:
    int id;
    std::string name;
    std::vector<Node*> next_nodes; 
    std::discrete_distribution<int> next_node_dist; 
    std::mt19937 gen;

    Node* get_next() {
        return next_nodes[next_node_dist(gen)];
    }

    virtual void process(Event* event, NetworkSim* sim) = 0;
    virtual ~Node() {}
};

class ServerNode : public Node {
public:
    Receiver receiver;
    Worker worker;
    Distribution* service_time_dist;

    ServerNode(int num_threads,
               int receiver_buffer_size,
               int total_cores,
               int thread_buffer_size,
               int core_buffer_size,
               double core_context_switch_time,
               double core_context_switch_overhead,
               Distribution* service_time_dist)
        : receiver(num_threads, receiver_buffer_size),
          worker(total_cores, thread_buffer_size, core_buffer_size, core_context_switch_time, core_context_switch_overhead),
          service_time_dist(service_time_dist) {}

    void process(Event* event, NetworkSim* sim) override;
};

class ClientNode : public Node {
public:
    int num_users;
    Distribution* think_time;

    ClientNode(int num_users, Distribution* think_time) : num_users(num_users), think_time(think_time) {}

    void process(Event* event, NetworkSim* sim) override;
};

#endif