#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <unistd.h>

#define BUFFER_SIZE 10000
//#define PORT_NO 5001

extern char **environ;

typedef struct ras_pipe_tt ras_pipe_t;
struct ras_pipe_tt {
	int counter;
	char *buffer;
	ras_pipe_t* left;
	ras_pipe_t* right;
};

ras_pipe_t* head = NULL;

ras_pipe_t* tail = NULL;

int exec_com( char* input , char* output ,int pipe_num, int nfd);

void insert_ras_pipe(char *insert_buffer,int insert_counter);

int pop_ras_pipe( char *ras_pipe_output );

void inc_ras_pipe();

void printallenv();

char *grep_env(char *pattern);

int main( int argc, char *argv[] )
{
	int PORT_NO;
	if( argc != 2 ) PORT_NO = 5001;
	else PORT_NO = atoi(argv[1]);
    char myhome[256] ;
    strcpy(myhome,getenv("HOME"));
    strcat(myhome,"/ras");
    chdir(myhome);
    int sockfd, newsockfd, portno, clilen;
    char *tok, *argtok;
    char pipe_output[BUFFER_SIZE];
    char commands[BUFFER_SIZE];
    char commands_tmp[BUFFER_SIZE];
    char buffer[BUFFER_SIZE];
    struct sockaddr_in serv_addr, cli_addr;
    int  i, n, pipe_counter;

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) 
    {
        perror("ERROR: Unable to open socket.");
        exit(1);
    }
    
    bzero((char *) &serv_addr, sizeof(serv_addr));
    portno = PORT_NO;
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(portno);
 
    if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0)
    {
         perror("ERROR: Unable to binding.");
         exit(1);
    }

    listen(sockfd,5);
    clilen = sizeof(cli_addr);

    newsockfd = accept(sockfd, (struct sockaddr *)&cli_addr,&clilen); 
    if (newsockfd < 0) 
    {
        perror("ERROR: Unable to accept.");
        exit(1);
    }
    
    bzero(buffer,BUFFER_SIZE);
    setenv("PATH","bin:.",1);

    send(newsockfd,"****************************************\n",41,0);
    send(newsockfd,"** Welcome to the information server. **\n",41,0);
    send(newsockfd,"****************************************\n",41,0);


