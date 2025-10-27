#include <linux/limits.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>
#include <errno.h>
#include <termios.h>

#define SIZEOFINPUT 128
#define HOSTNUSRNAME 32
#define HISTORYCOUNT 12
static char history[HISTORYCOUNT][SIZEOFINPUT];
static int history_count = 0;
static int history_ptr = 0;

// checks if function was succesful
void isValid(int(*f)(char*, size_t), char* argument1)
{
   if(f(argument1, HOSTNUSRNAME) != 0)
   {
       fprintf(stderr, "error: %s\n", strerror(errno));
   }
}

// sets up or removes char by char input
void setUpTerminal(int operation)
{
    struct termios old_settings, new_settings;
    tcgetattr(STDIN_FILENO, &old_settings);
    new_settings = old_settings;
    if (operation == 1)
    {
        tcsetattr(STDIN_FILENO, TCSAFLUSH, &old_settings);
    }
    else{
        new_settings.c_lflag &= ~(ICANON | ECHO);
        tcsetattr(STDIN_FILENO, TCSAFLUSH, &new_settings);
    } 
}

// appending history logic
void addHistory(char* args)
{

    if (history_count < HISTORYCOUNT - 1)
    {
        strcpy(history[history_count], args);
        history_count++;
        history_ptr = history_count;
    }
    else
    {
        for(int i = 0; i < history_count; i++)
        {
            strcpy(history[i], history[i + 1]);
        }
        strcpy(history[history_count], args);
        history_ptr = history_count;
    }
}

// querying history logic
int getHistory(char direction, char* input, int* pos) {
    int new_pos = *pos;
    int i;

    if (direction == 'u' && history_ptr > 0) {
        // erase current line
        for (i = 0; i < new_pos; i++)
            write(STDOUT_FILENO, "\b \b", 3);

        history_ptr--;
        strcpy(input, history[history_ptr]);
        new_pos = strlen(history[history_ptr]);
        write(STDOUT_FILENO, input, new_pos);
        *pos = new_pos;
        return new_pos;
    }

    else if (direction == 'd' && history_ptr < history_count - 1) {
        // erase current line
        for (i = 0; i < new_pos; i++)
            write(STDOUT_FILENO, "\b \b", 3);

        history_ptr++;
        strcpy(input, history[history_ptr]);
        new_pos = strlen(history[history_ptr]);
        write(STDOUT_FILENO, input, new_pos);
        *pos = new_pos;
        return new_pos;
    }

    return *pos; 
}

