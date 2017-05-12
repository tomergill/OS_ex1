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
typedef enum
{
    GREAT_JOB = 0, NO_C_FILE, COMPILATION_ERROR, TIMEOUT, BAD_OUTPUT,
    SIMILLAR_OUTPUT, MULTIPLE_DIRECTORY
} PENALTY;

typedef enum
{
    FALSE = 0, TRUE = 1
} BOOL;

/*
 * Holds the reason why the student's grade should be subtracted :(
 * and the depth of directories the c file was in.
 * Note: penalty1 could be GREAT_JOB if erverything is good, and
 * wrongDirectoryDepth could be 0 if the file was in main student's folder.
 */
typedef struct
{
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

/******************************************************************************
 * function name: HandleStudentDir.
 *
 * The Input: The depth of sub folders from the student's directory (starts
 * at 0), a pointer to a pointer to a penalties_t struct, a path to the
 * current folder, the file descriptor of the results file, the DIR stream
 * of the main directory, the path for the input file and the path of the
 * correct output to compare to.
 *
 * The output: penalties will hold either a penalty (except WRONG_DIRECTORY),
 * the depth of sub-directories the c-file is in, both of them (BO/SO/GJ + WD)
 * or NULL if there was an error the process didn't exit because of it.
 *
 * The Function operation: The function checks for a c-file in the directory.
 * If there isn't one, it checks if there is a single directory (no more, no
 * less), and calls itself on it (meaning if c-file will be found later it
 * will have WRONG_DIRECTORY), otherwise returns either NO_C_FILE or
 * MULTIPLE_DIRECTORIES (depends on case).
 * If c-file found, it will be compiled (if not returns COMPILATION_ERROR),
 * run with input from the input file under 5 seconds (otherwise returns
 * TIMEOUT) and the output will be compared with the correctOutput file. If
 * same returns GREAT_JOB, similar returns SIMILLAR_OUTPUT and different will
 * return BAD_OUTPUT.
*******************************************************************************/
void HandleStudentDir(int depth, penalties_t **penalties,
                      char *path, int resultsFd, DIR *mainDir,
                      char *inputFilePath, char *correctOutputFilePath);

/******************************************************************************
 * function name: MyWrite
 *
 * The Input: A file descriptor to write to, buffer to write from, how much
 * to write, the file descriptor of the results file and the DIR stream of
 * the main directory.
 *
 * The output: size of bytes is written from buffer to fd.
 *
 * The Function operation: Using write and exiting if failed.
*******************************************************************************/
void MyWrite(int fd, char *buffer, size_t size, int resultsFd, DIR *mainDir);

/*******************************************************************************
 * function name: GotZero
 * The Input: The file descriptor of the results file, a message to write in it
 * and the DIR stream of the main directory.
 * The output: "0,<message>" is written in results.csv.
 * The Function operation: writes 0, and then the message in the results file.
*******************************************************************************/
void GotZero(int resultsFd, char *message, DIR *mainDir);

/******************************************************************************
 * function name: MyReadDir
 *
 * The Input: A DIR stream to read from, the file descriptor of the results
 * file and the DIR stream of the main directory.
 *
 * The output: the next dirent read from dir.
 *
 * The Function operation: Returning the result of readdir or exiting if failed.
*******************************************************************************/
struct dirent *MyReadDir(DIR *dir, int resultsFd, DIR *mainDir);

/******************************************************************************
 * function name: CFileCompiled
 *
 * The Input: A path to a c-file, the file descriptor of the results
 * file and the DIR stream of the main directory.
 *
 * The output: The compiled c-file called 'temp.out' in the current working
 * directory and return value of TRUE, otherwise (if file couldn't be
 * compiled) FALSE is returned.
 *
 * The Function operation: Forks into another process who uses execvp to run
 * gcc on the file. When done the father checks if the compilation was
 * successful.
*******************************************************************************/
BOOL CFileCompiled(char *path, int resultsFd, DIR *mainDir);

/******************************************************************************
 * function name: MyFork
 *
 * The Input: The file descriptor of the results file and the DIR stream of
 * the main directory.
 *
 * The output: The process is forked and returns the rsult of fork().
 *
 * The Function operation: Tries to fork and exit if doesn't work.
*******************************************************************************/
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

/******************************************************************************
 * function name: main
 * The Input: A path to a configuration file, holding 3 rows exactly in that
 * order: a path to a directory holding sub directories of students, a path
 * to an input file and a path to a file contains the correct output to
 * compare with.
 * The output: A file names 'results.csv' which holds on each students the
 * following: name, grade and reasons for the grade.
 * The Function operation: Opens the config file, and then goes over every
 * directory in the folder specified by the path in the first row in the
 * config file. For each directory the function 'HandleStudentDir' is called,
 * which returns the reasons to subtract from the grade. Then main writes it
 * into the results file.
*******************************************************************************/
int main(int argc, char *argv[])
{
    char folderLocation[LINE_SIZE + 1], inputLocation[LINE_SIZE + 1],
            outputLocation[LINE_SIZE + 1];
    int configFd, resultsFd;
    DIR *mainDir = NULL;
    struct dirent *curDirent = NULL;
    penalties_t p, *penalties;
    char path[PATH_MAX + 1];
    int grade;

    if (argc < 2)
    {
        perror("Bad Usage");
        return -1;
    }

    /*
     * Open the config file.
     */
    if ((configFd = open(argv[1], O_RDONLY)) == -1)
    {
        char *err;
        if (errno == ENOENT)
            err = "Error : First file is NULL pointer ";
        else if (errno == EACCES)
            err = "Error : 'open' was unable to open the first file ";
        else
            err = "Error : unknown error opening the first file ";
        perror(err);
        return -1;
    }

    /*
     * Reading the 3 lines from the config file.
     */
    ReadLineFromFile(configFd, folderLocation);
    ReadLineFromFile(configFd, inputLocation);
    ReadLineFromFile(configFd, outputLocation);
    if (close(configFd) == -1)
    {
        perror("error closing configuration file");
        exit(-1);
    }

    /*
     * Creating the results file for writing.
     */
    if ((resultsFd = open("results.csv", O_WRONLY | O_CREAT | O_TRUNC,
                          S_IRUSR | S_IRGRP | S_IWUSR)) == -1)
    {
        char *err;
        if (errno == EACCES)
            err = "Error : 'open' was unable to open the results file ";
        else
            err = "Error : unknown error opening the results file ";
        perror(err);
        return -1;
    }

    MyOpenDir(&mainDir, folderLocation, resultsFd, NULL);

    /*
     * Reading each dirent from teh directory stream and handling the
     * ones that are directories.
     * Note: counter starts with value 1 (see above).
     */
    while ((curDirent = MyReadDir(mainDir, resultsFd, mainDir)) != NULL)
    {
        if (IS_HIDDEN_NAV_DIRS(curDirent->d_name))
            continue;

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

        if (IsDir(path, resultsFd, mainDir))
        {
            //initializing penalties before calling HandleStudentDir
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
                    resultsFd, mainDir); //write name and ,
            MyWrite(resultsFd, ",", strlen(","), resultsFd, mainDir);

            char *pnlty = NULL;
            BOOL isWrongDirectory = penalties->wrongDirectoryDepth > 0 ? TRUE
                                                                       : FALSE;
            switch (penalties->penalty1)
            {
                case NO_C_FILE:
                    GotZero(resultsFd, "NO_C_FILE\n", mainDir);
                    continue;
                case MULTIPLE_DIRECTORY:
                    GotZero(resultsFd, "MULTIPLE_DIRECTORIES\n", mainDir);
                    continue;
                case COMPILATION_ERROR:
                    GotZero(resultsFd, "COMPILE_ERROR\n", mainDir);
                    continue;
                case TIMEOUT:
                    GotZero(resultsFd, "TIMEOUT\n", mainDir);
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
            } //end of switch

            grade -= 10 * penalties->wrongDirectoryDepth;
            grade = grade < 0 ? 0 : grade;

            char gradeString[4];
            //writing grade
            sprintf(gradeString, "%d,", grade);
            MyWrite(resultsFd, gradeString, strlen(gradeString), resultsFd,
                    mainDir);

            if (!penalties->penalty1 == GREAT_JOB)
            {
                MyWrite(resultsFd, pnlty, strlen(pnlty), resultsFd, mainDir);
                if (isWrongDirectory)
                    MyWrite(resultsFd, ",WRONG_DIRECTORY",
                            strlen(",WRONG_DIRECTORY"), resultsFd, mainDir);
            }
            else //BAD_OUTPUT or SIMILLAR_OUTPUT
            {
                if (isWrongDirectory)
                    MyWrite(resultsFd, "WRONG_DIRECTORY,",
                            strlen(",WRONG_DIRECTORY"), resultsFd, mainDir);
                MyWrite(resultsFd, pnlty, strlen(pnlty), resultsFd, mainDir);
            }
            MyWrite(resultsFd, "\n", strlen("\n"), resultsFd, mainDir);
        } //end of if (IsDir(path, resultsFd, mainDir))
    } //end of while ((curDirent = MyReadDir(mainDir, resultsFd, mainDir))
    // != NULL)

    if (closedir(mainDir) == -1)
        perror("error closing main directory");
    if (close(resultsFd) == -1)
        perror("error closing results.cvs file");
    return 0;
}

