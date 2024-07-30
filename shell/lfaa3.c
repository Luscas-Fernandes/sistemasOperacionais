#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <pthread.h>

#define MAX_BUFF 1024
#define PARALLEL 1
#define SEQUENTIAL 0
#define INTERATIVA 0
#define BATCH 1

int num_comandos;
int flagEcomercial;
int static style;
char *historico[2];

void *execThread(void *comandos);
void executa(char **argumentos);
void execStyle(char* string);
int comandoExtra(char* token);
void defaultFork(char** argumentos);
void pipeFork(char *argumentos);
char** pegaString(int modoExecucao, FILE *arq);
void pegaArgumentos(char **comandos);


void *execThread(void *comandos)
{
    //printf("entrei\n");
    char *comandosNaoVoid = (char *)comandos;
    char *argumentosNaoVoid[MAX_BUFF];
    //printf("%s\n", comandosNaoVoid);
    
    int contador = 0;
    char *tokenn = strtok(comandosNaoVoid, " ");
    while(tokenn)
    {
        argumentosNaoVoid[contador] = strdup(tokenn);
        tokenn = strtok(NULL, " ");
        contador++;
    }
    argumentosNaoVoid[contador] = NULL;

    defaultFork(argumentosNaoVoid);

    //defaultFork(comandosNaoVoid[0]);
    pthread_exit(0);
}

void executa(char **argumentos)
{
    if(execvp(argumentos[0], argumentos) == -1) // executa o comando em outro processo literal, por isso o exit dá exit só do processo dele
    {
        printf("ARGUMENTO \"%s\" INVÁLIDO!\n", argumentos[0]);
        exit(EXIT_FAILURE);
    }
}

void execStyle(char* string)
{
    if(strstr(string, "style parallel"))
        style = PARALLEL;
    else if(strstr(string, "style sequential"))
        style = SEQUENTIAL;
}

int comandoExtra(char* token)
{
    int customComand;
    if(strstr(token, "&")) // -> arg[1]
        customComand =  1;
    else if(strstr(token, "<")) // -> arg[1]
        customComand =  2;
    else if(strstr(token, ">")) // -> arg[1]
        customComand =  3;
    else if(strstr(token, ">>")) // -> arg[1]
        customComand =  4;
    else if(strstr(token, "|")) // -> arg[1]
        customComand =  5;
    else
        customComand =  0; // default

    return customComand;
}

void defaultFork(char** argumentos)
{
    pid_t pid = fork();

    if (pid < 0) 
    {
        fprintf(stderr, "error forking\n");
        exit(EXIT_FAILURE);
    } 
    else if (pid == 0) // child
        executa(argumentos);
    else if (pid > 0 && flagEcomercial == 0) // papa
        wait(NULL);
}

void pipeFork(char *argumentos)
{
    int fd[2], pipeArgumentCounter = 0;
    char **pipeLista = (char **)malloc((MAX_BUFF / 2) *sizeof(char *));
    char *tok;
    char preString[MAX_BUFF], posString[MAX_BUFF];
    

    tok = strtok(argumentos, "|");
    while(tok)
    {
        pipeLista[pipeArgumentCounter] = strdup(tok);
        pipeArgumentCounter++;
        tok = strtok(NULL, "|");
    }

    strcpy(preString, pipeLista[0]);
    strcpy(posString, pipeLista[1]);


    if (pipe(fd) == -1)
    {
        fprintf(stderr, "Error openning pipe\n");
        exit(EXIT_FAILURE);
    }

    pid_t pidFork = fork();

    if (pidFork < 0) 
    {
        fprintf(stderr, "error forking pipe\n");
        exit(EXIT_FAILURE);
    } 
    else if (pidFork == 0) // child
    {
        dup2(fd[1], STDOUT_FILENO);
        close(fd[0]);

        //logica
        char *listaFilho[MAX_BUFF];
        int listaFilhoCounter = 0;
        listaFilho[listaFilhoCounter] = strtok(preString, " ");
        while(listaFilho[listaFilhoCounter] != NULL)
        {
            listaFilhoCounter++;
            listaFilho[listaFilhoCounter] = strtok(NULL, " ");
        }
        listaFilhoCounter++;
        listaFilho[listaFilhoCounter] = NULL;

        defaultFork(listaFilho);
        exit(EXIT_SUCCESS);
    }
    else if (pidFork > 0 && flagEcomercial == 0) // papa
    {
        close(fd[1]);
        dup2(fd[0], STDIN_FILENO);

        //logica
        char *listaPai[MAX_BUFF];
        int listaPaiCounter = 0;
        listaPai[listaPaiCounter] = strtok(posString, " ");
        while(listaPai[listaPaiCounter] != NULL)
        {
            listaPaiCounter++;
            listaPai[listaPaiCounter] = strtok(NULL, " ");
        }
        listaPaiCounter++;
        listaPai[listaPaiCounter] = NULL;

        wait(NULL);
        defaultFork(listaPai);
        exit(EXIT_SUCCESS);
    }
    free(pipeLista);
}

