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

//returns 1 if file name ends with ".c", hence a c file
#define IS_C_FILE(name, len) ((name[len - 1] == 'c') && (name[len - 2] == '.'))

//returns 1 if name is "." or ".."
#define IS_HIDDEN_NAV_DIRS(name) ((!strcmp((name), ".")) || (!strcmp((name),"..")))

//enum that represent a grade penalty reason, except WRONG_DIRECTORY.
typedef enum {
    GREAT_JOB = 0, NO_C_FILE, COMPILATION_ERROR, TIMEOUT, BAD_OUTPUT,
    SIMILLAR_OUTPUT, MULTIPLE_DIRECTORY
} PENALTY;

typedef enum {
    FALSE = 0, TRUE = 1
} BOOL;

/*
 * Holds the reason why the student's grade should be subtracted :(
 * and the depth of directories the c file was in.
 * Note: penalty1 could be GREAT_JOB if erverything is good, and
 * wrongDirectoryDepth could be 0 if the file was in main student's folder.
 */
typedef struct {
    PENALTY penalty1;
    int wrongDirectoryDepth;
} penalties_t;

/******************************************************************************
* function name: ReadLineFromFile
* The Input: The file descriptor of the file, and a buffer for the line.
* The output: The size of the line.
* The Function operation: reads one character at a time, insert into buffer,
 * and if found '\n' or file has been finished read, then stop.
*******************************************************************************/
int ReadLineFromFile(int fd, char *line);

/******************************************************************************
* function name: IsDir
* The Input: A path to a directory, the file descriptor of the results file
 * and the DIR stream of the main directory.
* The output: TRUE if path points to a directory, FALSE otherwise.
* The Function operation: Opens the path as struct stat and returns the value
 * of S_ISDIR.
*******************************************************************************/
BOOL IsDir(char *path, int resultsFd, DIR *mainDir);

/******************************************************************************
* function name: MyOpenDir
* The Input: A pointer to a pointer to an DIR stream, a path to a
 * directory, the file descriptor of the results file and the DIR stream of
 * the main directory.
* The output: *dir will point to DIR stream if opendir worked.
* The Function operation: Tries to opendir(path), and if works points *DIR to
 * it.
*******************************************************************************/
void MyOpenDir(DIR **dir, char *path, int resultsFd, DIR *mainDir);

/******************************************************************************
* function name: FindCFileInLevel
* The Input: A path to a student's directory, the file descriptor of the
 * results file and the DIR stream of the main directory.
* The output: the name of the c-file if exists, otherwise NULL.
* The Function operation: Opens the path as DIR stream, then loop through all
 * files in the directory looking for the c-file.
*******************************************************************************/
char *FindCFileInLevel(char *path, int resultsFd, DIR *mainDir);

void HandleStudentDir(int depth, penalties_t **penalties,
                      char *path, int resultsFd, DIR *mainDir,
                      char *inputFilePath, char *correctOutputFilePath);

void MyWrite(int fd, char *buffer, size_t size, int resultsFd, DIR
*mainDir);

void GotZero(int resultsFd, char *message, DIR *mainDir);

struct dirent *MyReadDir(DIR *dir, int resultsFd, DIR *mainDir);

BOOL CFileCompiled(char *path, int resultsFd, DIR *mainDir);

pid_t MyFork(int resultsFd, DIR *mainDir);

/******************************************************************************
* function name: FindOnlyDirectoryName
* The Input: A path to a student's directory, the file descriptor of the
 * results file and the DIR stream of the main directory.
* The output: the name of the only directory if exists, otherwise NULL if
 * there are two or more directories or "" if there is no directory there.
* The Function operation: Opens the path as DIR stream, then loop through all
 * files in the directory looking for a directory, if not found return "". If
 * found, searches for another directory. If found another returns NULL,
 * otherwise the only directory name.
*******************************************************************************/
char *FindOnlyDirectoryName(char *path, int resultsFd, DIR *mainDir);

