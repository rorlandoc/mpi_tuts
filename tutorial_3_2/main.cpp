#include <iostream>
#include <vector>

#include <mpi.h>

int main(int argc, char *argv[])
{
    MPI_Init(nullptr, nullptr);

    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    std::vector<int> packet;
    int max_packet_size = 100;
    int packet_size;
    if (rank == 0)
    {
        srand(time(nullptr));
        packet_size = (rand() / (float)RAND_MAX) * max_packet_size;
        for (int i = 0; i < packet_size; i++)
        {
            packet.push_back(rand());
        }

        MPI_Send(packet.data(), packet_size, MPI_INT, 1, 0, MPI_COMM_WORLD);

        std::cout << "Process 0 sent " << packet_size << " numbers" << std::endl;
        std::cout << "[" << rank << "] - ";
        std::for_each(packet.begin(), packet.end(), [](int &n)
                      { std::cout << n << " "; });
        std::cout << "\n";
    }
    else if (rank == 1)
    {
        MPI_Status status;
        MPI_Probe(0, 0, MPI_COMM_WORLD, &status);
        MPI_Get_count(&status, MPI_INT, &packet_size);

        packet.resize(packet_size);
        MPI_Recv(packet.data(), packet_size, MPI_INT, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

        std::cout << "Process 1 received " << packet_size << " numbers" << "\n";
        std::cout << "[" << rank << "] - Message source: " << status.MPI_SOURCE
                  << ", tag: " << status.MPI_TAG << "\n";
        std::cout << "[" << rank << "] - ";
        std::for_each(packet.begin(), packet.end(), [](int &n)
                      { std::cout << n << " "; });
        std::cout << "\n";
    }
    else
    {
        printf("Process %d did not receive any number\n", rank);
    }

    MPI_Finalize();
    return 0;
}