char** pegaString(int modoExecucao, FILE *arq)
{
    char *string = (char *)malloc(sizeof(char) * MAX_BUFF);
    char *stringCPY = (char *)malloc(sizeof(char) * MAX_BUFF);
    char **comandos = (char **)malloc((MAX_BUFF / 2) *sizeof(char *));
    
    char *token;
    
    num_comandos = 0;

    if (modoExecucao == INTERATIVA)
    {
        printf("lfaa3 %s> ", style == 0 ? "seq" : "par");
        fgets(string, MAX_BUFF, stdin);
        if(feof(stdin)) 
        {
            printf("Exiting via \"CTRL+D\"\n");
            exit(EXIT_SUCCESS);
        }
        string[strlen(string) - 1] = '\0';
    }
    else if(modoExecucao == BATCH)
    {
        size_t len = 0;
        char *line = (char*)malloc(sizeof(char) * 80);

        while (getline(&line, &len, arq) != -1) 
        {
            line[strcspn(line, "\r\n")] = '\0';
            line[strcspn(line, "\n")] = '\0';
        
            if(line[strlen(line) - 1] != ';')
                strcat(line, ";");
            
            strcat(line, " ");
            strcat(string, line);
        }
        if(!strstr(string, "exit"))
        {
            printf("Falta de comando \"exit\" ao fim do arquivo batch, adicionado automaticamente\n");
            strcat(string, "exit;");
        }

        free(line);
    }

    //strcpy(stringCPY, string); // stringCPY pro historico
    execStyle(string); // par ou seq

    token = strtok(string, ";");

    while(token)
    {
        if(!strcmp(token, "!!") || !strcmp(token, " !!")) // history
        {
            if(historico[0] != NULL)
                {
                    char *htok;
                    htok = strtok(historico[0], " ");
                    strcpy(historico[0], htok);
                    historico[1] = NULL;
                    defaultFork(historico);
                }
            else if(historico[0] == NULL)
                printf("No commands\n");
            strcpy(stringCPY, "!!");
        }
        else if(comandoExtra(token) == 0)                 // default
        {
            comandos[num_comandos] = strdup(token);
            if(modoExecucao == BATCH)
                printf("comando lido: %s\n", comandos[num_comandos]);
            num_comandos++;
        }
        else if(comandoExtra(token) == 5) // pipe
        {
            pipeFork(token);
        }

        //printf("num comandos: %d\n", num_comandos);
        //printf("%s\n", comandos[num_comandos]);
        //printf("comando: %s nº[%d]\n", comandos[num_comandos], num_comandos);
        //printf("historico: %s\n", historico);
        token = strtok(NULL, ";");
    }
    
    if(num_comandos > 0)
        strcpy(stringCPY, comandos[num_comandos - 1]);

    if(strlen(stringCPY) == 0)    
        historico[0] = NULL;
    else
        historico[0] = strdup(stringCPY);
        
    free(string);
    return comandos;
}

void pegaArgumentos(char **comandos)
{
    int contaComandos;
    char *token;
    char **argumentos = (char **)malloc(MAX_BUFF / 2 * sizeof(char*));
    char **comandoCPY = (char **)malloc((MAX_BUFF / 2) *sizeof(char *));

    for(int i = 0; i < num_comandos; i++)
        comandoCPY[i] = strdup(comandos[i]);

    for(int i = 0; i < num_comandos; i++)
    {
        flagEcomercial = 0;
        contaComandos = 0;
        token = strtok(comandos[i], " ");
        while(token)
        {
            argumentos[contaComandos] = strdup(token);
            //printf("comando: %s nº[%d]\n", argumentos[contaComandos], contaComandos);
            if(!strcmp(token, "exit")) exit(EXIT_SUCCESS);

            token = strtok(NULL, " ");
            contaComandos++;
        }

        argumentos[contaComandos] = NULL; // null do exec
        //printf("comando: %s nº[%d]\n", argumentos[contaComandos], contaComandos);

        if(style == SEQUENTIAL)
            defaultFork(argumentos); 

        for(int i = 0; i < contaComandos; i++)
            free(argumentos[i]);
    }

    if(style == PARALLEL)
    {
        pthread_t threadArray[num_comandos];
        for (int i = 0; i < num_comandos; i++)
            pthread_create(&threadArray[i], NULL, execThread, (void *)comandoCPY[i]);
        
        for (int i = 0; i < num_comandos; i++)
            pthread_join(threadArray[i], NULL);
    }
    //free(comandos);
}

int main(int argc, char *argv[])
{

    FILE *batch;
    int modoExecucao;

    // dealing with interactive or batch mode and too many argumments
    if(argc == 1)
        modoExecucao = INTERATIVA;
    else if(argc == 2)
    { 
        modoExecucao = BATCH;
        batch = fopen(argv[1], "r"); 
        if (batch == NULL)
        {
            fprintf(stderr, "Unable to open file, closing!\n");
            exit(EXIT_FAILURE);
        }
    }
    else
    {
        fprintf(stderr, "Too many arguments!\n");
        exit(EXIT_FAILURE);
    }

    while(1)
    {
        char **comandos = pegaString(modoExecucao, batch);
        pegaArgumentos(comandos);
    }

    return 0;
}