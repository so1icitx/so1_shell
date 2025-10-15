#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>


int main()
{
   while(1)
   {
        
        char* input = calloc(256, sizeof(char));
        char* args[256];
        char hostname[64];
        char username[64];
        char wd [128];
        char* pwd;
        
        gethostname(hostname, sizeof(hostname));
        getlogin_r(username, sizeof(username));
        getcwd(wd, sizeof(wd));
        if (strcspn(wd, "h") == 1)
        {
            wd[4] = '~';
            pwd = &(wd[4]);
        }
        printf("%s@%s %s> ", username, hostname, pwd);
        fflush(stdout);
        

        if (fgets(input, 128, stdin) == NULL)
        {
            fprintf(stderr, "error: invalid input\n");
            continue;
        }
        
        input[strcspn(input, "\n")] = 0;
        if (strlen(input) <= 1)
        {
            fprintf(stderr, "error: invalid input\n");
            continue;
        }

        if(strcmp(input, "exit") == 0)
        {
            free(input);
            break;
        }
        // forking a child proccess + checking its result
        int fk_num = fork();

        // if fork failed
        if (fk_num < 0)
        {
            fprintf(stderr, "error: fork failed!");
            free(input);
            exit(1);
        }

        // if child forking was successful
        else if(fk_num == 0)
        {
            char* token = strtok(input, " \n");
            int i = 0;
            while (token != NULL)
            {
                args[i] = token;
                i++;
                token = strtok(NULL, " \n");
            }
            args[i] = NULL;

            execvp(args[0], args);
            break;
        }

        // if parent
        else
        {
            wait(NULL);
        }
   }
}
