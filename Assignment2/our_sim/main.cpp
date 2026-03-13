#include "common.hh"

// Calculate confidence intervals for response time data
std::tuple<double, double> calculate_confidence_interval(const std::vector<double>& data, double confidence_level) {
    int n = data.size();
    if (n == 0) {
        return std::make_tuple(0, 0); // No data, return (0, 0) as confidence interval
    }
    
    // Calculate mean
    double mean = std::accumulate(data.begin(), data.end(), 0.0) / n;

    // Calculate standard deviation
    double variance = std::accumulate(data.begin(), data.end(), 0.0, [mean](double acc, double x) {
        return acc + (x - mean) * (x - mean);
    }) / n;
    double stddev = std::sqrt(variance);

    // Z-score for the given confidence level (e.g., 1.96 for 95% confidence)
    double z_score;
    if (confidence_level == 0.95) {
        z_score = 1.96;
    } else if (confidence_level == 0.90) {
        z_score = 1.645;
    } else if (confidence_level == 0.99) {
        z_score = 2.576;
    } else {
        std::cerr << "Unsupported confidence level! Using 95% confidence by default." << std::endl;
        z_score = 1.96;
    }

    // Calculate margin of error
    double margin_of_error = z_score * (stddev / std::sqrt(n));

    // Calculate confidence interval
    double lower_bound = mean - margin_of_error;
    double upper_bound = mean + margin_of_error;

    return std::make_tuple(lower_bound, upper_bound);
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <config_file>" << std::endl;
        return 1;
    }
    std::string config_file = argv[1];
    
    std::ifstream infile(config_file);
    if (!infile.good()) {
        std::cerr << "Error: Config file not found!" << std::endl;
        return 1;
    }

    int num_of_runs;
    infile >> num_of_runs;
    infile.close();

    NetworkSim sim(config_file);
    sim.print_config();

    std::vector<double> avg_response_times;
    std::vector<double> good_throughputs;
    std::vector<double> bad_throughputs;
    std::vector<double> total_throughputs;
    std::vector<double> drop_rates;
    std::vector<std::vector<std::tuple<int, int, double>>> all_cpu_times;

    for (int run = 0; run < num_of_runs; run++) {
        std::cout << "Run " << run + 1 << "..." << std::endl;

        sim.run();
        auto stats = sim.print_stats();
        avg_response_times.push_back(std::get<0>(stats));
        good_throughputs.push_back(std::get<1>(stats));
        bad_throughputs.push_back(std::get<2>(stats));
        total_throughputs.push_back(std::get<3>(stats));
        drop_rates.push_back(std::get<4>(stats));
        all_cpu_times.push_back(std::get<5>(stats));
    }

    for (int run = 0; run < num_of_runs; run++) {
        std::cout << "======================" << std::endl;
        std::cout << "Run " << run + 1 << " of " << num_of_runs << " Summary:" << std::endl;
        std::cout << "Average Response Time: " << avg_response_times[run] << " seconds" << std::endl;
        std::cout << "Good Throughput: " << good_throughputs[run] << " req/sec" << std::endl;
        std::cout << "Bad Throughput: " << bad_throughputs[run] << " req/sec" << std::endl;
        std::cout << "Total Throughput: " << total_throughputs[run] << " req/sec" << std::endl;
        std::cout << "Request Drop Rate: " << drop_rates[run] * 100 << " req/sec" << std::endl;
        // For each core, divide the total busy time by max_time to get the CPU utilization percentage
        // For each server node, print the CPU utilization of each core
        // For each server node, print the CPU utilization of the server node as a whole (total busy time of all cores divided by (number of cores * max_time))
        // Iterate through all servernodes, and for each core, print the total busy time and utilization percentage
        std::cout << "CPU Utilization:" << std::endl;
        std::vector<std::tuple<int, int, double>> cpu_times = all_cpu_times[run];
        std::map<int, std::vector<std::tuple<int, double>>> server_core_times; // server_id -> vector of (core_id, busy_time)
        for (const auto& entry : cpu_times) {
            int server_id = std::get<0>(entry);
            int core_id = std::get<1>(entry);
            double busy_time = std::get<2>(entry);
            server_core_times[server_id].push_back({core_id, busy_time});
        }
        for (const auto& server_entry : server_core_times) {
            int server_id = server_entry.first;
            const auto& core_times = server_entry.second;
            double total_busy_time = 0;
            std::cout << "  Server Node " << server_id << ":" << std::endl;
            for (const auto& core_entry : core_times) {
                int core_id = std::get<0>(core_entry);
                double busy_time = std::get<1>(core_entry);
                total_busy_time += busy_time;
                double utilization = (busy_time / sim.max_time) * 100;
                std::cout << "    Core " << core_id << ": Busy Time = " << busy_time << " seconds, Utilization = " << utilization << "%" << std::endl;
            }
            double server_utilization = (total_busy_time / (core_times.size() * sim.max_time)) * 100;
            std::cout << "  Server Node " << server_id << " Overall Utilization: " << server_utilization << "%" << std::endl;   
        }

        std::cout << "======================" << std::endl;
        std::cout << std::endl;
    }

    std::vector<double> confidence_levels = {0.90, 0.95, 0.99};
    for (double confidence_level : confidence_levels) {
        auto ci = calculate_confidence_interval(avg_response_times, confidence_level);
        std::cout << "Confidence Interval for Average Response Time at " << confidence_level * 100 << "% confidence: [" << std::get<0>(ci) << ", " << std::get<1>(ci) << "] seconds" << std::endl;
    }

    return 0;
}