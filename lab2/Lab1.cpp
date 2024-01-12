п»ї#include <iostream>

extern void run_mpi_cartesian(int* argc, char*** argv);

int main(int argc, char **argv)
{
    run_mpi_cartesian(&argc, &argv);
    return 0;
}
