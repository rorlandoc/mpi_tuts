#include <mpi.h>

#include <fstream>
#include <string>
#include <string_view>
#include <vector>

void log(std::string_view message, int p_rank)
{
    std::ofstream log_file;
    log_file.open("main.log." + std::to_string(p_rank), std::ios_base::app);
    log_file << message << "\n";
}

int main(int argc, char **argv)
{
    int rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    if (argc != 2)
    {
        log("Usage: main <number>", rank);
        return 1;
    }

    MPI_Init(nullptr, nullptr);

    int num = std::stoi(argv[1]);
    std::vector<int> values(num);

    if (rank == 0)
    {
        for (int i = 0; i < num; i++)
        {
            values[i] = i;
        }
    }
    MPI_Bcast(values.data(), values.size(), MPI_INT, 0, MPI_COMM_WORLD);
    if (rank != 0)
    {
        for (int i = 0; i < num; i++)
        {
            log("Process " + std::to_string(rank) + " received value " + std::to_string(values[i]), rank);
        }
    }

    MPI_Finalize();
    return 0;
}