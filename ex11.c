/*******************************************************************************
 * Student name: Tomer Gill
 * Student: 318459450
 * Course Exercise Group: 01 (CS student, actual group is 89231-03)
 * Exercise name: Exercise 1
*******************************************************************************/

#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>

//difference between an uppercase char to lowercase.
#define UPPER_TO_LOWER_DIFF ('a' - 'A')

//checks if c is ' ' or '\n'
#define IS_SPACING_CHAR(c) ((c) == ' ' || (c) == '\n' || (c) == '\t')

typedef enum
{
    FALSE = 0, TRUE = 1
} BOOL;

/*******************************************************************************
 * function name: readCharFromFile1
 * The Input: The file descriptor of file1
 * The output: The char read, at *char1, or finished1 = TRUE.
 * The Function operation: The function tries to read a single character from
 * file1. If succeeded it is in char1. If the read went all through the end
 * of file1, *finished is set to TRUE and char1's value should be ignored.
 * If there was an error while reading, an appropriate message
 * will be printed to stderr, and the program will return with exit code 3.
*******************************************************************************/
void ReadCharFromFile1(int file1, char *char1, BOOL *finished1);

/*******************************************************************************
 * function name: readCharFromFile2
 * The Input: The file descriptor of file2
 * The output: The char read, at *char2, or finished2 = TRUE.
 * The Function operation: The function tries to read a single character from
 * file2. If succeeded it is in char2. If the read went all through the end
 * of file2, finished is set to TRUE and char2's value should be ignored.
 * If there was an error while reading, an appropriate message will be printed
 * to stderr, and the program will return with exit code 3.
*******************************************************************************/
void ReadCharFromFile2(int file2, char *char2, BOOL *finished2);

//shortcuts
#define READ_1 ReadCharFromFile1(file1, &char1, &finished1)
#define READ_2 ReadCharFromFile2(file2, &char2, &finished2)

/*******************************************************************************
 * function name: main
 * The Input: 2 files to compare.
 * The output: returns with exit code 1 if files are equal, 2 if similar or 3 if
 * different.
 * The Function operation: The function reads one char at a time from each file,
 * and compare them. If they are different but similar (upper/lower versions,
 * or one of them is ' ' / '\n') the program lights a flag that says they are
 * similar. If a real difference is found, the program exits with exit code 3.
 * Also on an error in one of the system calls, the program writes an
 * appropriate error to stderr and exits with exit code 3.
*******************************************************************************/
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

    //Opening file1
    if ((file1 = open(argv[1], O_RDONLY)) == -1)
    {
        char *err;
        if (errno == ENOENT)
            err = "Error : First file is NULL pointer ";
        else if (errno == EACCES)
            err = "Error : 'open' was unable to open the first file ";
        else
            err = "Error : unknown error opening the first file ";
        perror(err);
        exit(3);
    }

    //Opening file2
    if ((file2 = open(argv[2], O_RDONLY)) == -1)
    {
        char *err;
        if (errno == ENOENT)
            err = "Error : Second file is NULL pointer ";
        else if (errno == EACCES)
            err = "Error : 'open' was unable to open the second file ";
        else
            err = "Error : unknown error opening the second file ";
        perror(err);
        exit(3);
    }

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
            if (abs(char1 - char2) == UPPER_TO_LOWER_DIFF)
            {
                //difference is upper/lower letter
                isSimilar = TRUE;
            }
            READ_1;
            READ_2;
            continue;
        }
        //characters are not equal
        if (IS_SPACING_CHAR(char1))
        {
            //if char1 is ' ' or '\n' count as similar and continue
            isSimilar = TRUE;
            READ_1;
            continue;
        }
        if (IS_SPACING_CHAR(char2)) //likewise for char2
        {
            isSimilar = TRUE;
            READ_2;
            continue;
        }

        //Definitely not equal
        if (close(file1) == -1)
            perror("problem closing file 1");
        if (close(file2) == -1)
            perror("problem closing file 2");
        exit(3);
    } //end of while(!finished1 && !finished2)

    /*
     * while there are still left chars to read in file1, read.
     * If all of them are ' ' or \n, the files are similar.
     */
    while (!finished1)
    {
        if (IS_SPACING_CHAR(char1))
            READ_1;
        else
        {
            if (close(file1) == -1)
                perror("problem closing file 1");
            if (close(file2) == -1)
                perror("problem closing file 2");
            return 3; //different
        }
    }

    /*
     * while there are still left chars to read in file2, read.
     * If all of them are ' ' or \n, the files are similar.
     */
    while (!finished2)
    {
        if (IS_SPACING_CHAR(char2))
            READ_2;
        else
        {
            if (close(file1) == -1)
                perror("problem closing file 1");
            if (close(file2) == -1)
                perror("problem closing file 2");
            return 3; //different
        }
    }

    /*
     * If the files are equal, isSimilar will be FALSE = 0, so 1 will be
     * returned. Other wise (they are similar) isSimilar will be TRUE = 1, so
     * 2 will be returned.
     * (It's clear to see that if the files are different, the program will exit
     * before, returning 3 as required.)
     */
    if (close(file1) == -1)
        perror("problem closing file 1");
    if (close(file2) == -1)
        perror("problem closing file 2");
    return 1 + isSimilar;
}

/*******************************************************************************
 * function name: readCharFromFile1
 * The Input: The file descriptor of file1
 * The output: The char read, at *char1, or finished1 = TRUE.
 * The Function operation: The function tries to read a single character from
 * file1. If succeeded it is in char1. If the read went all through the end
 * of file1, *finished is set to TRUE and char1's value should be ignored.
 * If there was an error while reading, an appropriate message
 * will be printed to stderr, and the program will return with exit code 3.
*******************************************************************************/
void ReadCharFromFile1(int file1, char *char1, BOOL *finished1)
{
    int status = read(file1, char1, sizeof(char));
    if (status == 0)
    {
        *finished1 = TRUE;
        return;
    }
    else if (status < 0)
    {
        if (errno == EBADF)
            perror("Error : first file is not a valid file descriptor or is "
                           "not open for reading.\n");
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

/*******************************************************************************
 * function name: readCharFromFile2
 * The Input: The file descriptor of file2
 * The output: The char read, at *char2, or finished2 = TRUE.
 * The Function operation: The function tries to read a single character from
 * file2. If succeeded it is in char2. If the read went all through the end
 * of file2, finished is set to TRUE and char2's value should be ignored.
 * If there was an error while reading, an appropriate message will be printed
 * to stderr, and the program will return with exit code 3.
*******************************************************************************/
void ReadCharFromFile2(int file2, char *char2, BOOL *finished2)
{
    int status = read(file2, char2, sizeof(char));
    if (status == 0)
    {
        *finished2 = TRUE;
        return;
    }
    else if (status < 0)
    {
        if (errno == EBADF)
            perror("Error : second file is not a valid file descriptor or is "
                           "not open for reading.\n");
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
