#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>
#include <errno.h>

#define SIZEOFINPUT 128
#define HOSTNUSRNAME 32

void isValid(int(*f)(char*, size_t), char* argument1)
{
   if( f(argument1, HOSTNUSRNAME) != 0)
   {
       fprintf(stderr, "error: %s\n", strerror(errno));
   }
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

        if (strcspn(wd, "h") == 1)
        {
            wd[4] = '~';
            pwd = &(wd[4]);
        }
        else
        {
            pwd = wd;
        }

        // prints shell
        printf("%s@%s %s> ", username, hostname, pwd);
        fflush(stdout);
        
        // validates input
        if (fgets(input, SIZEOFINPUT, stdin) == NULL)
        {
            fprintf(stderr, "error: %s\n", strerror(errno));
            continue;
        }
        if (strlen(input) <= 1)
        {
            fprintf(stderr, "error: invalid input\n");
            continue;
        }
        // removes newline char because its annoying
        input[strcspn(input, "\n")] = 0;
        
        
        // cd command logic
        if (input[0] == 'c' && input [1] == 'd')
        {
            
            if (input[3] == '\0')
            {
                chdir("/home");
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

        //exits the shell
        if(strcmp(input, "exit") == 0)
        {
            free(input);
            break;
        }

        // forking the parent process
        int f_num = fork();

        // if fork failed
        if (f_num < 0)
        {
            fprintf(stderr, "error: fork failed!");
            free(input);
            exit(1);
        }

        // child process route 
        else if(f_num == 0)
        {
            // parses input
            char* token = strtok(input, " \n");
            int i = 0;
            while (token != NULL)
            {
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
            wait(NULL);
            free(input);
        }
   }
}
