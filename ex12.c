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
#include <sys/types.h>
#include <dirent.h>
#include <string.h>
#include <sys/stat.h>

#define LINE_SIZE 160 //defines the max size for each line in the configuration file.

int readLineFromFile(int fd, char line[LINE_SIZE + 1]);

int main(int argc, char *argv[])
{
    char folderLocation[LINE_SIZE + 1], inputLocation[LINE_SIZE + 1], outputLocation[LINE_SIZE + 1];
    int configFd, resultsFd, counter = 1;
    DIR *mainDir = NULL, *childDir = NULL;
    struct dirent *curDirent = NULL;
    struct stat stat1;

    if (argc < 2)
    {
        perror("Bad Usage");
        return 1;
    }

    /*
     * Open the config file.
     */
    printf("O"
                   "pening config file\n");
    if ((configFd = open(argv[1], O_RDONLY)) == -1)
    {
        printf("errno = %d", errno);
        char *err;
        if (errno == ENOENT)
            err = "Error : First file is NULL pointer ";
        else if (errno == EACCES)
            err = "Error : 'open' was unable to open the first file ";
        else
            err = "Error : unknown error opening the first file ";
        perror(err);
        return 2;
    }

    /*
     * Reading the 3 lines from the config file.
     */
    readLineFromFile(configFd, folderLocation);
    readLineFromFile(configFd, inputLocation);
    readLineFromFile(configFd, outputLocation);
    close(configFd);

    /*
     * Creating the results file for writing.
     */
    if ((resultsFd = open("results.csv", O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IRGRP |
                                                                       S_IWUSR)) == -1)
    {
        printf("errno = %d", errno);
        char *err;
        if (errno == EACCES)
            err = "Error : 'open' was unable to open the results file ";
        else
            err = "Error : unknown error opening the results file ";
        perror(err);
        return 2;
    }

    if ((mainDir = opendir(folderLocation)) == NULL) //opening dir
    {
        //failed
        printf("errno = %d", errno);
        if (errno == EACCES)
            perror("Error accessing the main directory");
        else if (errno == ELOOP)
            perror("Loop of symbolic links opening main directory");
        else if (errno == ENAMETOOLONG)
            perror("Name of main dir too long");
        else if (errno == ENOENT)
            perror("path to dir isn't pointing to a directory / it is an empty string");
        else if (errno == ENOTDIR)
            perror("path to dir isn't pointing to a directory");
        else
            perror("Unknown error opening main directory");
        close(resultsFd);
        return 2;
    }

    if (chdir(folderLocation) == -1) //changing the working directory to main folder
    {
        //failed
        printf("errno = %d", errno);
        if (errno == EACCES)
            perror("Access error changing directory");
        else if (errno == EIO)
            perror("I/O error while changing directories");
        else if (errno == ELOOP)
            perror("Too many symbolic links were encountered in resolving path");
        else if (errno == ENAMETOOLONG)
            perror("Name too long");
        else if (errno == ENOENT)
            perror("Directory doesn't exist");
        else if (errno == ENOTDIR)
            perror("Error changing directory: path isn't a directory");
        else
            perror("Error changing directory");
        close(resultsFd);
        closedir(mainDir);
        return 2;
    }

    /*
     * Reading each dirent from teh directory stream and handling the ones that are directories.
     * Note: counter starts with value 1 (see above).
     */
    while ((curDirent = readdir(mainDir)) != NULL) //read every file in the directory
    {
        printf("%d dirent named %s", counter, curDirent->d_name);
        if ((stat(curDirent->d_name, &stat1)) == -1) //getting details on curDirent
        {
            printf("errno = %d", errno);
            //failed
            if (errno == EACCES)
                perror("Error accessing stat of curDirent");
            else if (errno == EFAULT)
                perror("dirent bad address");
            else if (errno == ELOOP)
                perror("Too much symlinks");
            else if (errno == ENAMETOOLONG)
                perror("Name of dirent too long");
            else if (errno == ENOENT)
                perror("dirent doesn't exist");
            else
                perror("Error getting dirent stat");
            close(resultsFd);
            closedir(mainDir);
            return 2;
        }

        if (S_ISDIR(stat1.st_mode)) //check if dir
        {
            if ((childDir = opendir(curDirent->d_name)) == NULL) //opening dir
            {
                //failed
                printf("errno = %d, name = %s\n", errno, curDirent->d_name);
                if (errno == EACCES)
                    perror("Error accessing child directory");
                else if (errno == ELOOP)
                    perror("Loop of symbolic links opening child directory");
                else if (errno == ENAMETOOLONG)
                    perror("Name of child dir too long");
                else if (errno == ENOENT)
                    perror("path to child dir isn't pointing to a directory / it is an empty "
                                   "string");
                else if (errno == ENOTDIR)
                    perror("path to child isn't pointing to a directory");
                else
                    perror("Unknown error opening child directory");
                close(resultsFd);
                closedir(mainDir);
                return 2;
            }
        }
//        int len = strlen(curDirent->d_name);
//        if (curDirent->d_name[len - 2] == '.' && curDirent->d_name[len - 1] == 'c') //c file
//        {
//
//        }
        counter++;
    }
    closedir(mainDir);

    close(resultsFd);
    return 0;
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
            printf("errno = %d", errno);
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
    line[LINE_SIZE] = '\0';
    return i + 1;
}