#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

int main(int argc, char** argv)
{
    while (1)
    {
        printf(">> ");
        fflush(stdout);
        int f_num = fork(); 
        if(f_num < 0){
            return 0;
        }
        else if(f_num == 0)
        {
            char* input = calloc(64, 1);
            if (fgets(input, 64, stdin) == NULL)
            {
                printf("Invalid input!\n");
                exit(1);
            }
            fflush(stdin);
            char* args[64];
            char* piece = strtok(input, " \n");
            
            int i = 0;
            while (piece != NULL){

                args[i] = piece;
                i++;
                piece = strtok(NULL, " \n"); 
            }
            args[i] = NULL;
            execvp(args[0], args); 
        }
        else{
            wait(NULL);
        }
        
    }
   
}
