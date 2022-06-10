// ReSharper disable CppPointerToIntegralConversion
#include <stdio.h>
#include <stdlib.h>
#include "mpi.h"


int CheckArgumentCorrectness(char* knapsackCapacity, char* filename)
{
	if (knapsackCapacity == NULL)
    {
	    fprintf(stderr, "No arguments given! Expected: <knapsack capacity> <data filename>\n");
		return 1;
    }

    char* pEnd;
    const int argKnapsackCapacity = _Null_ strtol(knapsackCapacity, &pEnd, 10);
    if (argKnapsackCapacity < 0)
    {
	    fprintf(stderr, 
            "Wrong knapsack capacity! Expected nonnegative integer, given: %s, which is interpreted as %d\n",
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

int* LoadDataFromFile(char* filename, unsigned long long* amount)
{
    MPI_File file;
    const int code = MPI_File_open(MPI_COMM_WORLD, filename, MPI_MODE_RDONLY, MPI_INFO_NULL, &file);
    if (code != 0) {
        fprintf(stderr, "Couldn't open data file: %s\n", filename);
        return NULL;
    }

    MPI_Offset fileSize;
    MPI_File_get_size(file, &fileSize);

    int* data = malloc(fileSize);
    if (data == NULL)
    {
	    fprintf(stderr, "Failed to allocate memory for calculating process!\n");
        return NULL;
    }

    *amount = fileSize / sizeof(int);
    MPI_File_read_all(file, data, (int)*amount, MPI_UNSIGNED_LONG, MPI_STATUS_IGNORE);
    MPI_File_close(&file);

    // The list contains each item weight & value.
    // Divide by 2 to have the amount of items.
    *amount /= 2;

    return data;
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

    if (CheckForErrors(&worldSize) != 0)
    {
		MPI_Abort(MPI_COMM_WORLD, 1);
    }

    printf("%s CPU started with rank %d out of %d processors\n",
           processorName, worldRank, worldSize);


    unsigned long long elementsCount;
    int* data = LoadDataFromFile(argv[2], &elementsCount);
    if (data == NULL)
    {
	    MPI_Abort(MPI_COMM_WORLD, 2);
    }
    
    int* solutionsCache = NULL;
    const int targetSolution = strtol(argv[1], NULL, 10);

    if (worldRank == 0)
    {
	    solutionsCache = malloc((targetSolution + 1) * sizeof(int));

	    if (solutionsCache == NULL)
	    {
		    fprintf(stderr, "Failed to allocate memory for calculating process!\n");
	        MPI_Abort(MPI_COMM_WORLD, 3);
	    }

	    // Mark those capacities that cannot fit any item and unknown values
	    int solutionsCached = 0;
	    for (int i = 0; i < targetSolution + 1; ++i)
        {
            if (i < data[0])
            {
            	solutionsCache[i] = 0;
                ++solutionsCached;
            }
            else
            {
	            solutionsCache[i] = -1;
            }
        }

        // Distribute tasks
        int process = 1;
        while (process < worldSize)
        {
            int toCalculate = solutionsCached + (process - 1);
	        if (toCalculate <= targetSolution)
	        {
                MPI_Send(&toCalculate, 1, MPI_INT, process, 1, MPI_COMM_WORLD);
                printf("[%d -> %d] The task %d sent.\n", worldRank, process, toCalculate);
	        }
            else
            {
                // Shutdown unused processes
                toCalculate = MPI_SUCCESS;
                MPI_Send(&toCalculate, 1, MPI_INT, process, MPI_SUCCESS, MPI_COMM_WORLD);
            }

            ++process;
        }

        int request;
        MPI_Status status;

        // Request handling loop
        while (solutionsCached <= targetSolution)
	    {
            MPI_Recv(&request, 1, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
            printf("[%d -> %d] Received %d.\n", status.MPI_SOURCE, worldRank, request);

            // Requesting cached function
            if (status.MPI_TAG == 0)
            {
                if (solutionsCache[request] != -1)
                {
                    const int functionValue = solutionsCache[request];

	                MPI_Send(&functionValue, 1, MPI_INT, status.MPI_SOURCE, MPI_SUCCESS, MPI_COMM_WORLD);
                    printf("[%d -> %d] Function exist! Sent the result of f(%d) = %d.\n", worldRank, status.MPI_SOURCE, request, functionValue);
                }
                else
                {
                    // Postpone process if the requested value isn't calculated yet (-1 equals unknown)
	                MPI_Send(&request, 1, MPI_INT, status.MPI_SOURCE, 1, MPI_COMM_WORLD);
                    printf("[%d -> %d] Delay the request! The %d is not calculated yet.\n", worldRank, status.MPI_SOURCE, request);
                }
            }

            // Finished calculations
            else
            {
	            solutionsCache[status.MPI_TAG] = request;
	            ++solutionsCached;

                const int nextCalculation = solutionsCached + worldSize - 2;
                printf("[ root ] f(%d) = %d added to cache!\n", status.MPI_TAG, request);

	            if (nextCalculation <= targetSolution)
                {
	                MPI_Send(&nextCalculation, 1, MPI_INT, status.MPI_SOURCE, 1, MPI_COMM_WORLD);
                    printf("[%d -> %d] The task %d sent.\n", worldRank, status.MPI_SOURCE, nextCalculation);
                }
                else
                {
	                // Shutdown the process when the algorithm has the final result
	                const int shutdown = MPI_SUCCESS;
	                MPI_Send(&shutdown, 1, MPI_INT, status.MPI_SOURCE, MPI_SUCCESS, MPI_COMM_WORLD);
                }
            }
	    }

        printf("[ root ] The final result: f(%d) = %d!\n", targetSolution, solutionsCache[targetSolution]);
    }
    else
    {
        int exit = 0, target;
    	MPI_Status status;

        while (!exit)
	    {
        	MPI_Recv(&target, 1, MPI_INT, 0, MPI_ANY_TAG, MPI_COMM_WORLD, &status);

            // If it was a message to finish, then exit
            if (target == MPI_SUCCESS)
            {
                printf("[%d -> %d] Shutting down...\n", status.MPI_SOURCE, worldRank);
	            exit = 1;
                continue;
            }

            int max = 0;
            for (unsigned long long i = 0; i < elementsCount; ++i)
            {
                const int weight = data[i * 2];  // NOLINT(bugprone-implicit-widening-of-multiplication-result)
                const int value = data[i * 2 + 1];
                int functionValue = target - weight;

                // If the next weights are exceeding the knapsack capacity then skip
                if (weight > target)
                {
                    break;
                }

                // Keep requesting the value until it's sent (tag 1 = value is not calculated yet)
	            int statusTag = 1;
                while (statusTag != MPI_SUCCESS)
                {
                    // f(w - w_i)
	                MPI_Send(&functionValue, 1, MPI_INT, 0, 0, MPI_COMM_WORLD);

                    MPI_Recv(&functionValue, 1, MPI_INT, 0, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
                    statusTag = status.MPI_TAG;
                }

                const int currentValue = functionValue + value;
                max = (currentValue > max) ? currentValue : max;
            }
            MPI_Send(&max, 1, MPI_INT, 0, target, MPI_COMM_WORLD);
        }
    }

    free(data);
    if (solutionsCache != NULL)
    {
	    free(solutionsCache);
    }

    MPI_Finalize();

    return 0;
}