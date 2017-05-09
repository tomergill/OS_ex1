/*******************************************************************************
 * Student name: Tomer Gill
 * Student: 318459450
 * Course Exercise Group: 01 (CS student, actual group is 89231-03)
 * Exercise name: Exercise 1
*******************************************************************************/

#include <malloc.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <fcntl.h>
#include <dirent.h>
#include <sys/stat.h>
#include <string.h>

//defines the max size for each line in the configuration file.
#define LINE_SIZE 160

#define IS_C_FILE(name, len) ((name[len - 1] == 'c') && (name[len - 2] == '.'))

typedef enum {OK = 0, NO_C_FILE, COMPILATION_ERROR, TIMEOUT, BAD_OUTPUT,
    SIMILLAR_OUTPUT, MULTIPLE_DIRECTORY} PENALTY;

typedef enum {FALSE = 0, TRUE = 1} BOOL;

typedef struct
{
    PENALTY penalty1;
    int wrongDirectoryDepth;
}penalties_t;

int readLineFromFile(int fd, char line[LINE_SIZE + 1]);
BOOL isDir(char *name, int resultsFd, DIR *mainDir);
void myOpenDir(DIR **dir, char *path, int resultsFd, DIR *mainDir);
char * findCFileInLevel(char *path, int resultsFd, DIR *mainDir);
void handleStudentDir(int depth, penalties_t **penalties,
                      char path[PATH_MAX + 1], int resultsFd, DIR *mainDir);
void myWrite(int fd, char *buffer, unsigned int size, int resultsFd, DIR
*mainDir);
void gotZero(int resultsFd, char *message, DIR *mainDir);

