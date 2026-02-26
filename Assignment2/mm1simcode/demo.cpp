#include <iostream>
#include <fstream>
#include <cmath>
#include <cstdlib>
#include "lcgrand.h"

using namespace std;

#define Q_LIMIT 3000
#define BUSY 1
#define IDLE 0
#define MAXRUNS 50
#define MAXDELAYS 5000

// Global variables
int next_event_type, num_custs_delayed, num_delays_required, num_events;
int num_in_q, server_status, num_of_repetitions;
int demo, run = 0;

float area_num_in_q, area_server_status;
float mean_interarrival, mean_service;
float sim_time, time_arrival[Q_LIMIT + 1];
float time_last_event, time_next_event[3];
float total_of_delays, alldelays[MAXRUNS][MAXDELAYS];

long seed;
int stream, j, i;

// File streams
ifstream infile("mm1.in");
ofstream outfile("mm1.out");
ofstream delayfile("delayfile");

// Function declarations
void initialize();
void timing();
void arrive();
void depart();
void report();
void update_time_avg_stats();
void printstate();
float expon(float mean);

// Main
int main()
{
    if (!infile.is_open())
    {
        cout << "Error opening mm1.in\n";
        return 1;
    }

    num_events = 2;

    // Read inputs from file
    infile >> demo;
    infile >> mean_interarrival >> mean_service >> num_delays_required;

    // Write header
    outfile << "Single-server queueing system\n\n";

    outfile << "Mean interarrival time "
            << mean_interarrival << " minutes\n\n";

    outfile << "Mean service time "
            << mean_service << " minutes\n\n";

    outfile << "Number of customers "
            << num_delays_required << "\n\n";

    infile >> num_of_repetitions;

    for (run = 0; run < num_of_repetitions; run++)
    {
        cout << "Run " << run << endl;

        infile >> seed >> stream;

        lcgrandst(seed, stream);

        initialize();

        while (num_custs_delayed < num_delays_required)
        {
            if (demo)
                printstate();

            timing();

            update_time_avg_stats();

            switch (next_event_type)
            {
                case 1:
                    arrive();
                    break;

                case 2:
                    depart();
                    break;
            }
        }

        report();
    }

    // Write delayfile
    for (j = 0; j < num_delays_required; j++)
    {
        for (i = 0; i < num_of_repetitions; i++)
        {
            delayfile << alldelays[i][j] << " ";
        }
        delayfile << endl;
    }

    infile.close();
    outfile.close();
    delayfile.close();

    return 0;
}

// Initialize simulation
void initialize()
{
    sim_time = 0.0;

    server_status = IDLE;
    num_in_q = 0;
    time_last_event = 0.0;

    num_custs_delayed = 0;
    total_of_delays = 0.0;
    area_num_in_q = 0.0;
    area_server_status = 0.0;

    time_next_event[1] = sim_time + expon(mean_interarrival);
    time_next_event[2] = 1.0e30;
}

// Timing function
void timing()
{
    float min_time_next_event = 1.0e29;

    next_event_type = 0;

    for (int i = 1; i <= num_events; i++)
    {
        if (time_next_event[i] < min_time_next_event)
        {
            min_time_next_event = time_next_event[i];
            next_event_type = i;
        }
    }

    if (next_event_type == 0)
    {
        outfile << "Event list empty at time "
                << sim_time << endl;
        exit(1);
    }

    sim_time = min_time_next_event;
}

// Arrival event
void arrive()
{
    float delay;

    time_next_event[1] = sim_time + expon(mean_interarrival);

    if (server_status == BUSY)
    {
        num_in_q++;

        if (num_in_q > Q_LIMIT)
        {
            outfile << "Queue overflow at time "
                    << sim_time << endl;
            exit(2);
        }

        time_arrival[num_in_q] = sim_time;
    }
    else
    {
        delay = 0.0;

        total_of_delays += delay;
        alldelays[run][num_custs_delayed] = delay;

        num_custs_delayed++;

        server_status = BUSY;

        time_next_event[2] = sim_time + expon(mean_service);
    }
}

// Departure event
void depart()
{
    float delay;

    if (num_in_q == 0)
    {
        server_status = IDLE;
        time_next_event[2] = 1.0e30;
    }
    else
    {
        num_in_q--;

        delay = sim_time - time_arrival[1];

        total_of_delays += delay;

        alldelays[run][num_custs_delayed] = delay;

        num_custs_delayed++;

        time_next_event[2] = sim_time + expon(mean_service);

        for (int i = 1; i <= num_in_q; i++)
        {
            time_arrival[i] = time_arrival[i + 1];
        }
    }
}

// Report
void report()
{
    outfile << "\nAverage delay in queue "
            << total_of_delays / num_custs_delayed
            << " minutes\n";

    outfile << "Average number in queue "
            << area_num_in_q / sim_time << endl;

    outfile << "Server utilization "
            << area_server_status / sim_time << endl;

    outfile << "Simulation ended at "
            << sim_time << " minutes\n";
}

// Update statistics
void update_time_avg_stats()
{
    float time_since_last_event =
        sim_time - time_last_event;

    time_last_event = sim_time;

    area_num_in_q += num_in_q * time_since_last_event;

    area_server_status +=
        server_status * time_since_last_event;
}

// Exponential random variable
float expon(float mean)
{
    return -mean * log(lcgrand(stream));
}

// Debug print
void printstate()
{
    cout << "Sim Time = " << sim_time << " | ";

    if (server_status == IDLE)
        cout << "Server Idle | ";
    else
        cout << "Server Busy | ";

    if (num_in_q)
    {
        cout << num_in_q << " in queue: ";

        for (int i = 1; i <= num_in_q; i++)
            cout << time_arrival[i] << " ";
    }
    else
        cout << "Queue Empty ";

    cout << "| Next Arrival: "
         << time_next_event[1];

    if (time_next_event[2] >= 1.0e29)
        cout << " | Next Departure: inf\n";
    else
        cout << " | Next Departure: "
             << time_next_event[2] << endl;
}