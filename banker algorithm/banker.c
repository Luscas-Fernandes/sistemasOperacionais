#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CUSTOMER_FILE "customer.txt"
#define COMMANDS_FILE "commands.txt"
#define RESULT_FILE "result.txt"
#define FALSE 0
#define TRUE 1
#define MAX_COMMAND_SIZE 128

// Aux Functions
int count_customers(FILE *pointer);
int count_resources(int resources);
void init_available(int number_of_resources, char **resources, int *available);
void init_NOC_and_NOR_matrix(int **matrix, int number_of_customers, int number_of_resources);
void init_maximum(FILE *pointer, int **matrix, int number_of_customers, int number_of_resources);
void calc_need(int **matrix, int **maximum, int **allocation, int number_of_customers, int number_of_resources);
void init_allocation(int **matrix, int number_of_customers, int number_of_resources);
void init_finish(int number_of_customers, int *array);
void read_commands(char **cmd, FILE *fp);
void print_RQ(FILE *target_pointer, char **command, int **need, int **allocation, int *available, int number_of_resources, int return_number);
void print_RL(FILE *target_pointer, char **command, int **need, int **allocation, int *available, int number_of_resources, int return_number);


// Banker Functions
int request_resources(FILE *target_pointer, char **command, int **need, int **allocation, int *available, int number_of_resources, int *finish, int number_of_customers);
int release_resources(FILE *target_pointer, char **command, int **need, int **allocation, int *available, int number_of_resources);
void print_table(FILE *target_pointer, int **maximum, int **allocation, int **need, int *available, int number_of_customers, int number_of_resources, char **command);


int main(int argc, char** argv)
{
	FILE *customerFP, *commandsFP, *target_pointer;
	int number_of_resources = count_resources(argc);

	commandsFP = fopen(COMMANDS_FILE, "r");
	if (commandsFP == NULL)
	{
		fprintf(stderr, "Fail to read commands.txt\n");
		exit(1);
	}

	int number_of_customers = count_customers(customerFP);
	int n = number_of_customers, m = number_of_resources;

	int *available = (int *)malloc(number_of_resources * sizeof(int));
	int **maximum = (int **)malloc(n * sizeof(int*)); // Allocating Columns
	int **allocation = (int **)malloc(n * sizeof(int*)); // Allocating Columns
	int **need = (int **)malloc(n * sizeof(int*)); // Allocating Columns
	int *finish = (int *)malloc(number_of_resources * sizeof(int)); 
	
	init_available(number_of_resources, argv, available); // Initializing available matrix

	init_NOC_and_NOR_matrix(maximum, number_of_customers, number_of_resources); // Initializing maximum matrix
	init_maximum(customerFP, maximum, number_of_customers, number_of_resources);

	init_NOC_and_NOR_matrix(allocation, number_of_customers, number_of_resources); // Initializing allocation matrix
	init_allocation(allocation, number_of_customers, number_of_resources);

	init_NOC_and_NOR_matrix(need, number_of_customers, number_of_resources); // Initializing allocation matrix
	calc_need(need, maximum, allocation, number_of_customers, number_of_resources); // calculating need matrix

	// command -> customer_number, resources, null (end of string)
	char **command = (char **)malloc((3 + number_of_resources) * sizeof(char *));
	int rq_result, rl_result;

	target_pointer = fopen("result.txt", "w");
	while (!feof(commandsFP)) 
	{
		read_commands(command, commandsFP);
		
		if(strcmp(command[0], "RQ") == 0)
		{
			rq_result = request_resources(target_pointer, command, need, allocation, available, number_of_resources, finish, number_of_customers);
			print_RQ(target_pointer, command, need, allocation, available, number_of_resources, rq_result);
		}
			
		else if(strcmp(command[0], "RL") == 0)
		{
			rl_result = release_resources(target_pointer, command, need, allocation, available, number_of_resources);
			print_RL(target_pointer, command, need, allocation, available, number_of_resources, rl_result);
		}

		else if(strcmp(command[0], "*") == 0)
		{
			print_table(target_pointer, maximum, allocation, need, available, number_of_customers, number_of_resources, command);
		}

	}

	/* for(int i = 0; i < n; i++)
	{
		free(maximum[n]); 
		free(allocation[n]);
		free(need[n]);
		free(command[n]);
	} */

	free(maximum);
	free(allocation);
	free(need);
	free(command);
	free(available);
	free(finish);

	
	
	fclose(commandsFP);
	fclose(target_pointer);
	return 0;
}


// Auxiliary Functions
int count_resources(int resources) 
{
	return --resources;
}


