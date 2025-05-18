#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <signal.h>

#define MAX_LINE 80

int number_of_args;
int fdi, fdo;
char *args[MAX_LINE / 2 + 1];
int SaveStdin, SaveStdout;

void Allocate_Memory(char *args_array[])
{
    for (int i = 0; i < MAX_LINE / 2 + 1; i++)
    {
        args_array[i] = (char *)malloc(sizeof(char) * MAX_LINE / 2);
    }
}

void EnterCommand(char command[], char history_command[][MAX_LINE], int count_HF, int *index)
{
    int pos = 0;
    char ch;

    printf("it007sh>");
    fflush(stdout);

    while (1)
    {
        ch = getchar();
        if (ch == 27) // ESC
        {
            if (getchar() == 91)
            {
                char dir = getchar();
                if (dir == 'UP') // UP
                {
                    if (*index > 0)
                        (*index)--;
                    printf("\33[2K\r");
                    printf("it007sh>%s", history_command[*index]);
                    fflush(stdout);
                    strcpy(command, history_command[*index]);
                    pos = strlen(command);
                }
                else if (dir == 'DOWN') // DOWN
                {
                    if (*index < count_HF - 1)
                        (*index)++;
                    printf("\33[2K\r");
                    printf("it007sh>%s", history_command[*index]);
                    fflush(stdout);
                    strcpy(command, history_command[*index]);
                    pos = strlen(command);
                }
            }
        }
        else if (ch == '\n')
        {
            command[pos] = '\0';
            printf("\n");
            break;
        }
        else
        {
            if (pos < MAX_LINE - 1)
            {
                command[pos++] = ch;
                putchar(ch);
                *index = count_HF; // reset index vì đang gõ lệnh mới
            }
        }
    }
}


void Tokernizer(char *tokens[], char *source, char *delim, int *num_of_words)
{
    char *p = strtok(source, delim);
    int count = 0;
    while (p != NULL)
    {
        strcpy(tokens[count], p);
        count++;
        p = strtok(NULL, delim);
    }
    *num_of_words = count;
}

void control_sig(int sign)
{
    printf("\n");
    fflush(stdout);
}

void redirect_output()
{
    SaveStdout = dup(STDOUT_FILENO);
    fdo = open(args[number_of_args - 1], O_CREAT | O_WRONLY | O_TRUNC, 0644);
    dup2(fdo, STDOUT_FILENO);
    free(args[number_of_args - 2]);
    args[number_of_args - 2] = NULL;
}

void redirect_input()
{
    SaveStdin = dup(STDIN_FILENO);
    fdi = open(args[number_of_args - 1], O_RDONLY);
    dup2(fdi, STDIN_FILENO);
    free(args[number_of_args - 2]);
    args[number_of_args - 2] = NULL;
}

void RestoreOut()
{
    close(fdo);
    dup2(SaveStdout, STDOUT_FILENO);
    close(SaveStdout);
}

void RestoreIn()
{
    close(fdi);
    dup2(SaveStdin, STDIN_FILENO);
    close(SaveStdin);
}

void Execute_pile(char *parsed[], char *parsedpipe[])
{
    pid_t PID1, PID2;
    int fd[2];

    PID1 = fork();
    if (PID1 < 0)
    {
        printf("\nLỗi không thể tạo tiến trình con");
        return;
    }

    if (PID1 == 0)
    {
        if (pipe(fd) < 0)
        {
            printf("\nTạo ống thất bại");
            return;
        }

        PID2 = fork();
        if (PID2 < 0)
        {
            printf("\nLỗi không thể tạo tiến trình con");
            exit(0);
        }

        if (PID2 == 0)
        {
            close(fd[0]);
            dup2(fd[1], STDOUT_FILENO);
            if (execvp(parsed[0], parsed) < 0)
            {
                printf("\nKhông thể thực thi");
                exit(0);
            }
        }
        else
        {
            wait(NULL);
            close(fd[1]);
            dup2(fd[0], STDIN_FILENO);
            if (execvp(parsedpipe[0], parsedpipe) < 0)
            {
                printf("\nKhông thể thực thi");
                exit(0);
            }
        }
    }
    else
    {
        wait(NULL);
    }
}

