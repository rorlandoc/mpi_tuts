#include <mpi.h>

#include <fstream>
#include <string>
#include <string_view>

void log(std::string_view message, int p_rank)
{
    std::ofstream log_file;
    log_file.open("main.log." + std::to_string(p_rank), std::ios_base::app);
    log_file << message << "\n";
}

void my_bcast(void *data, int count, MPI_Datatype datatype, int root, MPI_Comm communicator)
{
    int rank;
    int size;
    MPI_Comm_rank(communicator, &rank);
    MPI_Comm_size(communicator, &size);

    if (rank == root)
    {
        for (int i = 0; i < size; i++)
        {
            if (i != rank)
            {
                MPI_Send(data, count, datatype, i, 0, communicator);
            }
        }
    }
    else
    {
        MPI_Recv(data, count, datatype, root, 0, communicator, MPI_STATUS_IGNORE);
    }
}

int main(int argc, char **argv)
{
    MPI_Init(&argc, &argv);

    int rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    int value;
    if (rank == 0)
    {
        value = 100;
        log("Process 0 broadcasting value " + std::to_string(value), rank);
        my_bcast(&value, 1, MPI_INT, 0, MPI_COMM_WORLD);
    }
    else
    {
        my_bcast(&value, 1, MPI_INT, 0, MPI_COMM_WORLD);
        log("Process " + std::to_string(rank) + " received value " + std::to_string(value), rank);
    }

    MPI_Finalize();
    return 0;
}