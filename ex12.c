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
#include <sys/wait.h>

//defines the max size for each line in the configuration file.
#define LINE_SIZE 160

#define IS_C_FILE(name, len) ((name[len - 1] == 'c') && (name[len - 2] == '.'))

#define IS_HIDDEN_NAV_DIRS(name) ((!strcmp((name), ".")) || (!strcmp((name),"..")))

typedef enum {OK = 0, NO_C_FILE, COMPILATION_ERROR, TIMEOUT, BAD_OUTPUT,
    SIMILLAR_OUTPUT, MULTIPLE_DIRECTORY} PENALTY;

typedef enum {FALSE = 0, TRUE = 1} BOOL;

typedef struct
{
    PENALTY penalty1;
    int wrongDirectoryDepth;
}penalties_t;

int readLineFromFile(int fd, char line[LINE_SIZE + 1]);
BOOL isDir(char path[PATH_MAX + 1], int resultsFd, DIR *mainDir);
void myOpenDir(DIR **dir, char *path, int resultsFd, DIR *mainDir);
char * findCFileInLevel(char *path, int resultsFd, DIR *mainDir);
void handleStudentDir(int depth, penalties_t **penalties,
                      char path[PATH_MAX + 1], int resultsFd, DIR *mainDir,
                      char *inputFilePath, char *correctOutputFilePath);
void myWrite(int fd, char *buffer, size_t size, int resultsFd, DIR
*mainDir);
void gotZero(int resultsFd, char *message, DIR *mainDir);
struct dirent *myreaddir(DIR *dir, int resultsFd, DIR *mainDir);
BOOL cFileCompiled(char *path, int resultsFd, DIR *mainDir);
pid_t myfork(int resultsFd, DIR *mainDir);

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
        printf("errno = %d\n", errno);
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
        printf("errno = %d\n", errno);
        char *err;
        if (errno == EACCES)
            err = "Error : 'open' was unable to open the results file ";
        else
            err = "Error : unknown error opening the results file ";
        perror(err);
        return 2;
    }

    myOpenDir(&mainDir, folderLocation, resultsFd, NULL);

//    if (chdir(folderLocation) == -1)//changing the working directory to main dir
//    {
//        //failed
//        printf("errno = %d", errno);
//        if (errno == EACCES)
//            perror("Access error changing directory");
//        else if (errno == EIO)
//            perror("I/O error while changing directories");
//        else if (errno == ELOOP)
//            perror("Too many symbolic links were encountered in resolving "
//                           "path");
//        else if (errno == ENAMETOOLONG)
//            perror("Name too long");
//        else if (errno == ENOENT)
//            perror("Directory doesn't exist");
//        else if (errno == ENOTDIR)
//            perror("Error changing directory: path isn't a directory");
//        else
//            perror("Error changing directory");
//        close(resultsFd);
//        closedir(mainDir);
//        return 2;
//    }

    /*
     * Reading each dirent from teh directory stream and handling the
     * ones that are directories.
     * Note: counter starts with value 1 (see above).
     */
    while ((curDirent = myreaddir(mainDir, resultsFd, mainDir)) != NULL)
    {
        if (IS_HIDDEN_NAV_DIRS(curDirent->d_name))
            continue;

        printf("%d dirent named %s\n", counter, curDirent->d_name);

        /*
         * If directory, open it and check for c file in it;
         */
        if (strlen(folderLocation) + strlen(curDirent->d_name) + 1 > PATH_MAX)
        {
            perror("PATH TOO LONG");
            continue;
        }
        strcpy(path, folderLocation);
        strcat(path, "/");
        strcat(path, curDirent->d_name);
        if (isDir(path, resultsFd, mainDir))
        {
            penalties = &p;
            penalties->penalty1 = OK;
            penalties->wrongDirectoryDepth = 0;
//            if (strlen(folderLocation) + strlen(curDirent->d_name) + 1 >
//                PATH_MAX)
//            {
//                perror("PATH TOO LONG");
//                continue;
//            }
//            strcpy(path, folderLocation);
//            strcat(path, "/");
//            strcat(path, curDirent->d_name);
            handleStudentDir(0, &penalties, path, resultsFd, mainDir,
                             inputLocation, outputLocation);
            if (penalties == NULL)
                continue;

            /*
             * writing to results.csv
             */
            myWrite(resultsFd, curDirent->d_name, strlen(curDirent->d_name),
                    resultsFd, mainDir); //write name
            myWrite(resultsFd, ",", strlen(","), resultsFd, mainDir);

            char *pnlty = NULL;
            grade = 0;
            BOOL isWrongDirectory = penalties->wrongDirectoryDepth > 0 ? TRUE
                                                                       : FALSE;
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
                    pnlty = "BAD_OUTPUT";
                    grade = 0;
                    break;
                case SIMILLAR_OUTPUT:
                    pnlty = "SIMILLAR_OUTPUT";
                    grade = 70;
                    break;
                default:
                    pnlty = "GREAT_JOB";
                    grade = 100;
                    break;
            }
            grade -= 10 * penalties->wrongDirectoryDepth;
            grade = grade < 0 ? 0 : grade;
            char temp[5];
            //writing grade
            sprintf(temp, "%d,", grade);
            myWrite(resultsFd, temp, strlen(temp), resultsFd, mainDir);
            if (!penalties->penalty1 == OK)
            {
                myWrite(resultsFd, pnlty, strlen(pnlty), resultsFd, mainDir);
                if (isWrongDirectory)
                    myWrite(resultsFd, ",WRONG_DIRECTORY",
                            strlen(",WRONG_DIRECTORY"), resultsFd, mainDir);
            }
            else
            {
                if (isWrongDirectory)
                    myWrite(resultsFd, "WRONG_DIRECTORY,",
                            strlen(",WRONG_DIRECTORY"), resultsFd, mainDir);
                myWrite(resultsFd, pnlty, strlen(pnlty), resultsFd, mainDir);
            }
            myWrite(resultsFd, "\n", strlen("\n"), resultsFd, mainDir);
        }
        counter++;
    }
    closedir(mainDir);
    close(resultsFd);
    unlink("./output.txt");
    unlink("./a.out");
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
            printf("errno = %d\n", errno);
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

