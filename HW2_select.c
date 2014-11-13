#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <signal.h>
#include <unistd.h>

typedef struct _pipe_node {
    int counter;
    char *str;
    struct _pipe_node* last;
    struct _pipe_node* next;
}   pipe_node;

typedef struct {
    pipe_node* head;
    pipe_node* tail;
} plist;

int parsingCommand(plist* plptr, char *instr, char** out, int sock);

int execCommand(plist* plptr, char **argv, int args, char **outstr, const int sock );

int push_plist( plist *plptr, char* instr, int counter);

int pop_plist( plist* plptr, char** outstr);

int list_plist(plist *plptr);

int inc_counter_plist( plist* plptr);

void welcomeMessage( int sockbuff[], int sockidx );

int main( int argc, char *argv[] )
{
    int PORT_NO, servsockfd, clisockfd, clilen, isServing, bytesReceived;
    int status;
    int sockbuff[1024];
    int sockidx = 0;

    int pipelist[1024];
    int pipeidx = 0;
    
    char HOMEDIR[256];
    
    fd_set infd, tmpfd;
    
    pid_t PID;
    pid_t PID2;

    struct sockaddr_in serv_addr, cli_addr;
    char homedir[strlen("HOME")+4];
    strcpy(homedir,getenv("HOME"));
    strcat(homedir,"/ras");
    chdir(homedir);

    if( argc != 2 ) PORT_NO = 5001;
    else PORT_NO = atoi(argv[1]);

    strcpy(HOMEDIR,getenv("HOME"));
    strcat(HOMEDIR,"/ras");
    
    servsockfd = socket(AF_INET, SOCK_STREAM, 0);
    if( servsockfd < 0 )
    {
        perror("ERROR: Unable to open socket.");
        exit(1);
    }
    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(PORT_NO);
    if(bind(servsockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0)
    {
        perror("ERROR: Unable to binding.");
        exit(1);
    }
    listen(servsockfd,32);
    clilen = sizeof(cli_addr);
    
    isServing = 1;

    FD_ZERO(&infd);
    FD_SET(servsockfd,&infd);

    while( isServing )
    {
        int fdptr;
        int selectret;
        tmpfd = infd;
        selectret = select(FD_SETSIZE, &tmpfd, (fd_set *)0,(fd_set *)0, (struct timeval *) 0);
        if( selectret < 0 )
        {
            fprintf(stderr,"Select error code : %d\n",selectret );
            exit(EXIT_FAILURE);
        }

        for( fdptr = 0 ; fdptr < FD_SETSIZE ; fdptr++ )
        {
            if( FD_ISSET(fdptr,&tmpfd) )
            {
                if( fdptr == servsockfd )
                {
                    clisockfd = accept( servsockfd, (struct sockaddr *)&cli_addr, &clilen );
                    FD_SET(clisockfd,&infd);
                    printf("Client Accepted : %d\n",clisockfd);
                    send(clisockfd,"% ",2,0);
                }
                else
                {
                    char recvchar[1] = {0};
                    char *linebuff = malloc( 2*sizeof(char) );
                    int lenght = 1;
                    int i;
                    if(recv( fdptr, recvchar, 1, 0 ) <= 0)
                    {
                        exit( EXIT_SUCCESS );
                    }
                    while( recvchar[0] != '\n' )
                    {
                        linebuff = realloc(linebuff, (lenght+1)*sizeof(char) );
                        linebuff[lenght-1] = recvchar[0];
                        lenght++;
                        recvchar[0] = 0;
                        recvchar[1] = 0;
                        if(recv( fdptr, recvchar, 1, 0 ) <= 0)
                        {
                            exit( EXIT_SUCCESS );
                        }
                    }
                    linebuff[lenght-1] = 0;
                    if( linebuff[lenght-2] == '\r' ) linebuff[lenght-2] = 0;
                    printf("Recv : %s \n", linebuff);
                    send(clisockfd,"% ",2,0);
                }
            }
        }
    }
/*
        while ( isServing ) 
    {
        newsockfd = accept(sockfd, (struct sockaddr *)&cli_addr,&clilen);
    
        if (newsockfd < 0)
        {
            perror("ERROR: Unable to accept.");
            exit(1);
        }
        
        if( ( PID = fork() ) < 0 )
        {
            perror("Error: Unable to create child process.");
        }
        else if( PID == 0 )
        {
            if( (PID2 = fork()) < 0 )
            {
                perror("Error: Unable to create child process.");
            }
            else if( PID2 == 0 )
            {
                close(sockfd);

                setenv("PATH","bin:.",1);
                printf("%s  %s\n",homedir,getenv("PATH"));
                plist* plptr = malloc(sizeof(plist));
                plptr->head = NULL;
                plptr->tail = NULL;
                send(newsockfd,"****************************************\n",41,0);
                send(newsockfd,"** Welcome to the information server. **\n",41,0);
                send(newsockfd,"****************************************\n",41,0);
                while ( 1 )
                {
                    char recvchar[1];
                    char *linebuff = malloc( 2*sizeof(char) );
                    int lenght = 1;
                    int i;
                    recvchar[0] = 0;
                    recvchar[1] = 0;
                    send(newsockfd,"% ",2,0);
                    if(recv( newsockfd, recvchar, 1, 0 ) <= 0)
                    {
                        exit( EXIT_SUCCESS );
                    }

                    while( recvchar[0] != '\n' )
                    {
                        linebuff = realloc(linebuff, (lenght+1)*sizeof(char) );
                        linebuff[lenght-1] = recvchar[0];
                        lenght++;
                        recvchar[0] = 0;
                        recvchar[1] = 0;
                        if(recv( newsockfd, recvchar, 1, 0 ) <= 0)
                        {
                            exit( EXIT_SUCCESS );
                        }
                    }
                    linebuff[lenght-1] = 0;
                    if( linebuff[lenght-2] == '\r' ) linebuff[lenght-2] = 0;
                    printf("Recv : %s \n", linebuff);
                    if( !strncmp(linebuff,"exit",4) )
                    {
                        close(newsockfd);
                        exit(EXIT_SUCCESS);
                    }
                    parsingCommand(plptr,linebuff,NULL,newsockfd);
                    list_plist(plptr);
                    free(linebuff);
                }
            }
            else
            {
                exit(0);
            }
        }
        else
        {
            waitpid(PID,&status,NULL);
            welcomeMessage(sockbuff,sockidx);
            sockbuff[sockidx++] = newsockfd;
        }
        
    }
*/
    return 0;
}

int push_plist( plist *plptr, char* instr, int counter)
{
    if( instr == NULL ) return EXIT_FAILURE;
    pipe_node* newnode = malloc( sizeof( pipe_node ) );
    newnode->counter = counter;
    newnode->str = malloc( strlen(instr) * sizeof(char) );
    strcpy( newnode->str, instr );
    newnode->last = NULL;
    newnode->next = NULL;
    if( plptr->head == NULL )
    {
            plptr->head = newnode;
            plptr->tail = newnode;
    }
    else
    {
        pipe_node *ptr = plptr->tail;
        ptr->next = newnode;
        newnode->last = ptr;
        plptr-> tail = newnode;
    }
    return EXIT_SUCCESS;
}


int pop_plist( plist* plptr, char** outstr)
{
    int ispop = 0;
    pipe_node *ptr = plptr->head;
    if( *outstr != NULL ) free(*outstr);
    *outstr = malloc(sizeof(char));
    (*outstr)[0] = 0;
    while( ptr != NULL )
    {
        ptr->counter--;
        if( ptr->counter == 0 )
        {
            pipe_node *tmp = ptr->next;
            int newL = strlen(*outstr)+strlen(ptr->str);
            *outstr = realloc( *outstr ,  (newL+1) * sizeof(char));
            strcat(*outstr,ptr->str);
            if( ptr == plptr->head && ptr == plptr->tail )
            {
                plptr->head = NULL;
                plptr->tail = NULL;
            }
            else
            {
                if( ptr == plptr->head ) plptr->head = ptr->next;
                if( ptr == plptr->tail ) plptr->tail = ptr->last;
                if( ptr->last ) ptr->last->next = ptr->next;
                if( ptr->next ) ptr->next->last = ptr->last;
            }
            free(ptr->str);
            free(ptr);
            ptr = tmp;
            ispop = 1;
        }
        else ptr = ptr->next;
    }

    printf("pop:\n%s\n end\n",*outstr);
    return ispop;
}

int list_plist(plist *plptr)
{
    int i = 0;
    pipe_node* ptr = plptr->head;
    while( ptr != NULL )
    {
        printf("plist%d = %d %s\n",i++,ptr->counter,ptr->str);
        ptr = ptr->next;
    }
    return 0;

}

int inc_counter_plist( plist* plptr)
{
    pipe_node* ptr = plptr->head;
    while( ptr != NULL )
    {
        ptr->counter++;
        ptr = ptr->next;
    }
}

int parsingCommand(plist* plptr, char *instr, char** out, int sock )
{
    int instrL = strlen(instr);
    int i;
    int args = 0;
    int tokL = 1;
    char *tok = malloc(2*sizeof(char));
    char **argv = malloc(sizeof(char));
    tok[0] = 0;
    argv[0] = NULL;
    
    for( i = 0 ; i <= instrL ; i++ )
    {
        printf("i=%d\n",i);
       if( i == instrL )
       {
           char **output = malloc(sizeof(char*));
           *output = NULL;
           args++;
           argv = realloc ( argv , (args+1) * sizeof( char * ) );
           argv[args - 1] = malloc(sizeof(char)*tokL);
           argv[args] = NULL;
           memcpy(argv[args - 1],tok,tokL*sizeof(char));
           printf("arg[%d] = %s\n",args,argv[args - 1]);
           int tmpsock = sock;
           printf("%d\n",sock);
           if(strlen(tok)>0)
           {
              if( execCommand(plptr,argv,args,output,sock) != 0 )
              {
                  sock = tmpsock;
                  if(*output != NULL )free(*output);
                  free(tok);
                  free(output);
                  break;
              }
           }
           sock = tmpsock;
           printf("%d\n",sock);
           if(*output!=NULL)
           {
               printf("send : \n%s\n",*output);
               send(sock,*output,strlen(*output),0);
               free(*output);
           }
           printf("send end\n");
           free(tok);
           free(output);
           tok = malloc(2*sizeof(char));
           tokL = 1;
       }
       else if( instr[i] == '|' )
       {
           int pipecounter = 0;
           char **output = malloc(sizeof(char*));
           *output = NULL;
           if( i + 1 == instrL || instr[i+1] == ' ' || instr[i+1] == '\t' )
               pipecounter = 1;
           else
           {
               int j;
               i++;
               for( j = 0 ; instr[i] != ' ' && instr[i] != '\t'  && i != instrL ; j++, i ++ )
               {

               };
               char tmp[j];
               strncpy(tmp,&instr[i-j],j);
               pipecounter = atoi(tmp);
           }
           printf("PC=%d\n",pipecounter);
           int tmpsock = sock;
           if( execCommand(plptr,argv,args,output,sock) != 0 )
           {
               sock = tmpsock;
               while( args > 0 )
               {
                printf("free %s.\n",argv[args-1]);
                free(argv[args-1]);
                args--;
               }
               free(argv);
               argv = malloc( sizeof( char* ) );
               argv[0] = NULL;
               args = 0;
               free(tok);
               break;
           }
           sock = tmpsock;

           push_plist(plptr,*output,pipecounter);

           while( args > 0 )
           {
               printf("free %s.\n",argv[args-1]);
               free(argv[args-1]);
               args--;
           }
           free(argv);
           argv = malloc( sizeof( char* ) );
           argv[0] = NULL;
           args = 0;
           free(tok);
           tok = malloc(2*sizeof(char));
           tok[0] = 0;
           tok[1] = 0;
           tokL = 1;
           if( i  == instrL ) break;
       }
       else if( instr[i] == ' ' || instr[i] == '\t' )
       {
           if( tok == NULL || tok[0] == 0 ) continue;
           args++;
           argv = realloc ( argv , (args+1) * sizeof( char * ) );
           argv[args - 1] = malloc(sizeof(char)*tokL);
           memcpy(argv[args - 1],tok,tokL*sizeof(char));
           argv[args] = NULL;
           printf("arg[%d] = %s\n",args,argv[args - 1]);
           free(tok);
           tok = malloc(2*sizeof(char));
           tok[0] = 0;
           tok[1] = 0;
           tokL = 1;
       }
       else
       {
            tok = realloc( tok , (tokL+1) * sizeof(char) );
            tok[tokL - 1] = instr[i];
            tok[tokL] = 0;
            tokL++;
       }
    }
    return 0;
}

int execCommand(plist* plptr, char **argv, int args, char **outstr, const int sock )
{
    if( !strcmp(argv[0],"setenv") )
    {
        setenv(argv[1],argv[2],1);
        return EXIT_SUCCESS;
    }
    int pipe1[2], pipe2[2];
    int iii , tofile = 0;
    FILE *fptr = NULL;
    if(pipe(pipe1) == -1 )
    {
        perror("Error: Unable to create pipe.");
        exit(EXIT_FAILURE);
    }
    if(pipe(pipe2) == -1 )
    {
        perror("Error: Unable to create pipe.");
        exit(EXIT_FAILURE);
    }
    pid_t PID;
    if ((PID = fork()) < 0)
    {
        perror("Error: Unable to create child process.");
        exit(EXIT_FAILURE);
    }
    else if (PID == 0)
    {
        close(pipe2[1]);
        close(pipe1[0]);
        tofile = 0;
        for( iii = 0 ; iii < args ; iii++ )
        {
            if( argv[iii][0] == '>' )
            {
                argv[iii] = NULL;
                tofile = 1;
                break;
            }

        }

        if( tofile == 1 )
        {
            fptr = fopen(argv[iii+1],"w");
            dup2(fileno(fptr),STDOUT_FILENO);
            close(fileno(fptr));
        }
        else
        {
            dup2(pipe1[1],STDOUT_FILENO);
        }
        dup2(sock,STDERR_FILENO);
        dup2(pipe2[0],STDIN_FILENO);
        close(pipe1[1]);
        close(pipe2[0]);
        if( !strcmp(argv[0],"printenv") )
        {
            printf("%s=%s\n",argv[1],getenv(argv[1]));
            exit(EXIT_SUCCESS);
        }
        execvp(argv[0],argv);
        fprintf(stderr,"Unknown command: [%s].\n",argv[0]);
        exit(EXIT_FAILURE);
    }
    else
    {
        char tok[2];
        char *output = malloc(2*sizeof(char));
        char **input = malloc(sizeof(char*));
        *input = NULL;
        int oL = 0;
        int status = 0;
        close(pipe2[0]);
        close(pipe1[1]);
        printf("start pop\n");
        if( pop_plist(plptr,input) )
        {
            printf("pop : %s\n",*input);
            write(pipe2[1],*input,strlen(*input));
        }
        close(pipe2[1]);
        wait(&status);
        printf("st%d\n",status);
        if( status != 0 )
        {
            printf("Fail\n");
            push_plist(plptr,*input,0);
            inc_counter_plist(plptr);
            return EXIT_FAILURE;
        }
        while(read(pipe1[0],tok,1) > 0)
        {
            oL++;
            output = realloc(output,sizeof(char)*(oL+1));
            output[oL-1] = tok[0];
            tok[0] = 0;
        }
        output[oL] =0;
        close(pipe1[0]);
        if( *outstr != NULL ) free(*outstr);
        *outstr = malloc((oL+1)*sizeof(char));
        memcpy(*outstr,output,(oL+1)*sizeof(char));
        printf("output :%d\n%s\n",sock,*outstr);
        free(output);
        return EXIT_SUCCESS;
    }
    return EXIT_SUCCESS;
}

void welcomeMessage( int sockbuff[], int sockidx )
{
    int i;
    char message[1024];
    sprintf(message,"user [%d] is online.\n",sockidx);
    for( i = 0 ; i < sockidx ; i++ )
    {
        send( sockbuff[i], message, strlen(message), 0);
    }
}
