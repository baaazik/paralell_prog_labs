п»ї#include "mpi_block.h"
#include "mpi_nonblock.h"
#include "mpi_collective.h"

int main(int argc, char** argv)
{
    run_mpi_block(&argc, &argv);
    //run_mpi_nonblock(&argc, &argv);
}
