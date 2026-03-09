#include "common.hh"

Config::Config(std::string input_file_name) {
    Config::input_file = input_file_name;

    std::ifstream config_file(input_file);
    if (!config_file.is_open()) {
        std::cerr << "Error opening config file!" << std::endl;
        exit(1);
    }
    // Read configuration parameters from the file
    // Thread count, core count, think time distribution, service time distribution
    std::string think_dist, service_dist;
    int num_of_runs;
    config_file >> num_of_runs >> num_threads >> total_cores >> num_users >> timeout >> max_time >> request_buffer_size >> thread_buffer_size >> core_buffer_size >> core_context_switch_time >> core_context_switch_overhead >> think_dist >> service_dist;

    if (think_dist == "UNIFORM") {
        think_time_distribution = TimeDistributionType::UNIFORM;
    } else if (think_dist == "EXPONENTIAL") {
        think_time_distribution = TimeDistributionType::EXPONENTIAL;
    } else if (think_dist == "DETERMINISTIC") {
        think_time_distribution = TimeDistributionType::DETERMINISTIC;
    } else {
        std::cerr << "Invalid think time distribution in config file!" << std::endl;
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