/*
    write(newsockfd,"****************************************\n",41);
    write(newsockfd,"** Welcome to the information server. **\n",41);
    write(newsockfd,"****************************************\n",41);
*/
    while( 1 )
    {
	//write(newsockfd,"% ",2);
	send(newsockfd,"% ",2,0);
    	n = recv( newsockfd,buffer,BUFFER_SIZE,0 );
	
    	if (n < 0)
    	{
        	perror("ERROR: Unable to read from socket.");
        	exit(1);
    	}
	int buff_len = strlen(buffer);
	for( i = 0 ; i < buff_len ; i++ )
	{
		if(buffer[i]=='\r' || buffer[i] == '\n' )
			buffer[i] = 0;
		else if( buffer[i] == '/' )break;
	}
	if( i != buff_len ) continue;
	//buffer[strlen(buffer)-2]=0;
	char *tmp = strtok(buffer," ");
		while( tmp != NULL )
		{
			if( tmp[0] == '|')
			{
				if( tmp[1] == 0 )
					pipe_counter=1;
				else
				{ 
					pipe_counter=atoi(&tmp[1]);
				}
				if( !strncmp( commands , "exit " , 5) || ( strlen(commands) == 4 && !strncmp( commands , "exit" , 4 ) ) )
					return 0;

				bzero(pipe_output,BUFFER_SIZE);
				strcpy(commands_tmp,commands);
				commands_tmp[strlen(commands_tmp)-1] = 0;
				int len_tmp = strlen(commands_tmp);
				char *args[3] = {NULL};
				args[0] = commands_tmp;
				for( i = 0 ; i < len_tmp ; i++ )
				{
					if( commands_tmp[i] == ' ' && args[1] == NULL) { commands_tmp[i] = 0 ; args[1] = &commands_tmp[i+1]; }
					else if( commands_tmp[i] == ' ' && args[2] == NULL ) { commands_tmp[i] = 0 ; args[2] = &commands_tmp[i+1]; break; }
				}
				if(args[0] != NULL && !strcmp(args[0],"setenv")){
					if( args[2] == NULL ) args[2] = " ";
                                        setenv(args[1],args[2],1);
					printallenv();
                                	bzero(commands,BUFFER_SIZE);
                               		pipe_counter = 0;
                               		tmp = strtok(NULL," ");
                                }
				else 
				{
					if( exec_com(commands,pipe_output,pipe_counter,newsockfd) > 0 )
					{
						bzero(commands,BUFFER_SIZE);
						printf("failed\n");
						break;
					}
					insert_ras_pipe(pipe_output,pipe_counter);
					bzero(commands,BUFFER_SIZE);
					pipe_counter = 0;
					tmp = strtok(NULL," ");
				}
			}
			else
			{
				strcat(commands,tmp);
				strcat(commands," ");
				tmp = strtok(NULL," ");
			}
		}	
	if( tmp == NULL && commands != NULL )
	{
		if( !strncmp( commands , "exit " , 5) || ( strlen(commands) == 4 && !strncmp( commands , "exit" , 4 ) ) )
			return 0;
		bzero(pipe_output,BUFFER_SIZE);
		char *args[3] = {NULL};
		strcpy(commands_tmp,commands);
		/*tok = strtok(commands_tmp," ");
		i = 0;
                while(tok != NULL){
			args[i]=tok;
                        tok = strtok(NULL," ");
                        i++;
                }*/
		args[0] = strtok(commands_tmp," ");
		args[1] = strtok(NULL," ");
		args[2] = strtok(NULL," ");
                if(args[0] != NULL && !strcmp(args[0],"setenv")){
			if( args[2] == NULL ) args[2] = " ";
                        setenv(args[1],args[2],1);
			printallenv();
                }
		else  	
			exec_com(commands,pipe_output,pipe_counter,newsockfd);
		bzero(commands,BUFFER_SIZE);
		pipe_counter = 0;
	}
	strcpy(buffer,pipe_output);
	//write(newsockfd,buffer,strlen(buffer));
	send(newsockfd,buffer,strlen(buffer),0);
	if (n < 0)
    	{
        	perror("ERROR: Unable to write to socket.");
        	exit(1);
    	}
    	bzero(buffer,BUFFER_SIZE);
    }
    return 0; 
}

