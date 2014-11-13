#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
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

typedef struct {
    char* key;
    char* value;
}userenv;

typedef struct {
    int ID;
    int fd;
    char name[32];
    char IP[15];
    int port;
    int status;
    userenv env[128];
    int envnum;
} user;

int parsingCommand(plist* plptr, char *instr, char** out, int sock);

int execCommand(plist* plptr, char **argv, int args, char **outstr, const int sock );

int push_plist( plist *plptr, char* instr, int counter);

int pop_plist( plist* plptr, char** outstr);

int list_plist(plist *plptr);

int inc_counter_plist( plist* plptr);

void welcomeMessage( int servsockfd, int clisockfd, struct sockaddr_in *cli_addr );

void GlobalMessage( int servsockfd, char* message );

void usersetenv( user *ulist, int uid, char* key, char* value);

char *usergetenv( user *ulist, int uid, char* key );

int main( int argc, char *argv[] )
{
    int PORT_NO, servsockfd, clisockfd, clilen, isServing, bytesReceived;
    int status;
    int i;

    user *userlist = malloc(sizeof(user)*1024);
    int useridx = 0;

    for( i = 0 ; i < 1024 ; i++ )
    {
        userlist[i].status = 0;
    }

    plist **pipelist = malloc( 1024*sizeof(plist *) );
    for( i = 0 ; i < 1024 ; i++ )
    {
        pipelist[i] = malloc(sizeof(plist));
    }
    
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
                    for( i = 0 ; i < useridx ; i++ )
                    {
                        if( userlist[i].status == 0 )
                        {
                            userlist[i].ID = i;
                            userlist[i].fd = clisockfd;
                            userlist[i].port = cli_addr.sin_port;
                            strcpy(userlist[i].name,"(no name)");
                            strcpy(userlist[i].IP,inet_ntoa(cli_addr.sin_addr));
                            userlist[i].status = 1;
                            userlist[i].envnum = 0;
                            break;
                        }
                    }
                    if( i == useridx )
                    {
                        userlist[useridx].ID = useridx;
                        userlist[useridx].fd = clisockfd;
                        userlist[useridx].port = cli_addr.sin_port;
                        strcpy(userlist[useridx].name,"(no name)");
                        strcpy(userlist[useridx].IP,inet_ntoa(cli_addr.sin_addr));
                        userlist[useridx].status = 1;
                        userlist[i].envnum = 0;
                        useridx++;
                    }
                    send(clisockfd,"****************************************\n",41,0);
                    send(clisockfd,"** Welcome to the information server. **\n",41,0);
                    send(clisockfd,"****************************************\n",41,0);
                    welcomeMessage(servsockfd, clisockfd, &cli_addr);
                    send(clisockfd,"% ",2,0);
                }
                else
                {
                    char recvchar[1] = {0};
                    char *linebuff = malloc( 2*sizeof(char) );
                    int lenght = 1;
                    int recvret;
                    int i;
                    if( (recvret = recv( fdptr, recvchar, 1, 0 )) <= 0)
                    {
                        fprintf(stderr,"Recieve error code : %d\n",recvret );
                        char exitmessage[256];
                        for( i = 0 ; i < useridx ; i++ )
                        {
                            if( userlist[i].status == 1 && userlist[i].fd == fdptr )
                            {
                                sprintf(exitmessage,"*** User '%s' left. ***\n",userlist[i].name);
                                userlist[i].status = 0;
                                break;
                            }
                        }
                        close(fdptr);
                        FD_CLR(fdptr,&infd);
                        GlobalMessage(servsockfd,exitmessage);
                        break;
                    }
                    else
                    {
                        while( recvchar[0] != '\n' )
                        {
                            linebuff = realloc(linebuff, (lenght+1)*sizeof(char) );
                            linebuff[lenght-1] = recvchar[0];
                            lenght++;
                            recvchar[0] = 0;
                            recvchar[1] = 0;
                            if( (recvret = recv( fdptr, recvchar, 1, 0 ) <= 0) )
                            {
                                fprintf(stderr,"Recieve error code : %d\n",recvret );
                                char exitmessage[256];
                                for( i = 0 ; i < useridx ; i++ )
                                {
                                    if( userlist[i].status == 1 && userlist[i].fd == fdptr )
                                    {
                                        sprintf(exitmessage,"*** User '%s' left. ***\n",userlist[i].name);
                                        userlist[i].status = 0;
                                        break;
                                    }
                                }
                                close(fdptr);
                                FD_CLR(fdptr,&infd);
                                GlobalMessage(servsockfd,exitmessage);
                                break;
                            }
                        }
                        linebuff[lenght-1] = 0;
                        if( linebuff[0] == 0 )
                                send(fdptr,"% ",2,0);
                        if( recvret <= 0 )
                        {
                            if( linebuff[lenght-2] == '\r' ) linebuff[lenght-2] = 0;
                            printf("Recv : %s \n", linebuff);
                            if( strlen(linebuff) == 4 && !strncmp(linebuff,"exit",4) )
                            {
                                char exitmessage[256];
                                for( i = 0 ; i < useridx ; i++ )
                                {
                                    if( userlist[i].status == 1 && userlist[i].fd == fdptr )
                                    {
                                        sprintf(exitmessage,"*** User '%s' left. ***\n",userlist[i].name);
                                        userlist[i].status = 0;
                                        break;
                                    }
                                }
                                close(fdptr);
                                FD_CLR(fdptr,&infd);
                                GlobalMessage(servsockfd,exitmessage);
                            }
                            else 
                            {
                                if( !strcmp(linebuff,"who") )
                                {
                                    char whomessage[102400];
                                    sprintf(whomessage,"<ID>\t<nickname>\t<IP/port>\t<indicate me>\n");
                                    for( i = 0 ; i < useridx ; i++ )
                                    {
                                        if( userlist[i].status == 1 && userlist[i].fd == fdptr )
                                        {
                                            sprintf(whomessage,"%s%d\t%s\t%s/%d\t<-me\n",whomessage,userlist[i].ID,userlist[i].name,userlist[i].IP,userlist[i].port);
                                        }
                                        else if( userlist[i].status == 1 )
                                        {
                                            sprintf(whomessage,"%s%d\t%s\t%s/%d\n",whomessage,userlist[i].ID,userlist[i].name,userlist[i].IP,userlist[i].port);
                                        }
                                    }
                                    send( fdptr, whomessage, strlen(whomessage), 0 );
                                }
                                else if( !strncmp(linebuff,"name ",5) )
                                {
                                    char namemessage[256];
                                    int dupname = 0;
                                    for( i = 0 ; i < useridx ; i++ )
                                    {
                                        if( userlist[i].status == 1 )
                                        {
                                            if( !strcmp( userlist[i].name , &linebuff[5] ) )
                                            {
                                                dupname = 1;
                                                sprintf(namemessage,"*** User '%s' already exists. ***\n",&linebuff[5]);
                                                send(fdptr,namemessage,strlen(namemessage),0);
                                                break;
                                            }
                                        }
                                    }
                                    if( !dupname )
                                    {
                                        for( i = 0 ; i < useridx ; i++ )
                                        {
                                            if( userlist[i].status == 1 && userlist[i].fd == fdptr )
                                            {
                                                strcpy( userlist[i].name , &linebuff[5] );
                                                break;
                                            }
                                        }
                                        sprintf(namemessage,"*** User from %s/%d is named '%s'. ***\n",userlist[i].IP,userlist[i].port,userlist[i].name);
                                        GlobalMessage(servsockfd,namemessage);
                                    }
                                }
                                else if( !strncmp(linebuff,"yell ",5) )
                                {
                                    char yellmessage[102400];
                                    for( i = 0 ; i < useridx ; i++ )
                                    {
                                        if( userlist[i].status == 1 && userlist[i].fd == fdptr )
                                        {
                                            sprintf(yellmessage,"*** %s yelled ***: %s\n",userlist[i].name,&linebuff[5]);
                                            break;
                                        }
                                    }
                                    GlobalMessage(servsockfd,yellmessage);
                                }
                                else if( !strncmp(linebuff,"tell ",5) )
                                {
                                    char tellmessage[102400];
                                    for( i = 0 ; i < useridx ; i++ )
                                    {
                                        if( userlist[i].status == 1 && userlist[i].fd == fdptr )
                                        {
                                            break;
                                        }
                                    }
                                    int fromid = i;
                                    for( i = 5 ; linebuff[i] != ' ' ; i++ );
                                    linebuff[i] = 0;
                                    i++;
                                    int tellid = atoi(&linebuff[5]);
                                    if( userlist[tellid].status )
                                    {
                                        sprintf(tellmessage,"*** %s told you ***: %s\n",userlist[fromid].name,&linebuff[i]);
                                        send(userlist[tellid].fd,tellmessage,strlen(tellmessage),0);
                                    }
                                    else
                                    {
                                        sprintf(tellmessage,"*** Error: user #%d does not exist yet. ***\n",tellid);
                                        send(fdptr,tellmessage,strlen(tellmessage),0);
                                    }

                                }
                                else if( !strncmp(linebuff,"setenv ",7) )
                                {
                                    for( i = 0 ; i < useridx ; i++ )
                                    {
                                        if( userlist[i].status == 1 && userlist[i].fd == fdptr )
                                        {
                                            break;
                                        }
                                    }
                                    int uid = i;
                                    for( i = 7 ; linebuff[i] != ' ' ; i++ );
                                    linebuff[i] = 0;
                                    i++;
                                    usersetenv(userlist,uid,&linebuff[7],&linebuff[i]);
                                }
                                else if( !strncmp(linebuff,"printenv ",9) )
                                {
                                    char printenvmessage[1024];
                                    for( i = 0 ; i < useridx ; i++ )
                                    {
                                        if( userlist[i].status == 1 && userlist[i].fd == fdptr )
                                        {
                                            break;
                                        }
                                    }
                                    int uid = i;
                                    sprintf(printenvmessage,"%s=%s\n",&linebuff[9],usergetenv(userlist,uid,&linebuff[9]));
                                    send(fdptr,printenvmessage,strlen(printenvmessage),0);
                                }
                                else
                                {
                                    parsingCommand(pipelist[fdptr],linebuff,NULL,fdptr);
                                }
                                free(linebuff);
                                send(fdptr,"% ",2,0);
                            }
                        }
                    }
                }
            }
        }
    }
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
       // printf("i=%d\n",i);
       if( i == instrL )
       {
           char **output = malloc(sizeof(char*));
           *output = NULL;
           args++;
           argv = realloc ( argv , (args+1) * sizeof( char * ) );
           argv[args - 1] = malloc(sizeof(char)*tokL);
           argv[args] = NULL;
           memcpy(argv[args - 1],tok,tokL*sizeof(char));
        //   printf("arg[%d] = %s\n",args,argv[args - 1]);
           int tmpsock = sock;
        //   printf("%d\n",sock);
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
        //   printf("%d\n",sock);
           if(*output!=NULL)
           {
        //       printf("send : \n%s\n",*output);
               send(sock,*output,strlen(*output),0);
               free(*output);
           }
        //   printf("send end\n");
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
        //   printf("PC=%d\n",pipecounter);
           int tmpsock = sock;
           if( execCommand(plptr,argv,args,output,sock) != 0 )
           {
               sock = tmpsock;
               while( args > 0 )
               {
        //        printf("free %s.\n",argv[args-1]);
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
         //      printf("free %s.\n",argv[args-1]);
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
        //   printf("arg[%d] = %s\n",args,argv[args - 1]);
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

void welcomeMessage( int servsockfd, int clisockfd, struct sockaddr_in* cli_addr )
{
    int fdptr;
    char message[1024];
    sprintf(message,"*** User '(no name)' entered from %s/%d. ***\n",inet_ntoa(cli_addr->sin_addr), cli_addr->sin_port);
    for( fdptr = 0 ; fdptr < FD_SETSIZE ; fdptr++ )
    {
        if( fdptr != servsockfd ) send( fdptr, message, strlen(message), 0);
    }
}

void GlobalMessage( int servsockfd, char* message )
{
    int fdptr;
    for( fdptr = 0 ; fdptr < FD_SETSIZE ; fdptr++ )
    {
        if( fdptr != servsockfd ) send( fdptr, message, strlen(message), 0);
    }
}

void usersetenv( user *ulist, int uid, char* key, char* value)
{
    int i;
    for(  i = 0 ; i < ulist[uid].envnum ; i ++ )
    {
        if( ulist[uid].env[i].key != NULL && !strcmp(ulist[uid].env[i].key,key) )
        {
            ulist[uid].env[i].value = malloc( sizeof(char)*(strlen(value)+1) );
            strcpy(ulist[uid].env[i].value,value);
            ulist[uid].env[i].value[strlen(value)] = 0;
            return ;
        }
    }
    ulist[uid].env[i].key = malloc( sizeof(char)*(strlen(key)+1) );
    strcpy(ulist[uid].env[i].key,key);
    ulist[uid].env[i].key[strlen(key)] = 0;
    ulist[uid].env[i].value = malloc( sizeof(char)*(strlen(value)+1) );
    strcpy(ulist[uid].env[i].value,value);
    ulist[uid].env[i].value[strlen(value)] = 0;
    ulist[uid].envnum++;
}

char *usergetenv( user *ulist, int uid, char* key )
{
    int i;
    for(  i = 0 ; i < ulist[uid].envnum ; i ++ )
    {
        if( ulist[uid].env[i].key != NULL && !strcmp(ulist[uid].env[i].key,key) )
        {
            return ulist[uid].env[i].value;
        }
    }
    return NULL;
}
