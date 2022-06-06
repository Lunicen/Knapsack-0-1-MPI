// ReSharper disable CppPointerToIntegralConversion
#include <stdio.h>
#include <stdlib.h>
#include "mpi.h"

int CheckArgumentCorrectness(char* knapsackCapacity, char* filename, char* elementsAmount)
{
	if (knapsackCapacity == NULL)
    {
	    fprintf(stderr, "No arguments given!\n");
		return 1;
    }

    char* pEnd;
    const long argKnapsackCapacity = _Null_ strtol(knapsackCapacity, &pEnd, 10);
    if (argKnapsackCapacity < 1)
    {
	    fprintf(stderr, 
            "Wrong knapsack capacity! Expected positive integer, given: %s, which is interpreted as %ld\n",
            knapsackCapacity,
            argKnapsackCapacity
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

    const long argElementsAmount = _Null_ strtol(elementsAmount, &pEnd, 10);
    if (argElementsAmount < 1)
    {
	    fprintf(stderr, 
            "Wrong available elements amount! Expected positive integer, given: %s, which is interpreted as %ld\n",
            elementsAmount,
            argElementsAmount
        );
		return 1;
    }
    
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

int TryToSolve(unsigned long* cache, const unsigned long* capacity)
{
    // On capacity equal 0 we cannot pack anything - obvious scenario
	if (capacity == 0)
	{
		return 0;
	}


}

int main(int argc, char** argv) {

    if (CheckArgumentCorrectness(argv[1], argv[2], argv[3]) != 0)
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
    printf("Process initialized successfully on %s with rank %d out of %d processors\n",
           processorName, worldRank, worldSize);

    char* pEnd;
    unsigned long* solutionsCache = malloc(strtol(argv[3], &pEnd, 10) * sizeof(unsigned long));
    if (solutionsCache == NULL)
    {
	    fprintf(stderr, "Failed to allocate memory for calculating process!\n");
        MPI_Abort(MPI_COMM_WORLD, 2);
    }

    MPI_File data;
    const int fileCode = MPI_File_open(MPI_COMM_WORLD, argv[2], MPI_MODE_RDONLY, MPI_INFO_NULL, &data);
    if (fileCode) {
        fprintf(stderr, "Couldn't open data file: %s\n", argv[2]);
        MPI_Abort(MPI_COMM_WORLD, 3);
    }

    const unsigned long dataElements = strtol(argv[3], &pEnd, 10) * 2;
    unsigned long* dataValues = malloc(dataElements * sizeof(unsigned long));
    if (dataValues == NULL)
    {
	    fprintf(stderr, "Failed to allocate memory for calculating process!\n");
        MPI_Abort(MPI_COMM_WORLD, 2);
    }

    MPI_File_read_all(data, dataValues, (int)dataElements, MPI_UNSIGNED_LONG, MPI_STATUS_IGNORE);
    

	

    free(dataValues);
    free(solutionsCache);

    // Finalize the MPI environment.
    MPI_Finalize();

    return 0;
}