/*******************************************************************************
 * function name: GotZero
 * The Input: The file descriptor of the results file, a message to write in it
 * and the DIR stream of the main directory.
 * The output: "0,<message>" is written in results.csv.
 * The Function operation: writes 0, and then the message in the results file.
*******************************************************************************/
void GotZero(int resultsFd, char *message, DIR *mainDir)
{
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
int ReadLineFromFile(int fd, char *line)
{
    char c;
    int i = 0;

    for (; i < LINE_SIZE; ++i)
    {
        int status = (int) read(fd, &c, sizeof(char));
        if (status == 0)
        {
            line[i] = '\0';
            return i + 1;
        }
        else if (status == -1)
        {
            if (errno == EBADF)
                perror("Error : config file is not a valid file descriptor or"
                               " is not open for reading");
            else if (errno == EFAULT)
                perror("Error : char c is outside your accessible address "
                               "space");
            else if (errno == EIO)
                perror("Error : I/O error while reading the config file");
            else if (errno == EISDIR)
                perror("Error : config file descriptor points to a directory");
            else
                perror("Error : unknown error while reading config file");
            exit(-1);
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

/******************************************************************************
 * function name: isDir
 * The Input: A path to a directory, the file descriptor of the results file
 * and the DIR stream of the main directory.
 * The output: TRUE if path points to a directory, FALSE otherwise.
 * The Function operation: Opens the path as struct stat and returns the value
 * of S_ISDIR.
*******************************************************************************/
BOOL IsDir(char *path, int resultsFd, DIR *mainDir)
{
    struct stat stat1;
    if ((stat(path, &stat1)) == -1) //getting details on name
    {
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

        if (closedir(mainDir) == -1 || close(resultsFd) == -1)
            perror("error closing main directory or results file");
        exit(-1);
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
void MyOpenDir(DIR **dir, char *path, int resultsFd, DIR *mainDir)
{
    if ((*dir = opendir(path)) == NULL) //opening dir
    {
        //failed
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

        if ((mainDir != NULL && closedir(mainDir)) == -1 ||
                close(resultsFd) == -1)
            perror("error closing main directory or results file");
        exit(-1);
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
char *FindCFileInLevel(char *path, int resultsFd, DIR *mainDir)
{
    DIR *dir;
    struct dirent *d;
    MyOpenDir(&dir, path, resultsFd, mainDir);
    while ((d = MyReadDir(dir, resultsFd, mainDir)) != NULL)
    {
        if (IS_HIDDEN_NAV_DIRS(d->d_name))
            continue;
        if (IS_C_FILE(d->d_name, strlen(d->d_name)))
        {
             if (closedir(dir) == -1)
                 perror("error closing directory in 'FindCFileInLevel'");
            return d->d_name;
        }
    }
    if (closedir(dir) == -1)
        perror("error closing directory in 'FindCFileInLevel'");
    return NULL;
}

/******************************************************************************
 * function name: FindOnlyDirectoryName
 *
 * The Input: A path to a student's directory, the file descriptor of the
 * results file and the DIR stream of the main directory.
 *
 * The output: the name of the only directory if exists, otherwise NULL if
 * there are two or more directories or "" if there is no directory there.
 *
 * The Function operation: Opens the path as DIR stream, then loop through all
 * files in the directory looking for a directory, if not found return "". If
 * found, searches for another directory. If found another returns NULL,
 * otherwise the only directory name.
*******************************************************************************/
char *FindOnlyDirectoryName(char *path, int resultsFd, DIR *mainDir)
{
    DIR *dir;
    struct dirent *d, *temp;
    char dirPath[PATH_MAX + 1], tempPath[PATH_MAX + 1];
//    int length = strlen(path);


    MyOpenDir(&dir, path, resultsFd, mainDir);

    //find a directory
    while ((d = MyReadDir(dir, resultsFd, mainDir)) != NULL)
    {
        if (IS_HIDDEN_NAV_DIRS(d->d_name)) //checks if dir is . or ..
            continue;
        if (strlen(d->d_name) + strlen(path) > PATH_MAX)
        {
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
    {
        if (closedir(dir) == -1)
            perror("error closing directory in 'FindOnlyDirectoryName'");
        return "";  //indicates there is no directory
    }

    //search if there is another dir
    while ((temp = MyReadDir(dir, resultsFd, mainDir)) != NULL)
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
        if (IsDir(tempPath, resultsFd, mainDir))
        {
            if (closedir(dir) == -1)
                perror("error closing directory in 'FindOnlyDirectoryName'");
            return NULL; //indicates there are at least 2 directories or more
        }
    } //end while

    if (closedir(dir) == -1)
        perror("error closing directory in 'FindOnlyDirectoryName'");
    return d->d_name;
}

/******************************************************************************
 * function name: HandleStudentDir.
 *
 * The Input: The depth of sub folders from the student's directory (starts
 * at 0), a pointer to a pointer to a penalties_t struct, a path to the
 * current folder, the file descriptor of the results file, the DIR stream
 * of the main directory, the path for the input file and the path of the
 * correct output to compare to.
 *
 * The output: penalties will hold either a penalty (except WRONG_DIRECTORY),
 * the depth of sub-directories the c-file is in, both of them (BO/SO/GJ + WD)
 * or NULL if there was an error the process didn't exit because of it.
 *
 * The Function operation: The function checks for a c-file in the directory.
 * If there isn't one, it checks if there is a single directory (no more, no
 * less), and calls itself on it (meaning if c-file will be found later it
 * will have WRONG_DIRECTORY), otherwise returns either NO_C_FILE or
 * MULTIPLE_DIRECTORIES (depends on case).
 * If c-file found, it will be compiled (if not returns COMPILATION_ERROR),
 * run with input from the input file under 5 seconds (otherwise returns
 * TIMEOUT) and the output will be compared with the correctOutput file. If
 * same returns GREAT_JOB, similar returns SIMILLAR_OUTPUT and different will
 * return BAD_OUTPUT.
*******************************************************************************/
 void HandleStudentDir(int depth, penalties_t **penalties,
                      char *path, int resultsFd, DIR *mainDir,
                      char *inputFilePath, char *correctOutputFilePath)
{
    char cPath[PATH_MAX + 1], *name;
    if ((name = FindCFileInLevel(path, resultsFd, mainDir)) != NULL)
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

        /*
         * Trying to compile, it will be called 'temp.out'
         */
        if (!CFileCompiled(cPath, resultsFd, mainDir))
        {
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
        if (inputFd == -1 || outputFd == -1) //open failed
        {
            perror("can't open input/output file");
            *penalties = NULL;
            return;
        }

        int copyStdout = dup(1), copyStdin = dup(0);

        if (copyStdin == -1 || copyStdout == -1)
        {
            perror("error copying stdin or stdout");
            exit(-1);
        }

        //using input file as STDIN and using output file as STDOUT
        if (dup2(inputFd, 0) == -1 || dup2(outputFd, 1) == -1)
        {
            perror("error replacing stdin or stdout");
            exit(-1);
        }

        //running and checking for timeout
        int stat;
        char *args[] = {"./temp.out", NULL};
        pid_t cid = MyFork(resultsFd, mainDir);
        if (cid == 0) //child
        {

            if (execvp("./temp.out", args) == -1)
            {
                perror("execvp error temp.out");
                exit(-1);
            }
        }
        else
        {
            int i;
            BOOL under5Seconds = FALSE;
            for (i = 0; i < 5 && !under5Seconds; i++)
            {
                sleep(1);
                int temp = waitpid(cid, &stat, WNOHANG);
                if (temp > 0)
                    under5Seconds = TRUE;
                else if (temp == -1)
                {
                    perror("error waiting for child process");
                    exit(-1);
                }
            }

            if (!under5Seconds) //timeout
            {
                if (kill(cid, SIGKILL) == -1)
                {
                    perror("error killing on timeout");
                    exit(-1);
                }
                if (wait(&stat) == -1)
                {
                    perror("error waiting");
                    exit(-1);
                }
                (*penalties)->penalty1 = TIMEOUT;
                (*penalties)->wrongDirectoryDepth = 0;

                if (close(outputFd) == -1 || close(inputFd) == -1 ||
                        dup2(copyStdin, 0) == -1 || dup2(copyStdout, 1) == -1 ||
                        unlink("./output.txt") == -1 || unlink("./temp.out") == -1)
                {
                    perror("error closing up after timeout");
                    exit(-1);
                }
                return;
            }

            if (close(outputFd) == -1 || close(inputFd) == -1 ||
                dup2(copyStdin, 0) == -1 || dup2(copyStdout, 1) == -1)
            {
                perror("error closing files or returning stdin and stdout");
                exit(-1);
            }

            //comparing outputs
            char *args1[] = {"./comp.out", "./output.txt",
                             correctOutputFilePath, NULL};
            cid = MyFork(resultsFd, mainDir);
            if (cid == 0) //child running compare
            {
                if (execvp("./comp.out", args1) == -1)
                {
                    perror("Error running comp");
                    exit(-1);
                }
            }
            else //father returning compare result.
            {
                int s;
                //sleep(15);
                if (wait(&s) == -1)
                {
                    perror("error waiting for comp.out");
                    exit(-1);
                }
                if (WIFEXITED(s)) //exited normally
                {
                    switch (WEXITSTATUS(s)) //exit status of comp.out
                    {
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
                    if (unlink("./output.txt") == -1 ||
                            unlink("./temp.out") == -1)
                    {
                        perror("error deleting 'output.txt' ot 'temp.out' : a");
                        exit(-1);
                    }
                    return;
                }
            } //end of else (cid != 0)
        } //end of else
    } // end of if ((name = FindCFileInLevel(path, resultsFd, mainDir)) != NULL)

    //there is no c file
    if ((name = FindOnlyDirectoryName(path, resultsFd, mainDir)) != NULL &&
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
        //calling itself on the next dir
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

/******************************************************************************
 * function name: MyWrite
 *
 * The Input: A file descriptor to write to, buffer to write from, how much
 * to write, the file descriptor of the results file and the DIR stream of
 * the main directory.
 *
 * The output: size of bytes is written from buffer to fd.
 *
 * The Function operation: Using write and exiting if failed.
*******************************************************************************/
void MyWrite(int fd, char *buffer, size_t size, int resultsFd, DIR *mainDir)
{
    if (write(fd, buffer, size) < 0)
    {
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
        if (closedir(mainDir) == -1 || close(resultsFd) == -1)
            perror("error closing main directory or results file");
        if (fd != resultsFd)
            if (close(fd) == -1)
                perror("error closing file writing to");
        exit(-1);
    } //end of if (write(fd, buffer, size) < 0)
}

/******************************************************************************
 * function name: MyReadDir
 *
 * The Input: A DIR stream to read from, the file descriptor of the results
 * file and the DIR stream of the main directory.
 *
 * The output: the next dirent read from dir.
 *
 * The Function operation: Returning the result of readdir or exiting if failed.
*******************************************************************************/
struct dirent *MyReadDir(DIR *dir, int resultsFd, DIR *mainDir)
{
    int prevErrorno = errno;
    struct dirent *d = NULL;
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
        if (closedir(mainDir) == -1 || close(resultsFd) == -1)
            perror("error closing main directory or results file");
        if (dir != mainDir)
            if (closedir(dir) == -1)
                perror("error closing directory read from");
        exit(-1);
    } //end of if ((d = readdir(dir)) == NULL && errno != prevErrorno)
    return d;
}

/******************************************************************************
 * function name: CFileCompiled
 *
 * The Input: A path to a c-file, the file descriptor of the results
 * file and the DIR stream of the main directory.
 *
 * The output: The compiled c-file called 'temp.out' in the current working
 * directory and return value of TRUE, otherwise (if file couldn't be
 * compiled) FALSE is returned.
 *
 * The Function operation: Forks into another process who uses execvp to run
 * gcc on the file. When done the father checks if the compilation was
 * successful.
*******************************************************************************/
BOOL CFileCompiled(char *path, int resultsFd, DIR *mainDir)
{
    int stat;

    char *args[] = {"gcc", path, "-o", "./temp.out", NULL};
    if ((MyFork(resultsFd, mainDir)) == 0) //child proccess
    {

        if (execvp("gcc", args) == -1)
        {
            perror("can't run gcc");
            exit(-1);
        }
    }
    //father
    if (wait(&stat) == -1)
    {
        perror("error waiting to gcc");
        if (closedir(mainDir) == -1 || close(resultsFd) == -1)
            perror("error closing main directory or results file");
        exit(-1);
    }
    return (WEXITSTATUS(stat) == 0) != 0 ? TRUE : FALSE;
}

/******************************************************************************
 * function name: MyFork
 *
 * The Input: The file descriptor of the results file and the DIR stream of
 * the main directory.
 *
 * The output: The process is forked and returns the rsult of fork().
 *
 * The Function operation: Tries to fork and exit if doesn't work.
*******************************************************************************/
pid_t MyFork(int resultsFd, DIR *mainDir)
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
        if (closedir(mainDir) == -1 || close(resultsFd) == -1)
            perror("error closing main directory or results file");
        exit(-1);
    }
    return cid;
}
