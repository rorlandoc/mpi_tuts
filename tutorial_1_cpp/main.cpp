#include <iostream>
#include <string>

#include <mpi.h>

int main(int argc, char *argv[])
{
    MPI::Init(argc, argv);

    int rank = MPI::COMM_WORLD.Get_rank();
    int size = MPI::COMM_WORLD.Get_size();

    int name_len;
    std::string name(MPI_MAX_PROCESSOR_NAME, ' ');
    MPI::Get_processor_name(&name[0], name_len);
    name.resize(name_len);

    std::cout << "[" << name
              << "] - Hello from rank " << rank
              << " out of " << size
              << " processes\n";

    MPI::Finalize();
    return 0;
}