/***************************
 *EEL5934 ADV.SYSTEM.PROG
 *Assignment1
 *Juncheng Gu 5919-0572
 *Jan 19, 2015
 **************************/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

#define COMMAND_LINE_LENGTH 200
#define COMMAND_LENGTH 10
#define MAX_NUM_ARGS 10

/* Breaks the string pointed by str into words and stores them in the arg array*/
int getArguments(char *,char *[]);

/* Reads a line and discards the end of line character */
int getLine(char *str,int max);

char* splitCommand(char *, char);

/* Simulates a shell, i.e., gets command names and executes them */
int main(int argc, char *argv[])
{

  char commandLine[COMMAND_LINE_LENGTH+1]; 
  char *arguments[MAX_NUM_ARGS];
  char *arguments1[MAX_NUM_ARGS];
  char *arguments2[MAX_NUM_ARGS];
  char *arguments3[MAX_NUM_ARGS];

  
  printf("Welcome to Extremely Simple Shell\n");

  int exit= 0;
  do { 
      int status;
      int i = 0;
      char *temp, *index1=NULL, *index2=NULL, *index3=NULL;
      int fd[2];
      if (pipe(fd) == -1){
          continue;
      }      
     
      /* Prints the command prompt */
      printf("\n$ ");
      
      /* Reads the command line and stores it in array commandLine */
      getLine(commandLine,COMMAND_LINE_LENGTH+1);

      /* The user did not enter any commands */
      if (!strcmp(commandLine,""))
         continue;

      index1 = (char*)malloc(sizeof(char)*(strlen(commandLine)+1));
      strcpy(index1, commandLine);
      index2 = splitCommand(index1, '|');  
      if (index2 == NULL){
          index3 = splitCommand(index1, '>');
      }else{ 
          index3 = splitCommand(index2, '>');
      }

      /* Breaks the command line into arguments and stores the arguments in arguments array */
      int numArgs = getArguments(commandLine,arguments);
      int numArgs1 = getArguments(index1, arguments1);
      int numArgs2, numArgs3;
      if (index2 != NULL){
          numArgs2 = getArguments(index2, arguments2); 
      }     
      if (index3 != NULL){
          numArgs3 = getArguments(index3, arguments3);
      }
      /* Terminates the program when the user types exit or quit */
      if (numArgs == 0){
          continue;
      }else if (!strcmp(arguments[0],"quit") || !strcmp(arguments[0],"exit"))
         break;
        
      
      /* Creates a child process */   
      int pid = fork();

      if (pid == 0){ //in child
         
          if (index2 == NULL){
              int fd_new;
              if (index3 != NULL){ // do index3
                  fd_new = open(arguments3[0], O_CREAT | O_TRUNC | O_RDWR, S_IRUSR | S_IRGRP | S_IWGRP | S_IWUSR);
                  if (fd_new < 0){
                      return 0; 
                  }
                  dup2(fd_new, STDOUT_FILENO);
                  close(fd_new);
              }
              execvp(arguments1[0],arguments1);
              perror("Exec Error");
          }else{ 
              int status2; 
              int pid2 = fork();
              if (pid2 == 0){ //in sub-child          
                  //do index2
                  int fd_new_2;
                  int i = 0;
                  char buffer[1024];
                  close(fd[1]);
                  dup2(fd[0], STDIN_FILENO); // STDIN_FILENO --> pipe_read
                  close(fd[0]);
                  if (index3 != NULL){ //do index3  STDOUT_FILENO --> filename
                      fd_new_2 = open(arguments3[0], O_CREAT | O_TRUNC | O_RDWR, S_IRUSR | S_IRGRP | S_IWGRP | S_IWUSR);
                      if (fd_new_2 < 0){
                          return 0; 
                      }
                      dup2(fd_new_2, STDOUT_FILENO);
                      close(fd_new_2);
                  }
                  execvp(arguments2[0],arguments2);
                  perror("Exec Error");
              }else if (pid2 > 0){ // in sub-parent
                  //do index1
                  int pid3;
                  int status3;
                  close(fd[0]);
                  dup2(fd[1], STDOUT_FILENO); // STDOUT_FILENO --> pipe_write
                  close(fd[1]);
                  pid3 = fork();
                  if (pid3 == 0){
                      execvp(arguments1[0],arguments1);
                      perror("Exec Error");
                  }else if (pid3 > 0){
                      wait(&status3);
                      return 1;
                  }else{
                      perror("Fork Error");
                  }
                  waitpid(pid2, &status2, 0);
                  return 1;
              }else if (pid2 < 0){ //error
                  perror("Fork error\n"); 
              }
          }

      }else if (pid > 0){ // in parent
          waitpid(pid, &status, 0);
      }else if (pid < 0){ // error
          perror("Fork error\n");
      }
  }
  while (1); /* Use int values to represent boolean in C language: 0 denotes false, non-zero denotes true */

  return 0;
}

/* Breaks the string pointed by str into words and stores them in the arg array*/
int getArguments(char *str, char *arg[])
{
     char delimeter[] = " ";
     char *temp = NULL;
     int i=0;
     temp = strtok(str,delimeter);
     while (temp != NULL)
     {
           arg[i++] = temp;                   
           temp = strtok(NULL,delimeter);
     }
     arg[i] = NULL;     
     return i;
}

/* Reads a line and discards the end of line character */
int getLine(char *str,int max)
{
    char *temp;
    if ((temp = fgets(str,max,stdin)) != NULL)
    {
       int len = strlen(str);
       str[len-1] = '\0';
       return len-1;
    }   
    else return 0;
}

/* split commandline according to the specific character */
char* splitCommand(char *src, char s){
    int i = 0;
    long num = 0;
    char * re;
    if (src == NULL){
        return NULL;
    }
    re = strchr(src, s);
    if (re == NULL){
        return NULL;
    }
    num = re - src;
    re = (char*)malloc(sizeof(char)*(num+2));
    strcpy(re, src+num+1);
    src[num] = '\0';
    return re;
}
