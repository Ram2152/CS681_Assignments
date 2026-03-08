#include "common.hh"

Config::Config(int num_threads, int total_cores, TimeDistributionType arrival_time_distribution, TimeDistributionType service_time_distribution) {
    this->num_threads = num_threads;
    this->total_cores = total_cores;
    this->arrival_time_distribution = arrival_time_distribution;
    this->service_time_distribution = service_time_distribution;
}