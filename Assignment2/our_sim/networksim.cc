#include "common.hh"

// Config file format:
// num_nodes max_time
// For each node:
// node_type (client/server)
// If client: num_users, think_time_distribution_type, think_time_distribution_params
// If server: num_threads, receiver_buffer_size, total_cores, thread_buffer_size, core_buffer_size, core_context_switch_time, core_context_switch_overhead, service_time_distribution_type, service_time_distribution_params
// Adjacency Matrix (num_nodes x num_nodes) : containing probabilities of routing from node i to node j

NetworkSim::NetworkSim(std::string input_file) {
    std::ifstream config_file(input_file);
    if (!config_file.is_open()) {
        std::cerr << "Error opening config file!" << std::endl;
        exit(1);
    }

    // Parse the config file to initialize the network topology and node parameters
    int num_nodes;
    config_file >> num_nodes;
    double max_time;
    config_file >> max_time;
    std::vector<std::vector<double>> adjacency_matrix(num_nodes, std::vector<double>(num_nodes, 0.0));
    for (int i = 0; i < num_nodes; i++) {
        std::string node_type;
        config_file >> node_type;
        if (node_type == "CLIENT") {
            int num_users;
            std::string think_dist_type;
            config_file >> num_users >> think_dist_type;
            Distribution* think_time_dist;
            if (think_dist_type == "UNIFORM") {
                double min_think_time, max_think_time;
                config_file >> min_think_time >> max_think_time;
                think_time_dist = new UniformDistribution(min_think_time, max_think_time);
            } else if (think_dist_type == "EXPONENTIAL") {
                double lambda;
                config_file >> lambda;
                think_time_dist = new ExponentialDistribution(lambda);
            } else if (think_dist_type == "DETERMINISTIC") {
                double constant_time;
                config_file >> constant_time;
                think_time_dist = new ConstDistribution(constant_time);
            } else {
                std::cerr << "Unknown think time distribution type in config file!" << std::endl;
                exit(1);
            }
            ClientNode* client_node = new ClientNode(num_users, think_time_dist); 
            client_node->id = i; // Assign id to the node
            client_nodes.push_back(client_node);
        } else if (node_type == "SERVER") {
            int num_threads, receiver_buffer_size, total_cores, thread_buffer_size, core_buffer_size;
            double core_context_switch_time, core_context_switch_overhead;
            std::string service_dist_type;
            config_file >> num_threads >> receiver_buffer_size >> total_cores >> thread_buffer_size >> core_buffer_size >> core_context_switch_time >> core_context_switch_overhead >> service_dist_type;
            Distribution* service_time_dist;
            if (service_dist_type == "UNIFORM") {
                double min_service_time, max_service_time;
                config_file >> min_service_time >> max_service_time;
                service_time_dist = new UniformDistribution(min_service_time, max_service_time);
            } else if (service_dist_type == "EXPONENTIAL") {
                double lambda;
                config_file >> lambda;
                service_time_dist = new ExponentialDistribution(lambda);
            } else if (service_dist_type == "DETERMINISTIC") {
                double constant_time;
                config_file >> constant_time;
                service_time_dist = new ConstDistribution(constant_time);
            } else {
                std::cerr << "Unknown service time distribution type in config file!" << std::endl;
                exit(1);
            }
            ServerNode* server_node = new ServerNode(num_threads, receiver_buffer_size, total_cores, thread_buffer_size, core_buffer_size, core_context_switch_time, core_context_switch_overhead, service_time_dist, 0);
            server_node->id = i; // Assign id to the node
            server_nodes.push_back(server_node);
        } else {
            std::cerr << "Unknown node type in config file!" << std::endl;
            exit(1);
        }
    }

    // fill next_nodes and next_node_dist for each node based on the adjacency matrix
    for (ClientNode* client_node : client_nodes) {
        for (ClientNode* other_client_node : client_nodes) {
            double prob;
            config_file >> prob;
            client_node->next_node_dist.push_back(prob);
            client_node->next_nodes.push_back(other_client_node);
        }
        for (ServerNode* server_node : server_nodes) {
            double prob;
            config_file >> prob;
            client_node->next_node_dist.push_back(prob);
            client_node->next_nodes.push_back(server_node);
        }
    }
    for (ServerNode* server_node : server_nodes) {
        for (ClientNode* client_node : client_nodes) {
            double prob;
            config_file >> prob;
            server_node->next_node_dist.push_back(prob);
            server_node->next_nodes.push_back(client_node);
        }
        for (ServerNode* other_server_node : server_nodes) {
            double prob;
            config_file >> prob;
            server_node->next_node_dist.push_back(prob);
            server_node->next_nodes.push_back(other_server_node);
        }
    }
}