int Find_pile_char(char *cmd, char *cmdpiped[])
{
    for (int i = 0; i < 2; i++)
    {
        cmdpiped[i] = strsep(&cmd, "|");
        if (cmdpiped[i] == NULL)
            break;
    }
    return (cmdpiped[1] != NULL);
}

void parseCommandLine(char *cmd, char *parsedArg[])
{
    int i = 0;
    char *token;
    while ((token = strsep(&cmd, " ")) != NULL)
    {
        if (strlen(token) > 0)
        {
            parsedArg[i++] = token;
        }
    }
    parsedArg[i] = NULL;
}

int ExecuteString(char *cmd, char *parsed[], char *parsedpipe[])
{
    char *cmdpiped[2];
    int piped = Find_pile_char(cmd, cmdpiped);
    if (piped)
    {
        parseCommandLine(cmdpiped[0], parsed);
        parseCommandLine(cmdpiped[1], parsedpipe);
    }
    return piped;
}

int main(void)
{
    int count_HF = 0;
    int should_run = 1;
    char command[MAX_LINE];
    char history_command[MAX_LINE][MAX_LINE];
    char *First_Command[MAX_LINE / 2 + 1];
    char *Second_command[MAX_LINE / 2 + 1];
    int iPileExe = 0;
    pid_t pid;
    int iRedirectOut = 0, iRedirectIn = 0;

    signal(SIGINT, control_sig);

    while (should_run)
    {
        int history_index = count_HF;
        do {
             EnterCommand(command, history_command, count_HF, &history_index);
        } while (command[0] == 0);


        if (strcmp(command, "HF") == 0)
        {
            if (count_HF == 0)
            {
                printf("NULL\n");
                continue;
            }
            else
            {
                for (int i = 0; i < count_HF; i++)
                {
                    printf("%s\n", history_command[i]);
                }
                continue;
            }
        }
        else
        {
            strcpy(history_command[count_HF++], command);
        }

        iPileExe = ExecuteString(command, First_Command, Second_command);

        if (iPileExe == 0)
        {
            Allocate_Memory(args);
            strcpy(command, history_command[count_HF - 1]);
            Tokernizer(args, command, " ", &number_of_args);
            free(args[number_of_args]);
            args[number_of_args] = NULL;

            if (strcmp(args[0], "exit") == 0)
                break;

            else if (strcmp(args[0], "~") == 0)
            {
                char *homedir = getenv("HOME");
                printf("Home : %s\n", homedir);
            }
            else if (strcmp(args[0], "cd") == 0)
            {
                char dir[MAX_LINE];
                strcpy(dir, args[1]);
                if (strcmp(dir, "~") == 0)
                {
                    strcpy(dir, getenv("HOME"));
                }
                chdir(dir);
                printf("Thư mục hiện tại : ");
                getcwd(dir, sizeof(dir));
                printf("%s\n", dir);
            }
            else
            {
                int isBackground = strcmp(args[number_of_args - 1], "&") == 0 ? 1 : 0;
                if (isBackground)
                {
                    free(args[number_of_args - 1]);
                    args[number_of_args - 1] = NULL;
                }

                if ((number_of_args > 1) && strcmp(args[number_of_args - 2], ">") == 0)
                {
                    redirect_output();
                    iRedirectOut = 1;
                }
                else if ((number_of_args > 1) && strcmp(args[number_of_args - 2], "<") == 0)
                {
                    redirect_input();
                    iRedirectIn = 1;
                }

                pid = fork();
                if (pid < 0)
                {
                    fprintf(stderr, "Lỗi không thể tạo tiến trình con\n");
                    return -1;
                }

                if (pid == 0)
                {
                    int ret = execvp(args[0], args);
                    if (ret == -1)
                    {
                        printf("Lệnh không hợp lệ\n");
                    }
                    exit(ret);
                }
                else
                {
                    if (!isBackground)
                    {
                        while (wait(NULL) != pid)
                            ;
                        if (iRedirectOut)
                        {
                            RestoreOut();
                            iRedirectOut = 0;
                        }
                        if (iRedirectIn)
                        {
                            RestoreIn();
                            iRedirectIn = 0;
                        }
                    }
                }
            }
        }
        else
        {
            Execute_pile(First_Command, Second_command);
        }
    }
    return 0;}
