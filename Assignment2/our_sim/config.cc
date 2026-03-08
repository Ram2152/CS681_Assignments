#include "common.hh"

Config::Config(std::string input_file_name) {
    Config::input_file = input_file_name;

    std::ifstream config_file(input_file);
    if (!config_file.is_open()) {
        std::cerr << "Error opening config file!" << std::endl;
        exit(1);
    }
    // Read configuration parameters from the file
    // Thread count, core count, arrival time distribution, service time distribution
    std::string arrival_dist, service_dist;
    config_file >> num_threads >> total_cores >> request_buffer_size >> thread_buffer_size >> arrival_dist >> service_dist;

    if (arrival_dist == "UNIFORM") {
        arrival_time_distribution = TimeDistributionType::UNIFORM;
    } else if (arrival_dist == "EXPONENTIAL") {
        arrival_time_distribution = TimeDistributionType::EXPONENTIAL;
    } else if (arrival_dist == "DETERMINISTIC") {
        arrival_time_distribution = TimeDistributionType::DETERMINISTIC;
    } else {
        std::cerr << "Invalid arrival time distribution in config file!" << std::endl;
        exit(1);
    }

    if (service_dist == "UNIFORM") {
        service_time_distribution = TimeDistributionType::UNIFORM;
    } else if (service_dist == "EXPONENTIAL") {
        service_time_distribution = TimeDistributionType::EXPONENTIAL;
    } else if (service_dist == "DETERMINISTIC") {
        service_time_distribution = TimeDistributionType::DETERMINISTIC;
    } else {
        std::cerr << "Invalid service time distribution in config file!" << std::endl;
        exit(1);
    }
}