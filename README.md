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
   - Intel® oneAPI Data Analitics Library
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

5. Add bin directories to the PATH system variable:
   - Path to MPI (*by default, `C:\Program Files (x86)\Intel\oneAPI\mpi\<version>\bin`*)
   - Path to compiler (*by default, `C:\Program Files (x86)\Intel\oneAPI\compiler\<version>\windows\bin\intel64`*)
6. Open Command Prompt **as administrator** (cmd)
7. Run the `setvars.bat` file located in the MPI installation directory (*by default, `C:\Program Files (x86)\Intel\oneAPI`*)
8. Install & run hydra service (used for network MPI communication)
    ```bash
    hydra-service -install
    hydra-service -start
    ```
9.  Register your device to identify it in the MPI network 
    ```bash
    mpiexec -register
    ```
10. (*Optional step*) It's highly recommended to restart the PC
11. Clone this repository by using your favourite client software 😉
12. Open the directory with the cloned project and double click on the `.sln` file

    *Disclaimer: The project has already configured settings. No need to worry about them. If you want to configure your very own project, [here](https://www.intel.com/content/www/us/en/develop/documentation/mpi-developer-guide-windows/top/compiling-and-linking/configuring-a-visual-studio-project.html) is the link to the instruction*

13. Click ***Project*** > ***Intel Compiler*** > ***Use Intel oneAPI DPC++/C++ Compiler*** if it's not already in use
14. Click ***Project*** > ***(...) Properties***
15. (*Optional step*) Ensure that everything in the configuration is set properly ([according to the documentation](https://www.intel.com/content/www/us/en/develop/documentation/mpi-developer-guide-windows/top/compiling-and-linking/configuring-a-visual-studio-project.html))
16. Save and close the settings
17. Change the Solution Platform to "x64"
18. Click ***Build*** > ***Build Solution*** or press F7
19. You've installed and compiled the project successfully 🎉!

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

3. Here is the example result 👀 (*with the DEBUG option*):

    ![Local MPI result](./misc/local-communication.gif)
   

### In a MPI network
**Disclaimer**: Intel MPI takes care of Your safety! The communication between hosts is encrypted and machines can only connect to each other through estabilished, secure connection. 

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

## Trouble?
*Typical, annoying with no explanations MPI error...*
![Example error](./misc/error.png)

If you've encountered some errors, try to run the application using *Visual Studio Debugger*. It might alert you about some missing files or bad configuration.

### Missing DLL files
`The code execution cannot proceed because impi.dll was not found.`

The message implies that the executable (not Visual Studio!) was unable to locate the missing DLL. It is a [well-known behaviour](https://stackoverflow.com/a/4953976) when it comes to the DLL files and the workaround is provided below.

There's a probability that it happens when the Intel® oneAPI toolkit and the project are installed on separated drives.

#### Solution
Copy the missing DLL files from the MPI directory.
1. In Visual Studio click on ***Tools*** > ***Command Line*** > ***Developer Command Prompt***
2. Copy these files by typing:
   
   ```bash
   copy "%I_MPI_ONEAPI_ROOT%\bin\debug\impi.dll" x64\Debug
   copy "%I_MPI_ONEAPI_ROOT%\bin\release\impi.dll" x64\Release
   ```

   *Disclaimer: Make sure these directories were already created. Otherwise, compile the project in both Debug and Release modes.*

3. The problem should be fixed now 👷‍♂️

### It just doesn't work...
If you think that something is messed up on the Intel side, rather than with the configuration then simply open one of the Intel® OneAPI Toolkit panels (one of the downloaded toolkits) and reinstall or repair the tools.

After that do the steps detailed in the **Installation** section.

### It still doesn't work 😭
Okay, okay, calm down. There is one final solution that always works. Compiling and running code using OneAPI tools instead of Visual Studio.

1. Open Command Prompt **as administrator** (cmd)
2. Initialize the environment by running the `setvars.bat` script
   
   ```bash
   cd %I_MPI_ONEAPI_ROOT%
   ./setvars.bat
   ```
3. In **the same** (*it's very important*) cmd session go to the project folder
4. Go to the `src` directory (the one containing the `main.c` file)
5. Compile the project by typing
   ```bash
   mpiicc -o Knapsack.exe main.c
   ```
6. If you're getting this kind of result:
   
   ![Successful compilation](misc/successful_compilation.png)
   Then congratulations, you've compiled and executed the project successfully 🎉!
   *I know it's a poor workaround but hey! It works...*