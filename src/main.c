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
    const int argKnapsackCapacity = _Null_ strtol(knapsackCapacity, &pEnd, 10);
    if (argKnapsackCapacity < 1)
    {
	    fprintf(stderr, 
            "Wrong knapsack capacity! Expected positive integer, given: %s, which is interpreted as %d\n",
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

    const int argElementsAmount = _Null_ strtol(elementsAmount, &pEnd, 10);
    if (argElementsAmount < 1)
    {
	    fprintf(stderr, 
            "Wrong available elements amount! Expected positive integer, given: %s, which is interpreted as %d\n",
            elementsAmount,
            argElementsAmount
        );
		return 1;
    }
    
    return 0;
}

int CheckForErrors(const int* worldSize)
{
    const int minWorldSize = 2;

	if (*worldSize < minWorldSize)
    {
		fprintf(stderr, "World size must be no less than %d, given %d\n", minWorldSize, *worldSize);
        return 1;
	}

    return 0;
}

int* LoadDataFromFile(char* filename, const int amount)
{
    MPI_File file;
    const int code = MPI_File_open(MPI_COMM_WORLD, filename, MPI_MODE_RDONLY, MPI_INFO_NULL, &file);
    if (code != 0) {
        fprintf(stderr, "Couldn't open data file: %s\n", filename);
        return NULL;
    }

	int* data = malloc(amount * sizeof(int));
    if (data == NULL)
    {
	    fprintf(stderr, "Failed to allocate memory for calculating process!\n");
        return NULL;
    }

    MPI_File_read_all(file, data, amount, MPI_UNSIGNED_LONG, MPI_STATUS_IGNORE);
    MPI_File_close(&file);

    return data;
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


    if (CheckForErrors(&worldSize) != 0)
    {
		MPI_Abort(MPI_COMM_WORLD, 1);
    }

    printf("%s CPU started with rank %d out of %d processors\n",
           processorName, worldRank, worldSize);

    const int elementsCount = strtol(argv[3], NULL, 10);
    int* data = LoadDataFromFile(argv[2], elementsCount);
    if (data == NULL)
    {
	    MPI_Abort(MPI_COMM_WORLD, 2);
    }

    int* solutionsCache = NULL;
    if (worldRank == 0)
    {
	    solutionsCache = malloc(strtol(argv[3], NULL, 10) * sizeof(int));
	    if (solutionsCache == NULL)
	    {
		    fprintf(stderr, "Failed to allocate memory for calculating process!\n");
	        MPI_Abort(MPI_COMM_WORLD, 3);
	    }

        // When there is no space in knapsack, it cannot fit anything...
        solutionsCache[0] = 0;
    }

    int solutionsCalculated = 0;
    const int targetSolution = strtol(argv[1], NULL, 10);

    if (worldRank == 0)
    {
        int idleProcesses = worldRank - 1;

        // Distribute tasks
        while (idleProcesses > 1)
        {
            const int toCalculate = solutionsCalculated + 1;
            MPI_Send(&toCalculate, 1, MPI_INT, idleProcesses, MPI_ANY_TAG, MPI_COMM_WORLD);

            --idleProcesses;
        }

        int result;
        MPI_Status status;

	    while (solutionsCalculated < targetSolution)
	    {
            MPI_Recv(&result, 1, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);

            // Requesting cached function
            if (status.MPI_TAG < solutionsCalculated)
            {
	            MPI_Send(&solutionsCache[status.MPI_TAG], 1, MPI_INT, idleProcesses, MPI_ANY_TAG, MPI_COMM_WORLD);
            }

            // Finished calculations
            else
            {
	            solutionsCache[status.MPI_TAG] = result;
                ++solutionsCalculated;

                if (solutionsCalculated < targetSolution)
                {
                    const int toCalculate = solutionsCalculated + 1;
	                MPI_Send(&toCalculate, 1, MPI_INT, idleProcesses, MPI_ANY_TAG, MPI_COMM_WORLD);
                }
            }
	    }
    }
    else
    {
	    while (solutionsCalculated < targetSolution)
	    {
		    
	    }
    }

    free(data);
    if (solutionsCache != NULL)
    {
	    free(solutionsCache);
    }

    // Finalize the MPI environment.
    MPI_Finalize();

    return 0;
}