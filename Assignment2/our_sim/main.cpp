#include "common.hh"

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <config_file>" << std::endl;
        return 1;
    }
    std::string config_file = argv[1];
    Config config(config_file);
    Sim sim(config);
    sim.print_config();
    sim.run();
    sim.print_stats();
    return 0;
}