int count_customers(FILE *pointer)
{   
	pointer = fopen(CUSTOMER_FILE, "r");
	if (pointer == NULL)
	{
		fprintf(stderr, "Fail to read customer.txt\n");
		exit(1);
	}

	int number_of_customers = 1;
	char ch;

	while((ch=fgetc(pointer)) != EOF)
		if (ch=='\n')
			number_of_customers++;

	fclose(pointer);
	return number_of_customers;
}


void init_available(int number_of_resources, char **resources, int *available)
{   
	for(int i = 0; i < number_of_resources; i++)
		available[i] = atoi(resources[i + 1]);
}


// initializing NOC and NOR matrixes, matrixes that are composed by matrix[NUMBER_OF_CUSTOMERS][NUMBER_OF_RESOURCES] or matrix[n][m]
void init_NOC_and_NOR_matrix(int **matrix, int number_of_customers, int number_of_resources)
{
	// Alocating each row for the matrix matrix
	for(int i = 0; i < number_of_customers; i++)
		matrix[i] = (int *)malloc(number_of_resources * sizeof(int));
}


void init_maximum(FILE *pointer, int **matrix, int number_of_customers, int number_of_resources)
{
	pointer = fopen(CUSTOMER_FILE, "r");
	if(pointer == NULL)
	{
		fprintf(stderr, "Fail to read commands.txt\n");
		exit(1);
	}

	// Alocating each row for the matrix matrix
	for(int i = 0; i < number_of_customers; i++)
	{
		for(int j = 0; j < number_of_resources; j++)
		{
			fscanf(pointer, "%d,", &matrix[i][j]);
		}
	}

	fclose(pointer);
}


void init_allocation(int **matrix, int number_of_customers, int number_of_resources)
{
	for(int i = 0; i < number_of_customers; i++)
	{
		for(int j = 0; j < number_of_resources; j++)
		{
			matrix[i][j] = 0;
		}
	}
}

void calc_need(int **matrix, int **maximum, int **allocation, int number_of_customers, int number_of_resources)
{
	for(int i = 0; i < number_of_customers; i++)
	{
		for(int j = 0; j < number_of_resources; j++)
		{
			matrix[i][j] = maximum[i][j] - allocation[i][j];
		}
	}
}


void init_finish(int number_of_customers, int *array)
{
	for(int i = 0; i < number_of_customers; i++)
	{
		array[i] = FALSE;
	}
}


void read_commands(char **cmd, FILE *fp)
{
	char buffer[MAX_COMMAND_SIZE];

	fgets(buffer, MAX_COMMAND_SIZE, fp);

	int loc = 0;
	char *token = strtok(buffer, " \n");
	while (token != NULL) {
		size_t size = strlen(token) + 1;
		cmd[loc] = malloc(size);
		strcpy(cmd[loc], token);
		token = strtok(NULL, " \n");
		loc++;
	}
	cmd[loc] = NULL;
}

// End of aux functions


// Banker's Functions
// -2 cannot allocate resources, unsafe state
// -1 cannot allocate resources, request > need
//  0 cannot allocate resources, request > available
//  1 can allocate_resources 

int request_resources(FILE *target_pointer, char **command, int **need, int **allocation, int *available, int number_of_resources, int* finish, int number_of_customers)
{
	// Check if request > need, return -1 if it is
	for(int i = 0; i < number_of_resources; i++)
		if(atoi(command[i + 2]) > need[atoi(command[1])][i])
			return -1;

	
	// Check if request > available, return 0 if it is
	for(int i = 0; i < number_of_resources; i++)
		if(atoi(command[i + 2]) > available[i])
			return 0;


	// Check if it is in a safe state to allocate, return -2 if it is unsafe, 1 if safe
	int work[number_of_resources], todos_need, status_change, finish_count;

	// Initializing finish matrix and initializing work
	for(int i = 0; i < number_of_resources; i++) 
		work[i] = available[i];
	init_finish(number_of_customers, finish);

	printf("\n");
	finish_count = 0;
	while (finish_count < number_of_customers)
	{
		status_change = 0;
		for (int i = 0; i < number_of_customers; i++)
		{
			if (!finish[i])
			{
				int todos_need = 1; // Initialize to true for the current process
				for (int j = 0; j < number_of_resources; j++)
				{
					if (need[i][j] > work[j])
					{
						todos_need = 0;
						break;
					}
				}

				if (todos_need)
				{
					for (int j = 0; j < number_of_resources; j++)
					{
						printf("work[%d] += allocation[%d][%d]: %d\n", j, i, j, allocation[i][j]);
						printf("New work = %d\n", work[j]);
						work[j] += allocation[i][j];
					}
					finish[i] = 1;
					status_change = 1;
					finish_count++;
					printf("finish[%d] = %d, status_change = %d, finish_count = %d\n", i, finish[i], status_change, finish_count);
				}
			}
		}
		// If there is no status change for any finish[i], it is unsafe
		if (!status_change)
		{
			printf("No status change\n");
			return -2;
		}
	}

	printf("Can Allocate!\n");
	return 1;
}