int main(int argc, char *argv[])
{
    char folderLocation[LINE_SIZE + 1], inputLocation[LINE_SIZE + 1],
            outputLocation[LINE_SIZE + 1];
    int configFd, resultsFd, counter = 1;
    DIR *mainDir = NULL, *childDir = NULL;
    struct dirent *curDirent = NULL;
    penalties_t p, *penalties;
    penalties = &p;
    char path[PATH_MAX + 1];
    int grade;

    if (argc < 2)
    {
        perror("Bad Usage");
        return 1;
    }

    /*
     * Open the config file.
     */
    printf("Opening config file\n");
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
    if ((resultsFd = open("results.csv", O_WRONLY | O_CREAT | O_TRUNC,
                          S_IRUSR | S_IRGRP | S_IWUSR)) == -1)
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

    myOpenDir(&mainDir, folderLocation, resultsFd, NULL);

    if (chdir(folderLocation) == -1)//changing the working directory to main dir
    {
        //failed
        printf("errno = %d", errno);
        if (errno == EACCES)
            perror("Access error changing directory");
        else if (errno == EIO)
            perror("I/O error while changing directories");
        else if (errno == ELOOP)
            perror("Too many symbolic links were encountered in resolving "
                           "path");
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
     * Reading each dirent from teh directory stream and handling the
     * ones that are directories.
     * Note: counter starts with value 1 (see above).
     */
    while ((curDirent = readdir(mainDir)) != NULL) //TODO myReadDir
    {
        if (curDirent->d_name == "." || curDirent->d_name == "..")
            continue;

        printf("%d dirent named %s", counter, curDirent->d_name);

        /*
         * If directory, open it and check for c file in it;
         */
        if (isDir(curDirent->d_name, resultsFd, mainDir))
        {
            penalties = &p;
            penalties->penalty1 = OK;
            penalties->wrongDirectoryDepth = 0;
            if (strlen(folderLocation) + strlen(curDirent->d_name) + 1 >
                PATH_MAX)
            {
                perror("PATH TOO LONG");
                continue;
            }
            strcpy(path, folderLocation);
            strcat(path, "/");
            strcat(path, curDirent->d_name);
            handleStudentDir(0, &penalties, path, resultsFd, mainDir);

            /*
             * writing to results.csv
             */
            myWrite(resultsFd, curDirent->d_name, strlen(curDirent->d_name),
                    resultsFd, mainDir); //write name
            myWrite(resultsFd, ",", strlen(","), resultsFd, mainDir);

            char *pnlty = NULL;
            switch (penalties->penalty1)
            {
                case NO_C_FILE:
                    gotZero(resultsFd, "NO_C_FILE\n", mainDir);
                    counter++;
                    continue;
                case MULTIPLE_DIRECTORY:
                    gotZero(resultsFd, "MULTIPLE_DIRECTORIES\n", mainDir);
                    counter++;
                    continue;
                case COMPILATION_ERROR:
                    gotZero(resultsFd, "COMPILE_ERROR\n", mainDir);
                    counter++;
                    continue;
                case TIMEOUT:
                    gotZero(resultsFd, "TIMEOUT\n", mainDir);
                    counter++;
                    continue;
                case BAD_OUTPUT:
                    gotZero(resultsFd, "BAD_OUTPUT\n", mainDir);
                    counter++
                    continue;
                case SIMILLAR_OUTPUT:
                    pnlty = "SIMILLAR_OUTPUT";
                    grade = 70;
                    break;
                default:
                    grade = 100;
                    break;
            }
            grade -= 10 * penalties->wrongDirectoryDepth;
            grade = grade < 0 ? 0 : grade;
            char temp[5];
            //writing grade
            sprintf(temp, "%d,", grade);
            myWrite(resultsFd, temp, strlen(temp), resultsFd, mainDir);

            if (pnlty != NULL)
            {
                myWrite(resultsFd, pnlty, strlen(pnlty), resultsFd, mainDir);
                if (penalties->wrongDirectoryDepth > 0)
                    myWrite(resultsFd, ",WRONG_DIRECTORY",
                            strlen(",WRONG_DIRECTORY"), resultsFd, mainDir);
                myWrite(resultsFd, "\n", 1, resultsFd, mainDir);
            } else if (penalties->wrongDirectoryDepth > 0)
                myWrite(resultsFd, "WRONG_DIRECTORY\n",
                        strlen("WRONG_DIRECTORY\n"), resultsFd, mainDir);
            else
                myWrite(resultsFd, "GREAT_JOB\n", strlen("GREAT_JOB\n"),
                        resultsFd, mainDir);
        }
        counter++;
    }
    closedir(mainDir);
    close(resultsFd);
    return 0;
}

void gotZero(int resultsFd, char *message, DIR *mainDir)
{
    myWrite(resultsFd, "0,", strlen("0,"), resultsFd, mainDir);
    myWrite(resultsFd, message, strlen(message), resultsFd, mainDir);
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

BOOL isDir(char *name, int resultsFd, DIR *mainDir)
{
    struct stat stat1;
    if ((stat(name, &stat1)) == -1) //getting details on name
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
        exit(2);
    }
    if (S_ISDIR(stat1.st_mode))
        return TRUE;
    return FALSE;
}

void myOpenDir(DIR **dir, char *path, int resultsFd, DIR *mainDir)
{
    if ((*dir = opendir(path)) == NULL) //opening dir
    {
        //failed
        printf("errno = %d\n", errno);
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
        if (resultsFd > 2)
            close(resultsFd);
        if (mainDir != NULL)
            closedir(mainDir);
        exit (2);
    }
}

char * findCFileInLevel(char *path, int resultsFd, DIR *mainDir)
{
    DIR *dir;
    struct dirent *d;
    myOpenDir(&dir, path, resultsFd, mainDir);
    while ((d = readdir(dir)) != NULL)
    {
        if (d->d_name == "." || d->d_name == "..")
            continue;
        if (IS_C_FILE(d->d_name, strlen(d->d_name) + 1))
        {
            closedir(dir);
            return d->d_name;
        }
    }
    return NULL;
}

char *findOnlyDirectoryName(char *path, int resultsFd, DIR *mainDir)
{
    DIR *dir;
    struct dirent *d, *temp;
    char dirPath[PATH_MAX + 1], tempPath[PATH_MAX+1];
//    int length = strlen(path);


    myOpenDir(&dir, path, resultsFd, mainDir);

    //find a directory
    while ((d = readdir(dir)) != NULL)
    {
        if (d->d_name == "." || d->d_name == "..")
            continue;
        if (strlen(d->d_name) + strlen(path) > PATH_MAX)
        {
            perror("PATH TOO LONG");
            continue;
        }
        strcpy(dirPath, path);
        strcat(dirPath, "/");
        strcat(dirPath, d->d_name);
        if (isDir(dirPath, resultsFd, mainDir))
            break;
    }

    if (d == NULL)
        return "";  //indicates there is no directory

    //search if there is another dir
    while ((temp = readdir(dir)) != NULL)
    {
        if (temp->d_name == "." || temp->d_name == "..")
            continue;
        if (strlen(temp->d_name) + strlen(path) > PATH_MAX)
        {
            perror("PATH TOO LONG");
            continue;
        }
        strcpy(tempPath, path);
        strcat(tempPath, "/");
        strcat(tempPath, temp->d_name);
        if (isDir(tempPath, resultsFd, mainDir))
            return NULL; //indicates there are at least 2 directories or more
    }
    closedir(dir);
    return d->d_name;
}

void handleStudentDir(int depth, penalties_t **penalties,
                      char path[PATH_MAX + 1], int resultsFd, DIR *mainDir)
{
    char cPath[PATH_MAX + 1], *name;

//    if (isFolderEmpty(path, resultsFd, mainDir)) //folder is empty
//    {
//        (*penalties)->penalty1 = NO_C_FILE;
//        return;
//    }
    if ((name = findCFileInLevel(path, resultsFd, mainDir)) != NULL)
    {
        //there is a c file, called name
        strcpy(cPath, path);
        if (strlen(name) + strlen(cPath) > PATH_MAX)
        {
            perror("PATH TOO LONG");
            *penalties = NULL;
            return;
        }
        strcat(cPath, "/");
        strcat(cPath, name);
        //TODO stufffffff
    }
    //there is no c file
    if ((name = findOnlyDirectoryName(path, resultsFd, mainDir)) != NULL &&
            strcmp(name, "") != 0) //name isn't "" ot NULL
    {
        //only one directory
        if (strlen(name) + strlen(path) > PATH_MAX)
        {
            perror("PATH TOO LONG");
            *penalties = NULL;
            return;
        }
        strcpy(cPath, path);
        strcat(cPath, "/");
        strcat(cPath, name);
        handleStudentDir(depth + 1, penalties, cPath, resultsFd, mainDir);
        return;
    }
    if (strcmp(name, "") == 0) //no more directories
    {
        (*penalties)->penalty1 = NO_C_FILE;
        (*penalties)->wrongDirectoryDepth = 0;
        return;
    }
    //two or more directories == MULTIPLE DIRECTORIES
    (*penalties)->penalty1 = MULTIPLE_DIRECTORY;
    (*penalties)->wrongDirectoryDepth = 0;
    return;
}

void myWrite(int fd, char *buffer, unsigned int size, int resultsFd, DIR
*mainDir)
{
    printf("writing %s to fd %d\n", buffer, fd);
    if (write(fd, buffer, size) < 0)
    {
        printf("errno = %d\n", errno);
        switch (errno)
        {
            case EBADF:
                perror("fd is not a valid file descriptor or is not open for "
                               "writing");
                break;
            case EFAULT:
                perror("segmentation fault while writing");
                break;
            case EFBIG:
                perror("An attempt was made to write a file that exceeds the "
                               "implementation-defined maximum file size or "
                               "the process's file size limit, or to write at"
                               " a position past the maximum allowed offset");
                break;
            case EINVAL:
                perror("fd is attached to an object which is unsuitable for "
                               "writing; or the file was opened with the "
                               "O_DIRECT flag, and either the address "
                               "specified in buf, the value specified in "
                               "count, or the current file offset is not "
                               "suitably aligned");
                break;
            case EIO:
                perror("A low-level I/O error occurred while modifying the "
                               "inode");
                break;
            case ENOSPC:
                perror("Not enough space to write to the file");
                break;
            default:
                perror("Unknown error while writing file");
                break;
        }
        closedir(mainDir);
        close(resultsFd);
        if (fd != resultsFd)
            close(fd);
        exit(2);
    }
}

//BOOL isFolderEmpty(char *path, int resultsFd, DIR *mainDir)
//{
//    DIR *dir;
//    struct dirent *d;
//    myOpenDir(&dir, path, resultsFd, mainDir);
//    BOOL isEmpty = FALSE;
//
//    if ((d = readdir(dir)) == NULL)
//        isEmpty = TRUE;
//    closedir(dir);
//    return isEmpty;
//}

/*
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
}*/