int exec_com( char *input , char *output , int pipe_num ,int nfd)
{
	FILE *fptr = NULL;
	char *tok, *ftok, *file_name, input_tmp[BUFFER_SIZE];
	char pipe_input_buffer[BUFFER_SIZE];
	pop_ras_pipe(pipe_input_buffer);
	int to_file = 0;
	strcpy(input_tmp,input);
	input_tmp[strlen(input_tmp)-1] = 0;
	ftok = strstr(input_tmp," > ");
	if( ftok != NULL ) { 
		to_file = 1; 
		file_name = &ftok[3] ; 
	        fptr = fopen(file_name,"w");
		}

    int pipe_fd[2];
    int pipe2_fd[2];
    if (pipe(pipe_fd) == -1)
    {
        perror("Error: Unable to create pipe.");
        exit(EXIT_FAILURE);
    }
    if (pipe(pipe2_fd) == -1)
    {   
        perror("Error: Unable to create pipe.");
        exit(EXIT_FAILURE);
    }

    pid_t pid;
    if ((pid = fork()) < 0)
    {
        perror("Error: Unable to create child process.");
        exit(EXIT_FAILURE);
    }
    else if (pid == 0)
    {
        close(pipe2_fd[1]);
	if( to_file )
	{
		dup2(fileno(fptr), STDOUT_FILENO);
	}
        else 
		dup2(pipe_fd[1], STDOUT_FILENO);
        //dup2(pipe_fd[1], STDERR_FILENO);
        dup2(nfd, STDERR_FILENO);
	dup2(pipe2_fd[0], STDIN_FILENO);
        close(pipe_fd[1]);
        close(pipe2_fd[0]);
	int i=0;
	char *tok;
	char *args[256];
	char command[256]="";
	tok = strtok(input," ");
	strcat(command,tok);
	while(tok != NULL){
		args[i]=tok;
		tok = strtok(NULL," ");
		i++;
		if( tok != NULL && !strcmp(tok,">" ) ) break;
	}

	args[i] = 0;
	if( !strcmp(args[0],"printenv") ) 
	{
		printf("%s\n",grep_env(args[1]));
		exit(EXIT_SUCCESS);
	}
       	execvp(command,args);

        fprintf(stderr,"Unknown command: [%s].\n",args[0]);
        exit(EXIT_FAILURE);
    }
    else
    {

	close(pipe2_fd[0]);
        close(pipe_fd[1]);
	//char pipe_input_buffer[BUFFER_SIZE];
	//pop_ras_pipe(pipe_input_buffer);
	//printf("test\n");
	write(pipe2_fd[1],pipe_input_buffer,BUFFER_SIZE);
	//fprintf(pipe2_fd[1],pipe_input_buffer,BUFFER_SIZE);
	close(pipe2_fd[1]);
	//printf("test2\n");
	int status=0;
        wait(&status);
	read(pipe_fd[0],output,BUFFER_SIZE);
        close(pipe_fd[0]);
	//recv(pipe_fd[0],output,BUFFER_SIZE,MSG_WAITALL);
	if( to_file ) fclose(fptr);
	if(status > 0)
	{
		insert_ras_pipe(pipe_input_buffer,0);
		inc_ras_pipe();
		return EXIT_FAILURE;
	}
/*
	if( to_file )
	{
		FILE * fptr;
		fptr = fopen(file_name,"w");
		fprintf(fptr,"%s",output);
		fclose(fptr);
		bzero(output,strlen(output));
	}
*/  
    }
    return EXIT_SUCCESS;
}

void insert_ras_pipe(char *insert_buffer,int insert_counter)
{
        ras_pipe_t* new_node = malloc( sizeof(ras_pipe_t) );
        new_node->counter = insert_counter;
        new_node->buffer = malloc( BUFFER_SIZE*sizeof(char) );
        strcpy( new_node->buffer , insert_buffer );
        new_node->left = NULL;
        new_node->right = NULL;
        if( head == NULL )
        {
                head = new_node;
                tail = new_node;
        }
        else
        {
                ras_pipe_t *ptr = head;
                while( ptr->right != NULL ) ptr = ptr->right;
                ptr->right = new_node;
                new_node->left = ptr;
                tail = new_node;
        }
}

int pop_ras_pipe( char *ras_pipe_output )
{
        int is_pop = 0;
        ras_pipe_t *ptr = head;
        bzero(ras_pipe_output,BUFFER_SIZE);
        while( ptr != NULL )
        {
                ptr->counter--;
		if( ptr->counter == 0 )
                {
                        ras_pipe_t *tmp = ptr->right;
                        strcat(ras_pipe_output,ptr->buffer);
			if( ptr == head && ptr == tail ) 
			{
				head = NULL;
				tail = NULL;
			}
			else 
			{
                        	if( ptr == head ) head = ptr->right;
	                        if( ptr == tail ) tail = ptr->left;
	                        if( ptr->left ) ptr->left->right = ptr->right;
	                        if( ptr->right ) ptr->right->left = ptr->left;
			}
                        free(ptr->buffer);
                        free(ptr);
                        ptr = tmp;
                        is_pop = 1;

                }
                else ptr = ptr->right;
        }

        return is_pop;
}

void inc_ras_pipe()
{
        ras_pipe_t *ptr = head;
        while( ptr != NULL )
        {
                ptr->counter++;
                ptr = ptr->right;
        }
}

void printallenv()
{
	int i = 1;
	char *s = *environ;

	for (; s; i++) {
		printf("%s\n", s);
		s = *(environ+i);
	}
}

char *grep_env(char *pattern )
{
        int i = 1;
        char *s = *environ;
	//strcat(pattern,"=");
        for (; s; i++) {
                if( !strncmp(s,pattern,strlen(pattern)))
		{
			//return &s[strlen(pattern)];
			return s;
		}
                s = *(environ+i);
        }
	return NULL;
}