// Return -1 Cannot release, release exceeds allocated
// Return 1 Release
int release_resources(FILE *target_pointer, char **command, int **need, int **allocation, int *available, int number_of_resources)
{
	for(int i = 0; i < number_of_resources; i++)
		if(atoi(command[i + 2]) > allocation[atoi(command[1])][i])
			return -1;

	return 1;
}

// Print functions for Banker's algorithm
void print_RQ(FILE *target_pointer, char **command, int **need, int **allocation, int *available, int number_of_resources, int return_number)
{
	if(return_number == 1)
	{
		fprintf(target_pointer, "Allocate to customer %d the resources ", (atoi(command[1])));
		for(int i = 0; i < number_of_resources; i++)
		{
			available[i] -= atoi(command[i + 2]);
			allocation[atoi(command[1])][i] += atoi(command[i + 2]);
			need[atoi(command[1])][i] -= atoi(command[i + 2]);
			fprintf(target_pointer, "%d ", atoi(command[i + 2]));
		}
		fprintf(target_pointer, "\n");
	}
	else if(return_number == -1)
	{
		fprintf(target_pointer, "The customer %d request ", atoi(command[1]));
		for(int i = 0; i < number_of_resources; i++)
			fprintf(target_pointer, "%d ", atoi(command[i + 2]));
		fprintf(target_pointer, "was denied because exceed its maximum need\n");
	}
	else if(return_number == 0)
	{
		fprintf(target_pointer, "The resources ");
		for(int i = 0; i < number_of_resources; i++)
			fprintf(target_pointer, "%d ", available[i]);
		fprintf(target_pointer, "are not enough to customer %d request ", atoi(command[1]));
		for(int i = 0; i < number_of_resources; i++)
			fprintf(target_pointer, "%d ", atoi(command[i + 2]));
		fprintf(target_pointer, "\n");
	}
	else if(return_number == -2)
	{
		fprintf(target_pointer, "The customer %d request ", atoi(command[1]));
		for(int i = 0; i < number_of_resources; i++)
			fprintf(target_pointer, "%d ", atoi(command[i + 2]));
		fprintf(target_pointer, "was denied because result in an unsafe state\n");
	}
}

void print_RL(FILE *target_pointer, char **command, int **need, int **allocation, int *available, int number_of_resources, int return_number)
{
	if(return_number == 1)
	{
		fprintf(target_pointer, "Release from customer %d the resources ", atoi(command[1]));
		for(int i = 0; i < number_of_resources; i++)
		{
			fprintf(target_pointer, "%d ", atoi(command[i + 2]));
			available[i] += atoi(command[i + 2]);
			allocation[atoi(command[1])][i] -= atoi(command[i + 2]);
			need[atoi(command[1])][i] += atoi(command[i + 2]);
		}
		fprintf(target_pointer, "\n");
	}
	else if (return_number == -1)
	{
		fprintf(target_pointer, "The customer %d released ", atoi(command[1]));
		for(int i = 0; i < number_of_resources; i++)
			fprintf(target_pointer, "%d ", atoi(command[i + 2]));
		fprintf(target_pointer, "was denied because exceed its maximum allocation\n");
	}
}

void print_table(FILE *target_pointer, int **maximum, int **allocation, int **need, int *available, int number_of_customers, int number_of_resources, char **command) 
{
	fprintf(target_pointer, "MAXIMUM\t| ALLOCATION | NEED\n");
	for(int i = 0; i < number_of_customers; i++)
	{       
		for(int j = 0; j < number_of_resources; j++)
			fprintf(target_pointer, "%d ", maximum[i][j]);
		fprintf(target_pointer, "\t| ");
		for(int j = 0; j < number_of_resources; j++)
			fprintf(target_pointer, "%d ", allocation[i][j]);
		fprintf(target_pointer, "\t | ");
		for(int j = 0; j < number_of_resources; j++)
			fprintf(target_pointer, "%d ", need[i][j]);
		fprintf(target_pointer, "\n");
	}
	fprintf(target_pointer, "AVAILABLE ");
	for(int i = 0; i < number_of_resources; i++)
		fprintf(target_pointer, "%d ", available[i]);
	fprintf(target_pointer, "\n");
}