// print the configuration of the network simulation
// in the same format as the config file
void print_config(){
    std::cout << "Number of Client Nodes: " << client_nodes.size() << std::endl;
    std::cout << "Number of Server Nodes: " << server_nodes.size() << std::endl;
    std::cout << "Max Simulation Time: " << max_time << std::endl;
    for (ClientNode* client_node : client_nodes) {
        std::cout << "Client Node ID: " << client_node->id << std::endl;
        std::cout << "Number of Users: " << client_node->num_users << std::endl;
        std::cout << "Think Time Distribution: " << std::endl;
        if (dynamic_cast<UniformDistribution*>(client_node->think_time_dist)) {
            std::cout << "Uniform" << std::endl;
            std::cout << "Min Think Time: " << dynamic_cast<UniformDistribution*>(client_node->think_time_dist)->a << std::endl;
            std::cout << "Max Think Time: " << dynamic_cast<UniformDistribution*>(client_node->think_time_dist)->b << std::endl;
        } else if (dynamic_cast<ExponentialDistribution*>(client_node->think_time_dist)) {
            std::cout << "Exponential" << std::endl;
            std::cout << "Lambda: " << dynamic_cast<ExponentialDistribution*>(client_node->think_time_dist)->mean << std::endl;
        } else if (dynamic_cast<ConstDistribution*>(client_node->think_time_dist)) {
            std::cout << "Deterministic" << std::endl;
            std::cout << "Think Time: " << dynamic_cast<ConstDistribution*>(client_node->think_time_dist)->value << std::endl;
        } else {
            std::cout << "Unknown" << std::endl;
        }
    }
    for (ServerNode* server_node : server_nodes) {
        std::cout << "Server Node ID: " << server_node->id << std::endl;
        std::cout << "Number of Threads: " << server_node->num_threads << std::endl;
        std::cout << "Receiver Buffer Size: " << server_node->receiver_buffer_size << std::endl;
        std::cout << "Total Cores: " << server_node->total_cores << std::endl;
        std::cout << "Thread Buffer Size: " << server_node->thread_buffer_size << std::endl;
        std::cout << "Core Buffer Size: " << server_node->core_buffer_size << std::endl;
        std::cout << "Core Context Switch Time: " << server_node->core_context_switch_time << std::endl;
        std::cout << "Core Context Switch Overhead: " << server_node->core_context_switch_overhead << std::endl;
        std::cout << "Service Time Distribution: " << std::endl;
        if (dynamic_cast<UniformDistribution*>(server_node->service_time_dist)) {
            std::cout << "Uniform" << std::endl;
            std::cout << "Min Service Time: " << dynamic_cast<UniformDistribution*>(server_node->service_time_dist)->a << std::endl;
            std::cout << "Max Service Time: " << dynamic_cast<UniformDistribution*>(server_node->service_time_dist)->b << std::endl;
        } else if (dynamic_cast<ExponentialDistribution*>(server_node->service_time_dist)) {
            std::cout << "Exponential" << std::endl;
            std::cout << "Lambda: " << dynamic_cast<ExponentialDistribution*>(server_node->service_time_dist)->mean << std::endl;
        } else if (dynamic_cast<ConstDistribution*>(server_node->service_time_dist)) {
            std::cout << "Deterministic" << std::endl;
            std::cout << "Service Time: " << dynamic_cast<ConstDistribution*>(server_node->service_time_dist)->value << std::endl;
        } else {
            std::cout << "Unknown" << std::endl;
        }
    }
    std::cout << "Adjacency Matrix:" << std::endl;
    for (ClientNode* client_node : client_nodes) {
        for (ClientNode* other_client_node : client_nodes) {
            std::cout << client_node->next_node_dist[other_client_node->id] << " ";
        }
        for (ServerNode* server_node : server_nodes) {
            std::cout << client_node->next_node_dist[server_node->id] << " ";
        }
        std::cout << std::endl;
    }
    for (ServerNode* server_node : server_nodes) {
        for (ClientNode* client_node : client_nodes) {
            std::cout << server_node->next_node_dist[client_node->id] << " ";
        }
        for (ServerNode* other_server_node : server_nodes) {
            std::cout << server_node->next_node_dist[other_server_node->id] << " ";
        }
        std::cout << std::endl;
    }
}


void NetworkSim::run() {
    std::ofstream output_file("network_event_log.txt");

    output_file << "------------------------" << std::endl;

    while (!event_queue.empty() && event_queue.top()->timestamp < max_time) {
        output_file << "Current Time: " << event_queue.top()->timestamp << std::endl;
        Event* current_event = event_queue.top();
        event_queue.pop();
        // Print the current event details to the output file
        output_file << "Event Type: " << event_type_to_string(current_event->type) << std::endl;
        // Print request id, thread id, core id if they exist
        if (current_event->request) {
            output_file << "Request ID: " << current_event->request->id << std::endl;
        }
        if (current_event->thread) {
            output_file << "Thread ID: " << current_event->thread->id << std::endl;
        }
        if (current_event->core) {
            output_file << "Core ID: " << current_event->core->id << std::endl;
        }
        output_file << "Node ID: " << current_event->node->id << std::endl; // Print the node id where the event is happening
        output_file << "------------------------" << std::endl;
        // Process the current event based on its type
        current_event->node->process(current_event, this); 
    }
}
