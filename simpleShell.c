// ARIS PAPAVASSILIOU
#include <stdio.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h> 
#include <ctype.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
// PROGRAM CONSTANTS
const int maxCommandSize = 5000;
const int historyCapacity = 100;
const int maxNumPipes = 100;
//FUNCTION PROTOTYPES
void checkSize();
/*************************************************************************************************/ 
/*************************************************************************************************/ 
/* This section of code will contain our queue struct used to store history and related functions*/ 
/*************************************************************************************************/ 
/*************************************************************************************************/ 
struct Queue { 
 struct node* front; // node ptr to front of queue
 struct node* rear; // node ptr to back of queue 
 int size, capacity; // current size and max size 
};
struct node {
 char* command; // string command for each node
 struct node* next; // pointer to the next node
};
// Constructor for Queue 
struct Queue* createQueue(unsigned capacity)
{
 struct Queue* queue = (struct Queue*)malloc(sizeof(struct Queue)); // mallocate space for Queue
 queue->capacity = capacity; // stores max capacity to queue
 queue->front = queue->rear = NULL;// empty list pointers to null 
 queue->size = 0;// current size of queue is 0
 return queue;
}
// Function to add an element to a queue 
void enqueue(struct Queue* queue, char* item) // Arguements are the queue obj and the item being passed in
{ 
 struct node* newNode = (struct node*)malloc(sizeof(struct node)); // Allocating space for new node 
 newNode->command = malloc(strlen(item) + 1); // Allocate memory for the command string and copy the content
 strcpy(newNode->command, item); // coping item into new node 
 newNode->next = NULL; //set the next value to null since added at the end of the list 
 if (queue->size == 0) // if list is empty before creation 
 {
 queue->front = newNode; //store our newNode to front
 queue->rear = newNode; //store our newnode to rear
 queue->size++; // increment size
 return; //exit enqueue
 }
 // Eles we add newNode to the end of the queue 
 queue->rear->next = newNode;
 queue->rear = newNode;
 queue->size++;
 checkSize(queue); // verify queue isnt larger than capacity
}
// Function to remove element from queue (only called from checkSize when queue has > 100 elements)
void dequeue(struct Queue* queue)
{
 struct node* temp = queue->front; // store the front value to a temp pointer 
 queue->front = queue->front->next; // change front to next node 
 queue->size--; // decrement size of queue 
 free(temp->command); // free old head command
 free(temp); // free old head 
}
// Function to compare size to capacity which tells us if we need to dequeue
void checkSize(struct Queue* queue)
{
 if (queue->size > queue->capacity + 1)
 {
 dequeue(queue);
 }
}
// Function to print out queue or return command 
char* printQueue(struct Queue* queue, int index) { //if index = -1 we simply print out history 
 struct node* temp = queue->front; //node pointer to head for traversal
 int increment;
 if (index == -1) // if we want to print out entire queue
 { 
 increment = queue->size; // set our incremenet value to size of queue
 } 
 else if (index >= historyCapacity || index > queue->size)// check to make sure index isnt out of queue->size or historyCapacity
 { 
 return "error";
 } 
 else { // else we set increment to index and return the command 
 increment = index;
 }
 
 for (int i = 0; i < increment; i++) // for loops traversing the queue 
 {
 if (temp == NULL) // Make sure we dont accidently go out of bounds 
 { 
 break;
 }
 if (index == -1) {
 printf("%d", i); // print out index
 printf(" ");
 printf("%s\n", temp->command); // print out command 
 }
 if (temp->next != NULL) { //make sure we dont go out of bounds
 temp = temp->next;
 }
 }
 if (index != -1) // allocate space for command copy and return 
 {
 char* result = malloc(strlen(temp->command) + 1);
 strcpy(result, temp->command);
 return result; 
 }
 return NULL;
}
//Function to delete all items in queue 
void deleteQueue(struct Queue* q)
{
 while(q->size != 0)
 {
 dequeue(q);
 }
}
/*************************************************************************************************/ 
/*************************************************************************************************/ 
/* This part of the code includes our main driver and other functions for implimenting our shell */
/*************************************************************************************************/ 
/*************************************************************************************************/ 
// Function which determines if and how many pipes a token has 
int hasPipes(char **tokens) 
{
 int pipes = 0; 
 int i = 0;
 while (tokens[i] != NULL) // we loop through each token and each char of the token looking for "|"
 {
 int j = 0;
 while (tokens[i][j] != '\0') 
 {
 if (tokens[i][j] == '|') 
 {
 pipes++;
 }
 j++;
 }
 i++;
 }
 if (pipes >= maxNumPipes) // if too many pipes are found
 {
 return -1; 
 }
 return pipes; // else we return
}
// Function used to parse input string into a array by a specific delimiter
void tokenizeInput(char *input, char** tokens, const char* delimiter)
{
 int counter = 0;
 char *token = strtok(input,delimiter);
 while (token != NULL && counter < maxCommandSize - 1) // loop through line parsing it, validating we dont pass the maxCommandSize
 {
 tokens[(counter)++] = token;
 token = strtok(NULL, delimiter);
 }
 tokens[counter] = NULL; // NULL-terminate the token array
}
// Function used when a piped command is given
void exePipedCMD(char *input, int numofPipes)// input is default input string and number of pipes 
{
 char* commands[maxCommandSize]; // this will hold our parsed piped commands 
 const char* delimiter = "|"; // delimiter for the parsing 
 tokenizeInput(input, commands, delimiter); // tokenize the string 
 int numberofCommands = numofPipes + 1; // total number of commands is pipes + 1
 int pipes[numberofCommands - 1][2]; // Arrays to hold pipe descriptors and process IDS
 pid_t pids[numberofCommands];
 // for loop creating all our pipes
 for (int i = 0; i < numberofCommands - 1; i++) {
 if (pipe(pipes[i]) < 0) {
 perror("pipe");
 return;
 }
 }
 // for loop running all our commands
 for (int i = 0; i < numberofCommands; i++) {
 pids[i] = fork(); 
 if (pids[i] < 0) // if error detected in the forking process
 {
 perror("fork");
 exit(1);
 } 
 else if (pids[i] == 0) 
 {
 // Child process
 if (i > 0) {
 dup2(pipes[i - 1][0], STDIN_FILENO); // Redirect stdin from previous pipe
 close(pipes[i - 1][0]); // Close read end of previous pipe
 }
 if (i < numberofCommands - 1) {
 dup2(pipes[i][1], STDOUT_FILENO); // Redirect stdout to current pipe
 close(pipes[i][1]); // Close write end of current pipe
 }
 // Close all pipe file descriptors in child process
 for (int j = 0; j < numberofCommands - 1; j++) {
 close(pipes[j][0]);
 close(pipes[j][1]);
 }
 // Tokenize the command and execute
 char* tokens[maxCommandSize];
 tokenizeInput(commands[i], tokens, " ");
 execvp(tokens[0], tokens);
 perror("execvp"); // This will print an error message if any command fails
 exit(1);
 }
 }
 // Close all pipe file descriptors in parent process
 for (int i = 0; i < numberofCommands - 1; i++) {
 close(pipes[i][0]);
 close(pipes[i][1]);
 }
 // Wait for all child processes to finish
 for (int i = 0; i < numberofCommands; i++) {
 waitpid(pids[i], NULL, 0);
 }
}
// Function to Execute commands 
void exeCMD(char **tokens, struct Queue* history,char* input) // arguments are our tokenized string, history Queue, and orignal input line
{
 int totalPipes = hasPipes(tokens); // get total number of pipes 
 if (totalPipes == -1) // if to many pipes are found we print error message and return
 {
 printf("to many pipes listed\n");
 return;
 }
 if (tokens[0] == NULL) // If command is empty
 {
 return; 
 }
 else if (strcmp(tokens[0], "exit") == 0) // command to exit shell
 {
 exit(0);
 }
 else if (strcmp(tokens[0], "cd") == 0) // change directory command 
 {
 if (tokens[1] != NULL) // validate it has an argument 
 {
 if (chdir(tokens[1]) != 0) // perfrom cd 
 {
 perror("cd"); // print error message 
 }
 } 
 else // no argument 
 {
 fprintf(stderr, "cd: missing argument\n");
 }
 }
 else if (strcmp(tokens[0], "history") == 0) // command for history 
 {
 if (tokens[1] != NULL && strcmp(tokens[1], "-c") == 0) // deletes the queue 
 {
 deleteQueue(history);
 return;
 }
 else if (tokens[1] != NULL){ // history offset command finds specific command in history and executes it
 int offset = 0;
 offset = atoi(tokens[1]); // set offset to a int value 
 char* command = printQueue(history,offset);
 char *command_copy = malloc(strlen(command) + 1); // since token changes original string
 strcpy(command_copy, command);
 if (strcmp(command, "error") == 0) // if there was an error when traversing the queue
 {
 printf("Invalid history offset\n"); //print error message and return
 return;
 }
 else{ // else we tokenize new input and recall exeCMD with new command 
 tokenizeInput(command,tokens," ");
 exeCMD(tokens,history,command_copy);
 return;
 }
 }
 else{
 printQueue(history,-1); // else we entire history if no arguments
 }
 }
 else if (totalPipes > 0){ // a pipe is found 
 exePipedCMD(input,totalPipes);
 }
 else // for executing a single command 
 {
 pid_t pid = fork();
 if (pid < 0) {
 perror("fork"); // if fork failed 
 } else if (pid == 0) {
 execvp(tokens[0], tokens);
 perror("execvp"); // if execvp failed
 exit(1);
 } else {
 wait(NULL); // Wait for the child process to finish
 }
 }
}
int main()
{
 // variable initialization
 int exitCMD = 0;
 char *line = NULL;
 size_t bufsize = 0;
 struct Queue* history = createQueue(historyCapacity); // create our Queue passing in our capacity 
 char* tokens[maxCommandSize]; 
 const char* delimiter = " ";
 
 while (exitCMD != -1) // main shell loop
 {
 printf("sish> "); // default prompt
 if (getline(&line, &bufsize, stdin) == -1) { // reading prompt 
 perror("getline");
 exit(EXIT_FAILURE);
 }
 line[strcspn(line, "\n")] = '\0'; // removing trailing newline character
 char *line_copy = malloc(strlen(line) + 1); // since token changes original string
 strcpy(line_copy, line); // save the original string if we need to pipe it 
 enqueue(history, line); // add to history 
 tokenizeInput(line, tokens, delimiter); // input line into tokens
 exeCMD(tokens, history, line_copy); // function to execute commands 
 }
 // since history is allocated in memory we must delete
 dequeue(history);
 free(history);
 return 0;
}
