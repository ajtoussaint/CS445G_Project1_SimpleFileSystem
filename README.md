# Usage
The file system program was developed in 64bit ubuntu desktop. An ISO image can be downloaded from [https://ubuntu.com/download/desktop]. gcc compiler version 11.4.0 was used to compile. The pthreads library is required for use.

To execute the file system demonstration follow the following procedure:
1. Download the "project1.c" file
2. Open a new terminal and navigate to the location of the "project1.c" file
3. Run the following terminal command:
```
gcc project1.c -o filesystem -lpthread && ./filesystem
```
4. Observe the print statements in the terminal to monitor the progress of the demonstration

# Features Implemented
The main function of the file system is to perform the demonstration of modifying two files within three separate threads. This was achieved by implementing the following:

## High Level Features
- A simulated disk of 1MB
- A file control block strucutre that includes file size and location
- A global file table that monitors a file's name, control block, and total number of open instances
- A local file table structure that tracks a file's name and index within the global table
- A directory that stores a file's name, extension, size, and location as a linked list
- A PrintDir function to show the directory contents
- An Open function that updates global and per-process file tables
- A Create function which reserves space in memory and adds a file to the directory, then runs the Open function
- A Close function that updates global and per-process file tables
- A Read function that transfers data from a file in the simulated disk to a local variable
- A Write function that transfers data from a local variable to the simulated disk
- A Delete function that deallocates file space and removes the file from the directory
- Mutex locks for the global file table and simulated disk to allow for concurrent execution

## Implementation details
- Find/Add/Remove functions for the global file table
- Find/Add/Remove functions for the local file table
- Functions to read and write short and unsigend integers from location in the simulated disk
- A bitmap to track free space
- Functions to read and write bits from locations in the simulated disk associated with the bitmap
- A function to find the first available free space of size n based on the bitmap
- Find/Add/Remove functions for the directory
- Data structures to store information about the files being used in the demonstration

# Features Not Implemented
- Memory is allocated contiguously but there is no mechanism to deal with external fragmentation. A user could encounter a situtation where there was sufficient memory avaialble to store a file but, due to the organization of existing files, a contiguous slot could not be found. The user would need to delete some file to make room. This could be improved by applying compaction or some other algorithm when a new file is created.

- A file pointer was not implemented. Read and Write functions use a buffer that overwrites the entire file each time a write is performed and outputs the entire file when a read is performed.
