#include <mpi.h>

#include <fstream>
#include <iostream>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

std::pair<int, int> init()
{
    MPI_Init(nullptr, nullptr);
    std::pair<int, int> info;
    MPI_Comm_rank(MPI_COMM_WORLD, &info.first);
    MPI_Comm_size(MPI_COMM_WORLD, &info.second);
    std::ofstream log_file("main.log." + std::to_string(info.first));
    return info;
}

void finalize()
{
    MPI_Finalize();
}

void log(std::string_view message, int p_rank)
{
    std::ofstream log_file;
    log_file.open("main.log." + std::to_string(p_rank), std::ios_base::app);
    log_file << message << "\n";
}

void abort(std::string_view message, int p_rank)
{
    log(message, p_rank);
    MPI_Abort(MPI_COMM_WORLD, 1);
}

struct Walker
{
    int position;
    int steps_left;
    int id;
    std::string to_string() const
    {
        return "<" + std::to_string(id) + "> {" + std::to_string(position) + ", " + std::to_string(steps_left) + "}";
    }
};

struct WalkerInfo
{
    int count_per_process;
    int max_steps;
};

struct Domain
{
    int start;
    int end;
    int size() const { return end - start + 1; }
};

Domain decompose_domain(Domain global_domain, int p_rank, int p_size)
{
    if (p_size > global_domain.size())
    {
        abort("Error: number of processes is greater than the domain size", p_rank);
    }
    Domain local_domain;
    int local_domain_size = global_domain.size() / p_size;
    local_domain.start = local_domain_size * p_rank;
    if (p_rank == p_size - 1)
    {
        local_domain.end = global_domain.end;
    }
    else
    {
        local_domain.end = local_domain.start + local_domain_size - 1;
    }
    return local_domain;
}

std::vector<Walker> initialize_walkers(const WalkerInfo &walker_info, const Domain &local_domain, int p_rank)
{
    std::vector<Walker> walkers(walker_info.count_per_process);
    int id = 0;
    for (auto &walker : walkers)
    {
        walker.position = local_domain.start;
        walker.steps_left = (rand() / (float)RAND_MAX) * walker_info.max_steps;
        walker.id = p_rank * 1000 + (id++);
    }
    return walkers;
}

void walk(Walker &walker, std::vector<Walker> &outgoing, const Domain &local_domain, const Domain &global_domain, int p_rank)
{
    log("Starting walk for walker " + std::to_string(walker.id) + " (" + std::to_string(walker.steps_left) + " steps)", p_rank);
    while (walker.steps_left > 0)
    {
        if (walker.position == local_domain.end)
        {
            if (walker.position == global_domain.end)
            {
                walker.position = global_domain.start;
            }
            log("Adding walker to outgoing buffer", p_rank);
            outgoing.push_back(walker);
            break;
        }
        else
        {
            walker.steps_left--;
            walker.position++;
        }
        log("Walking: " + walker.to_string(), p_rank);
    }
}

void send_outgoing_walkers(std::vector<Walker> &outgoing, int p_rank, int p_size)
{
    int destination_rank = (p_rank + 1) % p_size;
    log("Sending " + std::to_string(outgoing.size()) + " walkers to process " + std::to_string(destination_rank), p_rank);
    for (const auto &walker : outgoing)
    {
        log("Sending " + walker.to_string(), p_rank);
    }
    MPI_Send((void *)outgoing.data(), outgoing.size() * sizeof(Walker), MPI_BYTE,
             destination_rank, 0, MPI_COMM_WORLD);
    outgoing.clear();
}

void receive_incoming_walkers(std::vector<Walker> &incoming, int p_rank, int p_size)
{
    MPI_Status status;
    int source_rank = (p_rank == 0) ? p_size - 1 : p_rank - 1;
    MPI_Probe(source_rank, 0, MPI_COMM_WORLD, &status);

    int incoming_size;
    MPI_Get_count(&status, MPI_BYTE, &incoming_size);

    log("Receiving " + std::to_string(incoming_size) + " bytes from process " + std::to_string(source_rank), p_rank);

    incoming.resize(incoming_size / sizeof(Walker));
    MPI_Recv((void *)incoming.data(), incoming_size, MPI_BYTE,
             source_rank, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

    for (const auto &walker : incoming)
    {
        log("Received " + walker.to_string(), p_rank);
    }
}

int main(int argc, char *argv[])
{
    auto [p_rank, p_size] = init();

    if (argc != 5)
    {
        abort("Usage: " + std::string{argv[0]} +
                  " <domain_min> <domain_max> <walkers_per_process> <max_walk_size>",
              p_rank);
    }
    if (p_size < 2)
    {
        abort("Error: at least two processes are required", p_rank);
    }

    srand(time(nullptr) * p_rank);
    Domain global_domain{std::stoi(argv[1]), std::stoi(argv[2])};
    WalkerInfo walker_info{std::stoi(argv[3]), std::stoi(argv[4])};

    if (p_rank == 0)
    {
        log("Domain: { " + std::to_string(global_domain.start) + ", " + std::to_string(global_domain.end) + " }", p_rank);
        log("Domain size: " + std::to_string(global_domain.size()), p_rank);
        log("Walkers per process: " + std::to_string(walker_info.count_per_process), p_rank);
        log("Max walk size: " + std::to_string(walker_info.max_steps), p_rank);
        log("Number of processes: " + std::to_string(p_size), p_rank);
    }

    Domain local_domain = decompose_domain(global_domain, p_rank, p_size);
    log("Local domain: { " + std::to_string(local_domain.start) + ", " + std::to_string(local_domain.end) + " } (" + std::to_string(local_domain.size()) + ")", p_rank);

    std::vector<Walker> incoming = initialize_walkers(walker_info, local_domain, p_rank);
    log("Initialized " + std::to_string(incoming.size()) + " walkers", p_rank);
    for (const auto &walker : incoming)
    {
        log("{" + std::to_string(walker.position) + ", " + std::to_string(walker.steps_left) + "}", p_rank);
    }

    int max_sends_recvs = walker_info.max_steps / (local_domain.size() / p_size) + 1;
    for (int m = 0; m < max_sends_recvs; m++)
    {
        std::vector<Walker> outgoing;
        for (auto &walker : incoming)
        {
            walk(walker, outgoing, local_domain, global_domain, p_rank);
        }

        if (p_rank % 2 == 0)
        {
            send_outgoing_walkers(outgoing, p_rank, p_size);
            receive_incoming_walkers(incoming, p_rank, p_size);
        }
        else
        {
            receive_incoming_walkers(incoming, p_rank, p_size);
            send_outgoing_walkers(outgoing, p_rank, p_size);
        }
    }

    finalize();
    return 0;
}