/*****************************
 * Student name: Tomer Gill
 * Student: 318459450
 * Course Exercise Group: 01
 * Exercise name: Exercise 1
******************************/

#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>

#define UPPER_TO_LOWER_DIFF ('a' - 'A')

typedef enum {FALSE = 0, TRUE = 1} BOOL;

inline int readCharFromFile1(int file1, char *char1, BOOL *finished1);
inline int readCharFromFile2(int file2, char *char2, BOOL *finished2);

int main(int argc, char *argv[])
{
    int file1, file2;
    char char1, char2;
    BOOL finished1 = FALSE, finished2 = FALSE;
    BOOL isSimiliar = FALSE; //false means they are equal.


    if (argc < 3)
    {
        char err[] = "Usage Error\n";
        //write()
    }

    if ((file1 = open(argv[1], O_RDONLY) == -1))
    {
        char *err;
        if (errno == ENONET)
            err = "Error : First file is NULL pointer\n";
        else if (errno == EACCES)
            err = "Error : 'open' was unable to open the first file.\n";
        else
            err = "Error : unknown error opening the first file.";
        write(2, err, sizeof(err));
        return 3;
    }

    if ((file2 = open(argv[2], O_RDONLY) == -1))
    {
        char *err;
        if (errno == ENONET)
            err = "Error : Second file is NULL pointer\n";
        else if (errno == EACCES)
            err = "Error : 'open' was unable to open the second file.\n";
        else
            err = "Error : unknown error opening the second file.";
        write(2, err, sizeof(err));
        return 3;
    }

    readCharFromFile1(file1, &char1, &finished1);
    readCharFromFile1(file2, &char2, &finished2);

    while (!finished1 && !finished2)
    {
        if (toupper(char1) == toupper(char2))//actual characters are equal
        {
            if (abs(char1 - char2) == UPPER_TO_LOWER_DIFF)
                isSimiliar = TRUE;

        }
    }
}

inline int readCharFromFile1(int file1, char *char1, BOOL *finished1)
{
    int status = read(file1, &char1, sizeof(char1));
    if (status == 0)
        *finished1 = TRUE;
    else if (status < 0)
    {
        if (errno == EBADF)
            perror("Error : first file is not a valid file descriptor or is not open for reading.\n");
        else if (errno == EFAULT)
            perror("Error : char1 is outside your accessible address space.\n");
        else if (errno == EIO)
            perror("Error : I/O error while reading the first file.\n");
        else if (errno == EISDIR)
            perror("Error : first file descriptor points to a directory.\n");
        else
            perror("Error : unknown error while reading first file.\n");
        exit(3);
    }
}

inline int readCharFromFile2(int file2, char *char2, BOOL *finished2)
{
    int status = read(file2, &char2, sizeof(char2));
    if (status == 0)
        *finished2 = TRUE;
    else if (status < 0)
    {
        if (errno == EBADF)
            perror("Error : second file is not a valid file descriptor or is not open for reading.\n");
        else if (errno == EFAULT)
            perror("Error : char2 is outside your accessible address space.\n");
        else if (errno == EIO)
            perror("Error : I/O error while reading the second file.\n");
        else if (errno == EISDIR)
            perror("Error : second file descriptor points to a directory.\n");
        else
            perror("Error : unknown error while reading second file.\n");
        return 3;
    }
}
