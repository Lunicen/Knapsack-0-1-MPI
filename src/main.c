#include <stdio.h>
#include <stdlib.h>
#include "mpi.h"

int CheckArgumentCorrectness(char* knapsackCapacity, char* filename)
{
	if (knapsackCapacity == NULL)
    {
	    fprintf(stderr, "No arguments given!\n");
		return 1;
    }

    char* pEnd;
    const long argValue = _Null_ strtol(knapsackCapacity, &pEnd, 10);
    if (argValue == 0)
    {
	    fprintf(stderr, 
            "Wrong knapsack capacity! Expected positive integer, given: %s, which is interpreted as %ld\n",
            knapsackCapacity,
            argValue
        );
		return 1;
    }

    FILE* file;
    if (fopen_s(&file, filename, "r") != 0)
    {
	    fprintf(stderr, 
            "The specified %s does not exist!\n",
            filename
        );
		return 1;
    }
    fclose(file);
    
    return 0;
}

void CheckForErrors(const int* worldSize)
{
    const int minWorldSize = 2;

	if (*worldSize < minWorldSize)
    {
		fprintf(stderr, "World size must be no less than %d, given %d\n", minWorldSize, *worldSize);
		MPI_Abort(MPI_COMM_WORLD, 1);
	}
}

int main(int argc, char** argv) {

    if (CheckArgumentCorrectness(argv[1], argv[2]) != 0)
    {
	    return 1;
    }

    // Initialize the MPI environment
    MPI_Init(NULL, NULL);

    int worldSize, worldRank, nameLen;
	char processorName[MPI_MAX_PROCESSOR_NAME];

    MPI_Comm_size(MPI_COMM_WORLD, &worldSize);
    MPI_Comm_rank(MPI_COMM_WORLD, &worldRank);

    MPI_Get_processor_name(processorName, &nameLen);

    CheckForErrors(&worldSize);

    // Print off a hello world message
    printf("Hello world from processor %s, rank %d out of %d processors\n",
           processorName, worldRank, worldSize);

    // Finalize the MPI environment.
    MPI_Finalize();

    return 0;
}