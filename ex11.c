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
#define IS_SPACING_CHAR(c) ((c) == ' ' || (c) == '\n')

typedef enum {FALSE = 0, TRUE = 1} BOOL;

/**********************************************************
 * function name: readCharFromFile1
 * The Input: The file descriptor of file1
 * The output: 01
 * The Function operation: Exercise 1
***********************************************************/
inline void readCharFromFile1(int file1, char *char1, BOOL *finished1);
inline void readCharFromFile2(int file2, char *char2, BOOL *finished2);

#define READ_1 readCharFromFile1(file1, &char1, &finished1)
#define READ_2 readCharFromFile2(file2, &char2, &finished2)

int main(int argc, char *argv[])
{
    int file1, file2;
    char char1, char2;
    BOOL finished1 = FALSE, finished2 = FALSE;
    BOOL isSimilar = FALSE; //false means they are equal.


    if (argc < 3)
    {
        perror("Usage Error");
        return -1;
    }

    printf("Opening file1\n");
    if ((file1 = open(argv[1], O_RDONLY) == -1))
    {
        char *err;
        if (errno == ENONET)
            err = "Error : First file is NULL pointer";
        else if (errno == EACCES)
            err = "Error : 'open' was unable to open the first file.";
        else
            err = "Error : unknown error opening the first file.";
        perror(err);
        return 3;
    }

    printf("Opening file2\n");
    if ((file2 = open(argv[2], O_RDONLY) == -1))
    {
        char *err;
        if (errno == ENONET)
            err = "Error : Second file is NULL pointer";
        else if (errno == EACCES)
            err = "Error : 'open' was unable to open the second file.";
        else
            err = "Error : unknown error opening the second file.";
        perror(err);
        return 3;
    }

    printf("first while\n");
    /*
     * While there are still characters to read in both files, compare them.
     * If they are equal read the next ones.
     */
    READ_1;
    READ_2;
    while (!finished1 && !finished2)
    {
        if (toupper(char1) == toupper(char2))//actual characters are equal
        {
            if (abs(char1 - char2) == UPPER_TO_LOWER_DIFF) //difference is upper/lower letter
                isSimilar = TRUE;
            READ_1;
            READ_2;
            continue;
        }
        //characters are not equal
        if (IS_SPACING_CHAR(char1)) //if char1 is ' ' or '\n' count as similar and continue
        {
            isSimilar = TRUE;
            READ_1;
            continue;
        }
        if (IS_SPACING_CHAR(char2)) //likewise for char2
        {
            isSimilar = TRUE;
            READ_2;
        }
    } //end of while

    printf("second while\n");
    /*
     * while there are still left chars to read in file1, read.
     * If all of them are ' ' or \n, the files are similar.
     */
    while (!finished1)
    {
        if (IS_SPACING_CHAR(char1))
            READ_1;
        else
            return 3; //different
    }

    printf("third while\n");
    /*
     * while there are still left chars to read in file1, read.
     * If all of them are ' ' or \n, the files are similar.
     */
    while (!finished2)
    {
        if (IS_SPACING_CHAR(char2))
            READ_2;
        else
            return 3; //different
    }

    /*
     * If the files are equal, isSimilar will be FALSE = 0, so 1 will be returned.
     * Other wise (they are similar) isSimilar will be TRUE = 1, so 2 will be returned.
     * (It's clear to see that if the files are different, the program will exit before,
     * returning 3 as required.)
     */
    return 1 + isSimilar;
}

inline void readCharFromFile1(int file1, char *char1, BOOL *finished1)
{
    printf("readCharFromFile1\n");
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

inline void readCharFromFile2(int file2, char *char2, BOOL *finished2)
{
    printf("readCharFromFile2\n");
    int status = read(file2, &char2, sizeof(char2));
    if (status == 0)
        *finished2 = TRUE;
    else if (status < 0)
    {
        if (errno == EBADF)
            perror("Error : second file is not a valid file descriptor or is not open for reading"
                           ".\n");
        else if (errno == EFAULT)
            perror("Error : char2 is outside your accessible address space.\n");
        else if (errno == EIO)
            perror("Error : I/O error while reading the second file.\n");
        else if (errno == EISDIR)
            perror("Error : second file descriptor points to a directory.\n");
        else
            perror("Error : unknown error while reading second file.\n");
        exit(3);
    }
}