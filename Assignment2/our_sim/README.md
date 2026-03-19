# 🛠️ Build & Usage Guide

## 📦 Prerequisites

Ensure the following are installed:

- **C++ Compiler** (supports C++17, e.g., `g++-15`)
- **simdjson** library
- **CMake** (version 3.10 or higher)

---

## 🏗️ Building the Project

This project uses a Makefile for building. Follow these steps:

#### Compile the project:

```bash
make
```

This will:
1. Compile all the source files (`*.cc`) into object files (`*.o`).
2. Link with simdjson
3. Generate the executable:

```bash
networksim
```

---

## 🚀 Running the Simulation

```bash
./networksim <config_file.json>
```

---

## 📥 Input Configuration

The simulator takes a JSON configuration file as input. It defines:

#### Global Parameters
- `num_runs`: Number of simulation runs
- `max_time`: Maximum simulation time
- `output_file`: CSV file to store results

#### Client Node Configuration

- User range:
  - `min_num_users`, `max_num_users`, `step_size`

- Think time distribution:
  - `DETERMINISTIC`, `UNIFORM`, `EXPONENTIAL`

- Timeout configuration:
  - `min_timeout`

- Timeout distribution:
  - `DETERMINISTIC`, `UNIFORM`, `EXPONENTIAL`

- `resend_on_timeout`: true/false

#### Server Node Configuration

- `num_threads`, `total_cores`

- Buffer sizes:
  - `receiver_buffer_size`
  - `thread_buffer_size`
  - `core_buffer_size`

- Scheduling policies:
  - `FCFS`, `SJF`, `RR`

- Context switching:
  - `core_context_switch_time`
  - `core_context_switch_overhead`

- Service time distribution

#### Network Topology
- `adjacency_matrix`: Routing probabilities between nodes

---

## 📊 Output Format

The simulator generates a CSV file specified by `output_file`.

Each row corresponds to a specific user count.

---

## 🔹 Core Performance Metrics

- `user_count`: Number of users in the system
- `avg_response_time`: Average request completion time
- `good_throughput`: Successful requests per unit time
- `bad_throughput`: Timed-out requests per unit time
- `total_throughput`: Total completed requests per unit time
- `drop_percentage`: Fraction of dropped requests

---

## Resource Utilization Metrics

- `server_X_utilization`: CPU utilization of server X
- `average_number_of_requests_in_server_X`: Average number of requests at server X

## 🔹 Statistical Metrics

Confidence intervals for response time:
- `ci_90_lower`, `ci_90_upper`
- `ci_95_lower`, `ci_95_upper`
- `ci_99_lower`, `ci_99_upper`

## 🔹 Analytical Validation Metrics
Little’s Law Verification

`N_calculated_from_little_s_law` = λ × R
`N_calculated_from_simulation`

Validates:
`N = λ × R`

Mean Value Analysis (MVA)
`R_calculated_from_mva` = S × (1 + Q(N-1))
`R_calculated_from_simulation`

Validates:
`R(N) = S × (1 + Q(N-1))`

---

## Cleaning Build Files

```bash
make clean
```

Removes:
- Object files (`*.o`)
- Executable (`networksim`)

---

# Visualization: Plotting Simulation Results

A Python script is provided to visualize the simulation output CSV file.

---

## Requirements

Install required libraries:

```bash
pip install matplotlib numpy os
```

---

## Usage

```bash
python plot_graphs.py
```

By default, it reads `tester.csv` and generates plots in a directory names after the input file (without extension).

Modify `input_file` and `num_servers` in the `__main__` block as needed.

## Output Structure

If input file is:

```bash
tester.csv
```

Output folder:

```bash
tester/
```