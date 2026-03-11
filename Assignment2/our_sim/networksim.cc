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
        std::vector<double> probs;
        for (ClientNode* other_client_node : client_nodes) {
            double prob;
            config_file >> prob;
            probs.push_back(prob);
            client_node->next_nodes.push_back(other_client_node);
        }
        for (ServerNode* server_node : server_nodes) {
            double prob;
            config_file >> prob;
            probs.push_back(prob);
            client_node->next_nodes.push_back(server_node);
        }
        client_node->next_node_dist = std::discrete_distribution<int>(probs.begin(), probs.end());
    }
    for (ServerNode* server_node : server_nodes) {
        std::vector<double> probs;
        for (ClientNode* client_node : client_nodes) {
            double prob;
            config_file >> prob;
            probs.push_back(prob);
            server_node->next_nodes.push_back(client_node);
        }
        for (ServerNode* other_server_node : server_nodes) {
            double prob;
            config_file >> prob;
            probs.push_back(prob);
            server_node->next_nodes.push_back(other_server_node);
        }
        server_node->next_node_dist = std::discrete_distribution<int>(probs.begin(), probs.end());
    }
}