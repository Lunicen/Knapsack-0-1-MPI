# About
The following project was done to provide an optimal way of solving the 0/1 Knapsack Problem. The implementation uses dynamic programming and parallel computations in the form of multiprocessing.

The project is written in C (for the best performance) and uses the Intel implementation of the MPI standard.

## How it works?
### The problem
The 0/1 Knapsack Problem is a well-known _weakly_ NP-hard ([through a reduction from Partition Problem - section 10.5](https://www.cs.cmu.edu/afs/cs/academic/class/15854-f05/www/scribe/lec10.pdf)), while there exists an implementation that has pseudo-polynomial complexity. It stems from the magnitude of input data, not the number of inputs.

### The solution
To solve this problem the recursive approach was implemented using the [SPMD](https://en.wikipedia.org/wiki/Single_program,_multiple_data) approach. At least 2 processes must be called to run the algorithm.

#### Root
The root node (process with rank 0) distributes tasks which consist in determining a particular knapsack size (from 0 to n) and caches the results into a 1-dimensional array.

#### Workers
The other processes wait for a task from the root node. On a given request, each of them computes the result asynchronously. Each node has its own array of items that can be packed (their weights and values).

However, the results of the computed knapsacks are stored in the root node. Therefore, to find out the value of the previously cached result, a query to the root node is sent. If the result is already calculated, the return message contains the answer. Otherwise, the information returned delays the process because that particular result has not yet been computed.

# Features
Besides the main computations, some small things might be useful.

- Error handling - if the user specifies wrong parameters, the code is designed to handle such situations and provides the potential fix.
- Logging modes - the implementation contains two logging modes made for tracking the process of calculations. These are:
  - `TRACE` - prints verbosely every action that has happened (who sent or received the message, its contents and response to them).
  - `DEBUG` - only prints the results of each iteration of the calculated knapsack size.

   *The reason why the debugging can be set only on the preprocessing stage is to avoid significant performance hit that exists by passing through unused ifs.*
- Custom types - the implementation does **not** force the user to use the hardcoded variable types (default is unsigned long). To do that, change the `VAR_TYPE` macro to the desired one, as well as `MPI_VAR_TYPE`.
  
    **WARNING**: Keep in mind that this type has to be defined by MPI standard!

# Installation
The provided tutorial was tested on Windows machines and it's **dedicated** purely to systems working on Intel CPUs. However, detailed steps on installation are on the [Intel MPI Library site in the "Get Started" section](https://www.intel.com/content/www/us/en/developer/tools/oneapi/mpi-library.html#gs.2t9kdv:~:text=Documentation-,Get%20Started,-Windows).

## Minimal requirements
- Systems based on the Intel® 64 architecture
- 2 GB RAM (1 GB of memory per core)
- 4 GB of free hard disk space*
- Visual Studio (2019/2022 version recommended)

*Beware that the required size excludes the highly recommended (and here used) Intel® OneAPI Base Toolkit (13 GB).

## Steps (for Windows)
1. Go to the Intel® oneAPI Base Toolkit download site ([link](https://www.intel.com/content/www/us/en/developer/tools/oneapi/base-toolkit-download.html))
2. Install Intel® OneAPI Base Toolkit tools:
   - Intel® DPC++ Compatibility Tool
   - Intel® Distribution for GDB
   - Intel® oneAPI DPC++ Library
   - Intel® oneAPI Threading Building Blocks
   - Intel® oneAPI DPC++/C++ Compiler
   - Intel® oneAPI Math Kernel Library
   - Intel® Integrated Performance Permitives Cryptography
   - Intel® Advisor
   - Intel® VTune(TM) Profiler
  
    *Disclaimer: It's **highly** recommended to install these tools in the default path. The custom one might lead to issues*

3. Go to the Intel® oneAPI HPC Toolkit download site ([link](https://www.intel.com/content/www/us/en/developer/tools/oneapi/hpc-toolkit-download.html))
4. Install Intel® OneAPI HPC Toolkit tools:
   - Intel® MPI Library
   - Intel® oneAPI DPC++/C++ Compiler & Intel® oneAPI C++ Compiler Classic

    *Disclaimer: The rest of the packages are made mostly for debugging and shorter compilation time. If you wish to install them then go ahead*

5. Open Command Prompt **as administrator** (cmd)
6. Run the `setvars.bat` file located in the MPI installation directory (*by default, C:\Program Files (x86)\Intel\oneAPI*)
7. Type
   ```bash
   hydra-service -install
   ```
8. Type
   ```bash
   hydra-service -start
   ```
9. Type
   ```bash
   mpiexec -register
   ```
10. Provide the details on how to identify your PC in the MPI network
11. Clone this repository by using your favourite client software 😉
12. Open the directory with the cloned project and double click on the `.sln` file

    *Disclaimer: The project has already configured settings. No need to worry about them. If you want to configure your very own project, [here](https://www.intel.com/content/www/us/en/develop/documentation/mpi-developer-guide-windows/top/compiling-and-linking/configuring-a-visual-studio-project.html) is the link to the instruction*

13. Change the Solution Platform to "x64"
14. Click ***Build*** > ***Build Solution*** or press F7
15. You've installed and compiled the project successfully 🎉!

## Launching application

### Locally
1. Click ***Tools*** > ***Command Line*** > ***Developer Command Prompt***
2. Type:
   ```bash
   mpiexec -n <number of processes> <path to the compiled project file> <args>
   ```
    
    For instance:
    ```bash
    mpiexec -n 4 "x64\Release\Knapsack 0-1 MPI.exe" 1000 "examples\KSP_testCase.bin"
    ```

    *Without the -n parameter, mpiexec will automatically determine the amount of the installed core and use all of them*

3. Here is the example result 👀 (*with the TRACE option*):
   

### In a MPI network
**Disclaimer**: Run this application **only** in the trusted networks! This operation requires showing the devices on the LAN, by staying hidden other devices might have problems with establishing the connection.

1. Click ***Tools*** > ***Command Line*** > ***Developer Command Prompt***
2. Type:
   ```bash
   mpiexec -n <number of processes> -ppn <number of processes per node> -hosts <host1>, <host2>, ..., <hostN> <path to the compiled project file> <args>
   ```
    
   For instance:
   ```bash
   mpiexec -n 4 -ppn 4 -hosts DESKTOP-I1VH5RB, DESKTOP-J4A39GC "x64\Release\Knapsack 0-1 MPI.exe" 1000 "examples\KSP_testCase.bin"
   ```

   *Info: Without the -n parameter, mpiexec will automatically determine the amount of the installed core and use all of them*

3. Here is the example result 👀 (*with the TRACE option*):