void pipeLogic(int pipe_c, char* input)
{
    if (pipe_c == 1)
    {
        char* left_str, *right_str;
        int fd[2];
        pipe(fd);
        left_str = strtok(input, "|");
        right_str = strtok(NULL, "|");
        while(*left_str == ' ')left_str++;
        while(*right_str == ' ')right_str++;
        int pid1 = fork();
        if (pid1 == 0)
        {
            char* argv[32];
            dup2(fd[1], STDOUT_FILENO);
            close(fd[1]);
            close(fd[0]);
            char* token = strtok(left_str, " ");
            int i = 0;
            while (token != NULL)
            {
                argv[i++] = token;
                token = strtok(NULL, " ");
            }
            argv[i] = NULL;
            execvp(argv[0], argv);
            fprintf(stderr, "error: %s\n", strerror(errno));
            exit(1);
        }
        int pid2 = fork();
        if (pid2 == 0)
        {
            char* argv[32];
            dup2(fd[0], STDIN_FILENO);
            close(fd[0]);
            close(fd[1]);
            char* token = strtok(right_str, " ");
            int i = 0;
            while (token != NULL)
            {
                argv[i++] = token;
                token = strtok(NULL, " ");
            }
            argv[i] = NULL;
            execvp(*argv, argv);
            fprintf(stderr, "error: %s\n", strerror(errno));
            exit(2);
        }
        if (pid1 != 0 && pid2 != 0)
        {
            close(fd[1]);
            close(fd[0]);
            wait(NULL);
            wait(NULL);
        }
    }
    return;
}
int main()
{
   while(1)
   {
        char* input = calloc(SIZEOFINPUT, sizeof(char));
        char* args[SIZEOFINPUT]; 
        char hostname[HOSTNUSRNAME];
        char username[HOSTNUSRNAME];
        char wd [SIZEOFINPUT];
        char* pwd;
        
        //validating
        isValid(&gethostname, hostname);
        isValid(&getlogin_r, username);
        if (getcwd(wd, sizeof(wd)) == NULL)
        {
             fprintf(stderr, "error: %s\n", strerror(errno));
        }

        // home shell check
        if (strcspn(wd, "h") == 1)
        {
            wd[4 + strlen(username) + 1] = '~';
            pwd = &(wd[4 + strlen(username) + 1]);
        }
        else
        {
            pwd = wd;
        }

        // prints shell
        printf("%s@%s %s> ", username, hostname, pwd);
        fflush(stdout);
        
        // setups up line by line char interpreting
        setUpTerminal(0);
        int pos = 0;
        char c;
        int esc_state = 0;
        int pipeCount = 0;

        while (1)
        { 
            read(STDIN_FILENO, &c, sizeof(char));

            if (esc_state == 0)
            {
                // detects esc char (\033)
                if(c == 27)
                {
                    esc_state = 1;
                }
                
                // detects enter keybind (\n) on some terminals enter gets detected as \r because of ICANON
                else if (c == '\n' || c == '\r')
                {
                    input[pos] = '\0';
                    write(STDOUT_FILENO, "\n", sizeof(char));
                    break;
                }
                // detects backspace
                else if(c == 127)
                {
                    if(pos > 0)
                    {
                        pos--;
                        input[pos] = '\0';
                        write(STDOUT_FILENO, "\b \b", 3);
                    }
                }
                else if(c == '|')
                {
                    pipeCount++;
                    input[pos] = c;
                    pos++;
                    write(STDOUT_FILENO, &c, sizeof(char));
                }
                else
                {
                    input[pos] = c;
                    pos++;
                    write(STDOUT_FILENO, &c, sizeof(char));
                }

            }
            // detects [ char
            else if (esc_state == 1)
            {
                
                if (c == 91)
                {
                    esc_state = 2;
                }
            }
            else if (esc_state == 2)
            {
                esc_state = 0;
                // detects up arrow (A)
                if (c == 65)
                {
                    pos = getHistory('u', input, &pos);
                }
                // detects down arrow (B)
                else if (c == 66)
                {
                    pos = getHistory('d', input, &pos);
                }
            }
        }
        setUpTerminal(1);
        fflush(stdin);
        fflush(stdout);

        // validates input
        if (strlen(input) <= 1)
        {
            fprintf(stderr, "error: invalid input\n");
            continue;
        }
         
        //exits the shell
        if(strcmp(input, "exit") == 0)
        {
            free(input);
            break;
        }

        // cd command logic
        if (input[0] == 'c' && input [1] == 'd')
        {
            
            if (input[2] == '\0')
            {
                char cd_home[HOSTNUSRNAME];
                snprintf(cd_home, HOSTNUSRNAME, "/home/%s", username);
                chdir(cd_home);
            }
            else
            {
                char* dir = strtok(input, " \n");
                dir = strtok(NULL, " \n");
                if (chdir(dir) != 0)
                {
                    fprintf(stderr, "error: %s\n", strerror(errno));
                }

            }
            
            free(input);
            continue;
        }

        

        // forking the parent process
        int f_num;
        if (pipeCount < 1)
        {
            f_num = fork();
        }
        

        // if fork failed
        if (f_num < 0)
        {
            fprintf(stderr, "error: fork failed!");
            free(input);
            exit(1);
        }

        // child process route 
        else if (f_num == 0)
        {
            if (pipeCount >= 1)
            {
                continue;
            }

            // parses input
            char* token = strtok(input, " \n");
            int pipeCount = 0; 
            int i = 0;
            printf("%s", token);
            while (token != NULL)
            {
                if(*token == '|')
                {
                    pipeCount++;
                }
                args[i] = token;
                i++;
                token = strtok(NULL, " \n");
            }
            args[i] = NULL;
            
            if (execvp(args[0], args) == -1){
                free(input);
                fprintf(stderr, "error: %s\n", strerror(errno));
            }
            
        }

        // parent process route
        else
        {  
            
            if (pipeCount >= 1)
            {
                pipeLogic(pipeCount, input);
            }
            else
            {
                wait(NULL);
            }
            addHistory(input);
            free(input);
        }
   }
   
}