int main(int argc, char *argv[]) {
    char folderLocation[LINE_SIZE + 1], inputLocation[LINE_SIZE + 1],
            outputLocation[LINE_SIZE + 1];
    int configFd, resultsFd, counter = 1;
    DIR *mainDir = NULL, *childDir = NULL;
    struct dirent *curDirent = NULL;
    penalties_t p, *penalties;
    penalties = &p;
    char path[PATH_MAX + 1];
    int grade;

    if (argc < 2) {
        perror("Bad Usage");
        return 1;
    }

    /*
     * Open the config file.
     */
    printf("Opening config file\n");
    if ((configFd = open(argv[1], O_RDONLY)) == -1) {
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
    ReadLineFromFile(configFd, folderLocation);
    ReadLineFromFile(configFd, inputLocation);
    ReadLineFromFile(configFd, outputLocation);
    close(configFd);

    /*
     * Creating the results file for writing.
     */
    if ((resultsFd = open("results.csv", O_WRONLY | O_CREAT | O_TRUNC,
                          S_IRUSR | S_IRGRP | S_IWUSR)) == -1) {
        printf("errno = %d\n", errno);
        char *err;
        if (errno == EACCES)
            err = "Error : 'open' was unable to open the results file ";
        else
            err = "Error : unknown error opening the results file ";
        perror(err);
        return 2;
    }

    MyOpenDir(&mainDir, folderLocation, resultsFd, NULL);

    /*
     * Reading each dirent from teh directory stream and handling the
     * ones that are directories.
     * Note: counter starts with value 1 (see above).
     */
    while ((curDirent = MyReadDir(mainDir, resultsFd, mainDir)) != NULL) {
        if (IS_HIDDEN_NAV_DIRS(curDirent->d_name))
            continue;

        printf("%d dirent named %s\n", counter, curDirent->d_name);

        /*
         * If directory, open it and check for c file in it;
         */
        if (strlen(folderLocation) + strlen(curDirent->d_name) + 1 > PATH_MAX) {
            perror("PATH TOO LONG");
            continue;
        }
        strcpy(path, folderLocation);
        strcat(path, "/");
        strcat(path, curDirent->d_name);
        if (IsDir(path, resultsFd, mainDir)) {
            penalties = &p;
            penalties->penalty1 = GREAT_JOB;
            penalties->wrongDirectoryDepth = 0;
            HandleStudentDir(0, &penalties, path, resultsFd, mainDir,
                             inputLocation, outputLocation);
            if (penalties == NULL)
                continue;

            /*
             * writing to results.csv
             */
            MyWrite(resultsFd, curDirent->d_name, strlen(curDirent->d_name),
                    resultsFd, mainDir); //write name
            MyWrite(resultsFd, ",", strlen(","), resultsFd, mainDir);

            char *pnlty = NULL;
            BOOL isWrongDirectory = penalties->wrongDirectoryDepth > 0 ? TRUE
                                                                       : FALSE;
            switch (penalties->penalty1) {
                case NO_C_FILE:
                    GotZero(resultsFd, "NO_C_FILE\n", mainDir);
                    counter++;
                    continue;
                case MULTIPLE_DIRECTORY:
                    GotZero(resultsFd, "MULTIPLE_DIRECTORIES\n", mainDir);
                    counter++;
                    continue;
                case COMPILATION_ERROR:
                    GotZero(resultsFd, "COMPILE_ERROR\n", mainDir);
                    counter++;
                    continue;
                case TIMEOUT:
                    GotZero(resultsFd, "TIMEOUT\n", mainDir);
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
            MyWrite(resultsFd, temp, strlen(temp), resultsFd, mainDir);
            if (!penalties->penalty1 == GREAT_JOB) {
                MyWrite(resultsFd, pnlty, strlen(pnlty), resultsFd, mainDir);
                if (isWrongDirectory)
                    MyWrite(resultsFd, ",WRONG_DIRECTORY",
                            strlen(",WRONG_DIRECTORY"), resultsFd, mainDir);
            } else {
                if (isWrongDirectory)
                    MyWrite(resultsFd, "WRONG_DIRECTORY,",
                            strlen(",WRONG_DIRECTORY"), resultsFd, mainDir);
                MyWrite(resultsFd, pnlty, strlen(pnlty), resultsFd, mainDir);
            }
            MyWrite(resultsFd, "\n", strlen("\n"), resultsFd, mainDir);
        }
        counter++;
    }
    closedir(mainDir);
    close(resultsFd);
    unlink("./output.txt");
    unlink("./temp.out");
    return 0;
}

void GotZero(int resultsFd, char *message, DIR *mainDir) {
    MyWrite(resultsFd, "0,", strlen("0,"), resultsFd, mainDir);
    MyWrite(resultsFd, message, strlen(message), resultsFd, mainDir);
}

/*******************************************************************************
 * function name: readLineFromFile
 * The Input: The file descriptor of the file, and a buffer for the line.
 * The output: The size of the line.
 * The Function operation: reads one character at a time, insert into buffer,
 * and if found '\n' or file has been finished read, then stop.
*******************************************************************************/
int ReadLineFromFile(int fd, char *line) {
    char c;
    int i = 0;

    for (; i < LINE_SIZE; ++i) {
        int status = read(fd, &c, sizeof(char));
        if (status == 0) {
            line[i] = '\0';
            return i + 1;
        } else if (status == -1) {
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
        if (c == '\n') {
            line[i] = '\0';
            return i + 1;
        }
        line[i] = c;
    }
    line[LINE_SIZE] = '\0';
    return i + 1;
}

/******************************************************************************
* function name: isDir
* The Input: A path to a directory, the file descriptor of the results file
 * and the DIR stream of the main directory.
* The output: TRUE if path points to a directory, FALSE otherwise.
* The Function operation: Opens the path as struct stat and returns the value
 * of S_ISDIR.
*******************************************************************************/
BOOL IsDir(char *path, int resultsFd, DIR *mainDir) {
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

/******************************************************************************
* function name: MyOpenDir
* The Input: A pointer to a pointer to an DIR stream, a path to a
 * directory, the file descriptor of the results file and the DIR stream of
 * the main directory.
* The output: *dir will point to DIR stream if opendir worked.
* The Function operation: Tries to opendir(path), and if works points *DIR to
 * it.
*******************************************************************************/
void MyOpenDir(DIR **dir, char *path, int resultsFd, DIR *mainDir) {
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
        exit(2);
    }
}

/******************************************************************************
* function name: FindCFileInLevel
* The Input: A path to a student's directory, the file descriptor of the
 * results file and the DIR stream of the main directory.
* The output: the name of the c-file if exists, otherwise NULL.
* The Function operation: Opens the path as DIR stream, then loop through all
 * files in the directory looking for the c-file.
*******************************************************************************/
char *FindCFileInLevel(char *path, int resultsFd, DIR *mainDir) {
    DIR *dir;
    struct dirent *d;
    MyOpenDir(&dir, path, resultsFd, mainDir);
    while ((d = MyReadDir(dir, resultsFd, mainDir)) != NULL) {
        if (IS_HIDDEN_NAV_DIRS(d->d_name))
            continue;
        if (IS_C_FILE(d->d_name, strlen(d->d_name))) {
            closedir(dir);
            return d->d_name;
        }
    }
    return NULL;
}

/******************************************************************************
* function name: FindOnlyDirectoryName
* The Input: A path to a student's directory, the file descriptor of the
 * results file and the DIR stream of the main directory.
* The output: the name of the only directory if exists, otherwise NULL if
 * there are two or more directories or "" if there is no directory there.
* The Function operation: Opens the path as DIR stream, then loop through all
 * files in the directory looking for a directory, if not found return "". If
 * found, searches for another directory. If found another returns NULL,
 * otherwise the only directory name.
*******************************************************************************/
char *FindOnlyDirectoryName(char *path, int resultsFd, DIR *mainDir) {
    DIR *dir;
    struct dirent *d, *temp;
    char dirPath[PATH_MAX + 1], tempPath[PATH_MAX + 1];
//    int length = strlen(path);


    MyOpenDir(&dir, path, resultsFd, mainDir);

    //find a directory
    while ((d = MyReadDir(dir, resultsFd, mainDir)) != NULL) {
        if (IS_HIDDEN_NAV_DIRS(d->d_name))
            continue;
        if (strlen(d->d_name) + strlen(path) > PATH_MAX) {
            perror("PATH TOO LONG");
            continue;
        }
        strcpy(dirPath, path);
        strcat(dirPath, "/");
        strcat(dirPath, d->d_name);
        if (IsDir(dirPath, resultsFd, mainDir))
            break;
    }

    if (d == NULL)
        return "";  //indicates there is no directory

    //search if there is another dir
    while ((temp = MyReadDir(dir, resultsFd, mainDir)) != NULL) {
        if (IS_HIDDEN_NAV_DIRS(temp->d_name))
            continue;
        if (strlen(temp->d_name) + strlen(path) > PATH_MAX) {
            perror("PATH TOO LONG");
            continue;
        }
        strcpy(tempPath, path);
        strcat(tempPath, "/");
        strcat(tempPath, temp->d_name);
        if (IsDir(tempPath, resultsFd, mainDir))
            return NULL; //indicates there are at least 2 directories or more
    }
    closedir(dir);
    return d->d_name;
}

/******************************************************************************
 * function name: HandleStudentDir.
 * The Input: The depth of sub folders from the student's directory (starts
 * at 0), a pointer to a pointer to a penalties_t struct, a path to the
 * current folder, the file descriptor of the results file, the DIR stream
 * of the main directory, the path for the input file and the path of the
 * correct output to compare to.
 * The output: penalties will hold either a penalty (except WRONG_DIRECTORY),
 * the depth of sub-directories the c-file is in, both of them (BO/SO/GJ + WD)
 * or NULL if there was an error the process didn't exit because of it.
 * The Function operation:
*******************************************************************************/
 void HandleStudentDir(int depth, penalties_t **penalties,
                      char *path, int resultsFd, DIR *mainDir,
                      char *inputFilePath, char *correctOutputFilePath) {
    char cPath[PATH_MAX + 1], *name;
    if ((name = FindCFileInLevel(path, resultsFd, mainDir)) != NULL) {
        printf("found c file: %s/%s\n", path, name);
        //there is a c file, called name
        strcpy(cPath, path);
        if (strlen(name) + strlen(cPath) > PATH_MAX) {
            perror("PATH TOO LONG");
            *penalties = NULL;
            return;
        }
        strcat(cPath, "/");
        strcat(cPath, name);

        /*
         * Trying to compile
         */
        if (!CFileCompiled(cPath, resultsFd, mainDir)) {
            (*penalties)->penalty1 = COMPILATION_ERROR;
            (*penalties)->wrongDirectoryDepth = 0;
            return;
        }

        /*
         * Executing the out file.
         */
        int inputFd = open(inputFilePath, O_RDONLY), outputFd = open
                ("./output.txt", O_WRONLY | O_CREAT | O_TRUNC,
                 S_IWUSR | S_IRUSR |
                 S_IRGRP);
        if (inputFd == -1 || outputFd == -1) {
            perror("can't open input/output file");
            *penalties = NULL;
            return;
        }
        printf("executing temp.out\n");
        int copyStdout = dup(1), copyStdin = dup(0);
        dup2(inputFd, 0); //using input file as STDIN
        dup2(outputFd, 1); //using output file as STDOUT

        int stat;
        char *args[] = {"./temp.out", NULL};
        pid_t cid = MyFork(resultsFd, mainDir);
        if (cid == 0) //child
        {

            if (execvp("./temp.out", args) == -1) {
                perror("execvp error temp.out");
                exit(-1);
            }
        } else {
            int i;
            BOOL under5Seconds = FALSE;
            for (i = 0; i < 5 && !under5Seconds; i++) {
                sleep(1);
                if (waitpid(cid, &stat, WNOHANG))
                    under5Seconds = TRUE;
            }
            //printf("under5Seconds is %s\n", under5Seconds ? "TRUE" : "FALSE");
            if (!under5Seconds) {
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
                unlink("./temp.out");
                return;
            }

            close(outputFd);
            close(inputFd);
            dup2(copyStdin, 0);
            dup2(copyStdout, 1);

            //comparing outputs
            char *args1[] = {"./comp.out", "./output.txt",
                             correctOutputFilePath, NULL};
            cid = MyFork(resultsFd, mainDir);
            if (cid == 0) //child running compare
            {
//                printf("comparing output files\n");
//                char *args[] = {"./comp.out", "./output",
//                                correctOutputFilePath, NULL};
                if (execvp("./comp.out", args1) == -1) {
                    perror("Error running comp");
                    exit(110);
                }
            } else //father returning compare result.
            {
                int s;
                //sleep(15);
                wait(&s);
                if (WIFEXITED(s)) {
                    printf("wala exit stauts is %d\n", WEXITSTATUS(s));
                    switch (WEXITSTATUS(s)) {
                        default:
                            *penalties = NULL;
                            return;
                        case 1:
                            (*penalties)->penalty1 = GREAT_JOB;
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
                    unlink("./temp.out");
                    return;
                }
            }
        }
    }
    //there is no c file
    if ((name = FindOnlyDirectoryName(path, resultsFd, mainDir)) != NULL &&
        strcmp(name, "") != 0) //name isn't "" ot NULL
    {
        //only one directory
        if (strlen(name) + strlen(path) > PATH_MAX) {
            perror("PATH TOO LONG");
            *penalties = NULL;
            return;
        }
        strcpy(cPath, path);
        strcat(cPath, "/");
        strcat(cPath, name);
        HandleStudentDir(depth + 1, penalties, cPath, resultsFd, mainDir,
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

void MyWrite(int fd, char *buffer, size_t size, int resultsFd, DIR
*mainDir) {
    printf("writing %s to fd %d\n", buffer, fd);
    if (write(fd, buffer, size) < 0) {
        printf("errno = %d\n", errno);
        switch (errno) {
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

struct dirent *MyReadDir(DIR *dir, int resultsFd, DIR *mainDir) {
    int prevErrorno = errno;
    struct dirent *d = NULL;
    if ((d = readdir(dir)) == NULL && errno != prevErrorno) {
        switch (errno) {
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

BOOL CFileCompiled(char *path, int resultsFd, DIR *mainDir) {
    pid_t cid;
    int stat;

    printf("compiling %s\n", path);
    char *args[] = {"gcc", path, "-o", "./temp.out", NULL};
    if ((cid = MyFork(resultsFd, mainDir)) == 0) //child proccess
    {

        if (execvp("gcc", args) == -1) {
            perror("can't run gcc");
            exit(1);
        }
    }
    //father
    wait(&stat);
    return (WEXITSTATUS(stat) == 0) != 0 ? TRUE : FALSE;
}

pid_t MyFork(int resultsFd, DIR *mainDir) {
    pid_t cid;

    if ((cid = fork()) == -1) {
        switch (errno) {
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
