/***************************************************************************************************
 * Student name: Tomer Gill
 * Student: 318459450
 * Course Exercise Group: 01 (CS student, actual group is 89231-03)
 * Exercise name: Exercise 1
***************************************************************************************************/

#include <malloc.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <fcntl.h>

#define LINE_SIZE 160 //defines the max size for each line in the configuration file.

int readLineFromFile(int fd, char line[LINE_SIZE + 1]);

int main(int argc, char *argv[])
{
    char folderLocation[LINE_SIZE + 1], inputLocation[LINE_SIZE + 1], outputLocation[LINE_SIZE + 1];
    int fd, i;

    if (argc < 2)
    {
        perror("Bad Usage");
        return 1;
    }

    /*
     * Open the file.
     */
    printf("Opening config file\n");
    if ((fd = open(argv[1], O_RDONLY)) == -1)
    {
        char *err;
        if (errno == ENOENT)
            err = "Error : First file is NULL pointer ";
        else if (errno == EACCES)
            err = "Error : 'open' was unable to open the first file ";
        else
            err = "Error : unknown error opening the first file ";
        perror(err);
        return 3;
    }

    /*
     * Reading the 3 lines from the config file.
     */
    readLineFromFile(fd, folderLocation);
    readLineFromFile(fd, inputLocation);
    readLineFromFile(fd, outputLocation);


}


int readLineFromFile(int fd, char line[LINE_SIZE + 1])
{
    char c;
    int i = 0;

    for (; i < LINE_SIZE; ++i)
    {
        int status = read(fd, &c, sizeof(char));
        if (status == 0)
        {
            line[i] = '\0';
            return i + 1;
        }
        else if (status == -1)
        {
            if (errno == EBADF)
                perror("Error : config file is not a valid file descriptor or is not open for "
                               "reading.\n");
            else if (errno == EFAULT)
                perror("Error : char c is outside your accessible address space.\n");
            else if (errno == EIO)
                perror("Error : I/O error while reading the config file.\n");
            else if (errno == EISDIR)
                perror("Error : config file descriptor points to a directory.\n");
            else
                perror("Error : unknown error while reading config file.\n");
            exit(3);
        }
        if (c == '\n')
        {
            line[i] = '\0';
            return i + 1;
        }
        line[i] = c;
    }
    return i + 1;
}