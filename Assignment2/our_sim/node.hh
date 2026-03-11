#ifndef NODE_HH
#define NODE_HH

//Now, I am planning to extend this to Queuing Network. Where there is one client node, multiple junctions, multiple server machine (worker in my code). So, create a Node class that has child classes, Worker node, Junction node, Client node. Node class will have pointer to next node. Worker node will have a Worker and Receiver inside of it. Junction node will have a list of next possible nodes from that junction with a probability associated with them. Junction node also has a Multinomial distribution to decide which route the request will take from the Junction. Client node has nothing so far. Plan this and create a system design for this.
#include "common.hh"

class Node {
public:
    int id;
    std::string name;

    virtual void process(Event* event, Sim* sim) = 0;
    virtual Node* get_next(Request* req) = 0;

    virtual ~Node() {}
};

class ServerNode : public Node {
public:
    Receiver receiver;
    Worker worker;

    Node* next_node;

    ServerNode(int num_threads,
               int receiver_buffer_size,
               int total_cores,
               int thread_buffer_size,
               int core_buffer_size,
               double core_context_switch_time,
               double core_context_switch_overhead)
        : receiver(num_threads, receiver_buffer_size),
          worker(total_cores, thread_buffer_size, core_buffer_size, core_context_switch_time, core_context_switch_overhead) {}

    void process(Event* event, Sim* sim) override;

    Node* get_next(Request* req) override {
        return next_node;
    }
};

class ClientNode : public Node {
public:
    Distribution* think_time;
    Node* next_node;

    void process(Event* event, Sim* sim) override;

    Node* get_next(Request* req) override {
        return next_node;
    }
};

class JunctionNode : public Node {
public:
    std::vector<Node*> next_nodes;

    std::discrete_distribution<> dist;
    std::mt19937 gen;

    JunctionNode(std::vector<Node*> nodes,
                 std::vector<double> probs)
        : next_nodes(nodes),
          dist(probs.begin(), probs.end()) {}

    Node* get_next(Request* req) override {
        return next_nodes[dist(gen)];
    }

    void process(Event* event, Sim* sim) override;
};

#endif