BOOL isDir(char path[PATH_MAX + 1], int resultsFd, DIR *mainDir)
{
    struct stat stat1;
    if ((stat(path, &stat1)) == -1) //getting details on name
    {
        printf("errno = %d\n", errno);
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
    while ((d = myreaddir(dir, resultsFd, mainDir)) != NULL)
    {
        if (IS_HIDDEN_NAV_DIRS(d->d_name))
            continue;
        if (IS_C_FILE(d->d_name, strlen(d->d_name)))
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
    while ((d = myreaddir(dir, resultsFd, mainDir)) != NULL)
    {
        if (IS_HIDDEN_NAV_DIRS(d->d_name))
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
    while ((temp = myreaddir(dir, resultsFd, mainDir)) != NULL)
    {
        if (IS_HIDDEN_NAV_DIRS(temp->d_name))
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
                      char path[PATH_MAX + 1], int resultsFd, DIR *mainDir,
                      char *inputFilePath, char *correctOutputFilePath)
{
    char cPath[PATH_MAX + 1], *name;
    if ((name = findCFileInLevel(path, resultsFd, mainDir)) != NULL)
    {
        printf("found c file: %s/%s\n", path, name);
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

        /*
         * Trying to compile
         */
        if (!cFileCompiled(cPath, resultsFd, mainDir))
        {
            (*penalties)->penalty1 = COMPILATION_ERROR;
            (*penalties)->wrongDirectoryDepth = 0;
            return;
        }

        /*
         * Executing the out file.
         */
        int inputFd = open(inputFilePath, O_RDONLY), outputFd = open
                ("./output.txt", O_WRONLY | O_CREAT | O_TRUNC,  S_IWUSR | S_IRUSR |
                        S_IRGRP);
        if (inputFd == -1 || outputFd == -1)
        {
            perror("can't open input/output file");
            *penalties = NULL;
            return;
        }
        printf("executing a.out\n");
        int copyStdout =dup(1), copyStdin = dup(0);
        dup2(inputFd, 0); //using input file as STDIN
        dup2(outputFd, 1); //using output file as STDOUT

        int stat; char *args[] = {"./a.out", NULL};
        pid_t cid = myfork(resultsFd, mainDir);
        if (cid == 0) //child
        {

            if (execvp("./a.out",  args) == -1)
            {
                perror("execvp error a.out");
                exit (-1);
            }
        }
        else
        {
            int i;
            BOOL under5Seconds = FALSE;
            for (i = 0; i < 5 && !under5Seconds; i++)
            {
                sleep(1);
                if (waitpid(cid, &stat, WNOHANG))
                    under5Seconds = TRUE;
            }
            //printf("under5Seconds is %s\n", under5Seconds ? "TRUE" : "FALSE");
            if (!under5Seconds)
            {
                //TODO kill process
                //TIMEOUT
                kill(cid, SIGKILL);
                wait(&stat);
                (*penalties)->penalty1 = TIMEOUT;
                (*penalties)->wrongDirectoryDepth = 0;
                close(outputFd);
                close(inputFd);
                dup2(copyStdin, 0);
                dup2(copyStdout, 1);
                unlink("./output");
                unlink("./a.out");
                return;
            }

            close(outputFd);
            close(inputFd);
            dup2(copyStdin, 0);
            dup2(copyStdout, 1);

            //comparing outputs
            char *args1[] = {"./comp.out", "./output.txt",
                             correctOutputFilePath, NULL};
            cid = myfork(resultsFd, mainDir);
            if (cid == 0) //child running compare
            {
//                printf("comparing output files\n");
//                char *args[] = {"./comp.out", "./output",
//                                correctOutputFilePath, NULL};
                if (execvp("./comp.out", args1) == -1)
                {
                    perror("Error running comp");
                    exit(110);
                }
            }
            else //father returning compare result.
            {
                int s;
                //sleep(15);
                wait(&s);
                if (WIFEXITED(s))
                {
                    printf("wala exit stauts is %d\n", WEXITSTATUS(s));
                    switch (WEXITSTATUS(s))
                    {
                        default:
                            *penalties = NULL;
                            return;
                        case 1:
                            (*penalties)->penalty1 = OK;
                            break;
                        case 2:
                            (*penalties)->penalty1 = SIMILLAR_OUTPUT;
                            break;
                        case 3:
                            (*penalties)->penalty1 = BAD_OUTPUT;
                            break;
                    }
                    (*penalties)->wrongDirectoryDepth = depth;
                    unlink("./output");
                    unlink("./a.out");
                    return;
                }
            }
        }
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
        handleStudentDir(depth + 1, penalties, cPath, resultsFd, mainDir,
                         inputFilePath, correctOutputFilePath);
        return;
    }
    if (name == NULL) //two or more directories == MULTIPLE DIRECTORIES
    {
        (*penalties)->penalty1 = MULTIPLE_DIRECTORY;
        (*penalties)->wrongDirectoryDepth = 0;
        return;
    }
    //no more directories
    (*penalties)->penalty1 = NO_C_FILE;
    (*penalties)->wrongDirectoryDepth = 0;
    return;
}

void myWrite(int fd, char *buffer, size_t size, int resultsFd, DIR
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

struct dirent *myreaddir(DIR *dir, int resultsFd, DIR *mainDir)
{
    int prevErrorno = errno;
    struct dirent * d = NULL;
    if ((d = readdir(dir)) == NULL && errno != prevErrorno)
    {
        switch (errno)
        {
            case EOVERFLOW:
                perror("One of the values in the structure to be returned "
                               "cannot be represented correctly.");
                break;
            case EBADF:
                perror("dir isn't pointing to an open dir");
                break;
            case ENOENT:
                perror("The current position of the directory stream is "
                               "invalid.");
                break;
            default:
                perror("Unknown error while getting dirent form dir");
                break;
        }
        closedir(mainDir);
        close(resultsFd);
        if (dir != mainDir)
            closedir(dir);
        exit(2);
    }
    return d;
}

BOOL cFileCompiled(char *path, int resultsFd, DIR *mainDir)
{
    pid_t cid;
    int stat;

    printf("compiling %s\n", path);
    char *args[] = {"gcc", path, NULL};
    if ((cid = myfork(resultsFd, mainDir)) == 0) //child proccess
    {

        if (execvp("gcc", args) == -1)
        {
            perror("can't run gcc");
            exit(1);
        }
    }
    //father
    wait(&stat);
    return  (WEXITSTATUS(stat) == 0) != 0 ? TRUE : FALSE;
}

pid_t myfork(int resultsFd, DIR *mainDir)
{
    pid_t cid;

    if ((cid = fork()) == -1)
    {
        switch (errno)
        {
            case EAGAIN:
                perror("Not enough space to copy process");
                break;
            case ENOMEM:
                perror("Not enough memory to copy process");
                break;
            default:
                perror("Unknown error while fork");
                break;
        }
        closedir(mainDir);
        close(resultsFd);
        exit(2);
    }
    return cid;
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
