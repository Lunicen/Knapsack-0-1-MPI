#include <stdio.h>
#include <stdlib.h>
#include "mpi.h"
//BAJOJAJO BAJOJAJO
// Customize variable type
// WARNING: Remember to only use the types included in default MPI library!
#define VAR_TYPE unsigned long
#define MPI_VAR_TYPE MPI_UNSIGNED_LONG

// Debug options
#define TRACE 0
#define DEBUG 0

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

VAR_TYPE* LoadDataFromFile(char* filename, unsigned long long* amount)
{
    MPI_File file;
    const int code = MPI_File_open(MPI_COMM_WORLD, filename, MPI_MODE_RDONLY, MPI_INFO_NULL, &file);
    if (code != 0) {
        fprintf(stderr, "Couldn't open data file: %s\n", filename);
        return NULL;
    }

    MPI_Offset fileSize;
    MPI_File_get_size(file, &fileSize);

    VAR_TYPE* data = malloc(fileSize);
    if (data == NULL)
    {
	    fprintf(stderr, "Failed to allocate memory for calculating process!\n");
        return NULL;
    }

    *amount = fileSize / sizeof(int);
    MPI_File_read_all(file, data, (int)*amount, MPI_VAR_TYPE, MPI_STATUS_IGNORE);
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

    const double startTime = MPI_Wtime();

    int worldSize, worldRank, nameLen;
	char processorName[MPI_MAX_PROCESSOR_NAME];

    MPI_Comm_size(MPI_COMM_WORLD, &worldSize);
    MPI_Comm_rank(MPI_COMM_WORLD, &worldRank);

    MPI_Get_processor_name(processorName, &nameLen);

    if (CheckForErrors(&worldSize) != 0)
    {
		MPI_Abort(MPI_COMM_WORLD, 1);
    }

#if TRACE || DEBUG
    printf("%s CPU started with rank %d out of %d processors\n",
           processorName, worldRank, worldSize);
#endif

    unsigned long long elementsCount;
    VAR_TYPE* data = LoadDataFromFile(argv[2], &elementsCount);
    if (data == NULL)
    {
	    MPI_Abort(MPI_COMM_WORLD, 2);
    }

    if (worldRank == 0)
    {
        const VAR_TYPE targetSolution = strtol(argv[1], NULL, 10);
        const VAR_TYPE smallestWeight = data[0];

        VAR_TYPE* solutionsCache = malloc((targetSolution + 1) * sizeof(VAR_TYPE));
        VAR_TYPE* currentlyCalculatedValues = malloc((worldSize - 1) * sizeof(VAR_TYPE));

        if (solutionsCache == NULL || currentlyCalculatedValues == NULL)
	    {
		    fprintf(stderr, "Failed to allocate memory for calculating process!\n");
	        MPI_Abort(MPI_COMM_WORLD, 3);
	    }

        // Skip those capacities that cannot fit any item
        VAR_TYPE solutionsCached = 0;
	    for (VAR_TYPE i = 0; i < targetSolution + 1; ++i)
        {
            if (i < smallestWeight)
            {
            	++solutionsCached;
            }

	    	solutionsCache[i] = 0;
        }

        // Distribute tasks
        int process = 1;
        while (process < worldSize)
        {
            VAR_TYPE toCalculate = solutionsCached + (process - 1);
	        if (toCalculate <= targetSolution)
	        {
                currentlyCalculatedValues[process - 1] = toCalculate;
                MPI_Send(&toCalculate, 1, MPI_VAR_TYPE, process, 1, MPI_COMM_WORLD);
#if TRACE
                printf("[%d -> %d] The task %lu sent.\n", worldRank, process, toCalculate);
#endif
	        }
            else
            {
                // Shutdown unused processes
                toCalculate = MPI_SUCCESS;
                MPI_Send(&toCalculate, 1, MPI_VAR_TYPE, process, MPI_SUCCESS, MPI_COMM_WORLD);
            }

            ++process;
        }

        VAR_TYPE request;
        MPI_Status status;

        // Request handling loop
        while (solutionsCached <= targetSolution)
	    {
            MPI_Recv(&request, 1, MPI_VAR_TYPE, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
#if TRACE
            printf("[%d -> %d] Received %lu.\n", status.MPI_SOURCE, worldRank, request);
#endif

            // Requesting cached function
            if (status.MPI_TAG != MPI_SUCCESS)
            {
                if (solutionsCache[request] != 0 || request < smallestWeight)
                {
                    const VAR_TYPE functionValue = solutionsCache[request];

	                MPI_Send(&functionValue, 1, MPI_VAR_TYPE, status.MPI_SOURCE, MPI_SUCCESS, MPI_COMM_WORLD);
#if TRACE
                	printf("[%d -> %d] Function exist! Sent the result of f(%lu) = %lu.\n", worldRank, status.MPI_SOURCE, request, functionValue);
#endif
                }
                else
                {
                    // Postpone process if the requested value isn't calculated yet (-1 equals unknown)
	                MPI_Send(&request, 1, MPI_VAR_TYPE, status.MPI_SOURCE, 1, MPI_COMM_WORLD);
#if TRACE
                    printf("[%d -> %d] Delay the request! The %lu is not calculated yet.\n", worldRank, status.MPI_SOURCE, request);
#endif
                }
            }

            // Finished calculations
            else
            {
                const VAR_TYPE rankWithValue = currentlyCalculatedValues[status.MPI_SOURCE - 1];
	            solutionsCache[rankWithValue] = request;
	            ++solutionsCached;

                const VAR_TYPE nextCalculation = solutionsCached + worldSize - 2;
#if TRACE || DEBUG
                printf("[ root ] f(%lu) = %lu added to cache!\n", rankWithValue, request);
#endif

	            if (nextCalculation <= targetSolution)
                {
                    currentlyCalculatedValues[status.MPI_SOURCE - 1] = nextCalculation;
	                MPI_Send(&nextCalculation, 1, MPI_VAR_TYPE, status.MPI_SOURCE, 1, MPI_COMM_WORLD);
#if TRACE
                    printf("[%d -> %d] The task %lu sent.\n", worldRank, status.MPI_SOURCE, nextCalculation);
#endif
                }
                else
                {
	                // Shutdown the process when the algorithm has the final result
	                const int shutdown = MPI_SUCCESS;
	                MPI_Send(&shutdown, 1, MPI_INT, status.MPI_SOURCE, MPI_SUCCESS, MPI_COMM_WORLD);
                }
            }
	    }

        printf("[ root ] The final result: f(%lu) = %lu\n", targetSolution, solutionsCache[targetSolution]);

        free(currentlyCalculatedValues);
        free(solutionsCache);

        const double endTime = MPI_Wtime();
        printf("Time elapsed: %f s\n", endTime - startTime);
    }
    else
    {
		int exit = 0;
        VAR_TYPE target;
    	MPI_Status status;

        while (!exit)
	    {
        	MPI_Recv(&target, 1, MPI_VAR_TYPE, 0, MPI_ANY_TAG, MPI_COMM_WORLD, &status);

            // If it was a message to finish, then exit
            if (target == MPI_SUCCESS)
            {
#if TRACE || DEBUG
                printf("[%d -> %d] Shutting down...\n", status.MPI_SOURCE, worldRank);
#endif
	            exit = 1;
                continue;
            }

            VAR_TYPE max = 0;
            for (unsigned long long i = 0; i < elementsCount; ++i)
            {
                const VAR_TYPE weight = data[i * 2];
                const VAR_TYPE value = data[i * 2 + 1];
                VAR_TYPE functionValue = target - weight;

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
	                MPI_Send(&functionValue, 1, MPI_VAR_TYPE, 0, 1, MPI_COMM_WORLD);

                    MPI_Recv(&functionValue, 1, MPI_VAR_TYPE, 0, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
                    statusTag = status.MPI_TAG;
                }

                const VAR_TYPE currentValue = functionValue + value;
                max = currentValue > max ? currentValue : max;
            }

            MPI_Send(&max, 1, MPI_VAR_TYPE, 0, MPI_SUCCESS, MPI_COMM_WORLD);
        }
    }

    free(data);

    MPI_Finalize();

    return 0;
}
