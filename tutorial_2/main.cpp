#include <iostream>

#include <mpi.h>

int main(int argc, char *argv[])
{
    MPI_Init(nullptr, nullptr);

    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    int number;
    if (rank == 0)
    {
        printf("Running on %d processes\n", size);

        number = 420;
        MPI_Send(&number, 1, MPI_INT, 1, 0, MPI_COMM_WORLD);
    }
    else if (rank == 1)
    {
        MPI_Recv(&number, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        printf("Process %d received number %d from process 0\n", rank, number);
    }
    else
    {
        printf("Process %d did not receive any number\n", rank);
    }

    MPI_Finalize();
    return 0;
}