#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ipc.h> 
#include <sys/shm.h> 
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <signal.h>
#include <unistd.h>

#define useridx (*useridxptr)

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
} userenv;

typedef struct {
    int from;
    char message[1024];
} mespipe;

typedef struct {
    int ID;
    int fd;
    char name[32];
    char IP[15];
    int port;
    int status;
    userenv env[128];
    int envnum;
    mespipe mpbuff[10];
    int pipemesnum;
    char mesg[65535];
} user;

int parsingCommand( user* ulist, plist* plptr, char *instr, char** out, int serversock, int sock, int myuid );

int execCommand( user* ulist, plist* plptr, char **argv, int args, char **outstr, const int sock, int myuid, char** retmessage);

int push_plist( plist *plptr, char* instr, int counter);

int pop_plist( plist* plptr, char** outstr);

int list_plist(plist *plptr);

int inc_counter_plist( plist* plptr);

void welcomeMessage( int servsockfd, int clisockfd, struct sockaddr_in *cli_addr, user* ulist );

void GlobalMessage( int servsockfd, char* message, user* ulist );

void usersetenv( user *ulist, int uid, char* key, char* value);

char *usergetenv( user *ulist, int uid, char* key );

//static user **ulptr;

int main( int argc, char *argv[] )
{
    int PORT_NO, servsockfd, clisockfd, clilen, isServing, bytesReceived;
    int status;
    int i;
    
//    ulptr = mmap(NULL, sizeof(*ulptr), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
//    *ulptr = malloc(sizeof(user)*1024);
    key_t shkey = 1160901;
    key_t shkey2 = 1160902;
    int shmid = shmget(shkey, 1024*sizeof(user), SHM_R|SHM_W|IPC_CREAT);
    if( shmid < 0 )
    {
   //     perror("shared memory error");
    }

    printf("%d %d\n",shmid,1024*sizeof(user));
    user *userlist = shmat(shmid, NULL ,0);

    printf("%x\n",userlist);
    int shmid2 = shmget(shkey2, sizeof(int), SHM_R|SHM_W|IPC_CREAT);
    if( shmid2 < 0 )
    {
        perror("shared memory error");
    }
    int *useridxptr = shmat(shmid2, NULL ,0);
    *useridxptr = 1;

    printf("uidx %d\n",useridx);
    //userlist = malloc(1024*sizeof(user));
//    user tmplist[1024];
//    user *userlist = mmap(NULL, sizeof(tmplist), PROT_READ | PROT_WRITE, MAP_SHARED |              MAP_ANONYMOUS, -1, 0);
//    printf("%d\n",sizeof(userlist));    
//
//    int useridx = 1;

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
    
    pid_t PID;
    pid_t PID2;

    struct sockaddr_in serv_addr, cli_addr;
    char homedir[strlen("HOME")+4];
    strcpy(homedir,getenv("HOME"));
    strcat(homedir,"/ras");
    chdir(homedir);
    printf("home %s\n",homedir);
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


    while( isServing )
    {
        clisockfd = accept(servsockfd, (struct sockaddr *)&cli_addr,&clilen);


        if( servsockfd < 0 )
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
                int fdptr = clisockfd;
//                shmid = shmget(shkey, 1024*sizeof(user), SHM_R|SHM_W);
//                userlist = shmat(shmid, NULL ,0);
                useridxptr = shmat(shmid2, NULL ,0);
                printf("uidx %d\n",useridx);
                printf("online %d\n",userlist[1].status);
                printf("Client Accepted : %d\n",clisockfd);
                for( i = 1 ; i < useridx ; i++ )
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
                        int j ;
                        for( j = 0 ; j < 10 ; j ++ )
                        {
                            userlist[i].mpbuff[j].from = -1;
                            bzero(userlist[i].mpbuff[j].message,1024);
                        }
                        userlist[i].pipemesnum = 0;
                        break;
                    }
                    else printf("fd %d\n",userlist[i].fd);
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
                    int j ;
                    for( j = 0 ; j < 10 ; j ++ )
                    {
                        userlist[i].mpbuff[j].from = -1;
                        bzero(userlist[i].mpbuff[j].message,1024);
                    }
                    userlist[i].pipemesnum = 0;
                    useridx++;
                    printf("fd %d\n",userlist[i].fd);
                }
            int myid = i;
            if( (PID2 = fork()) < 0 )
            {
                perror("Error: Unable to create child process.");
            }
            else if( PID2 == 0 )
            {
                char tmp[65535];
                while(1)
                {
                    if( userlist[myid].status == 0 )exit(0);
                    if( strlen(userlist[myid].mesg) != 0 )
                    {
                        strcpy( tmp , userlist[myid].mesg );
                        printf("sending ==> : %s\n",tmp);
                        send(userlist[myid].fd,tmp,strlen(tmp),0);
                        bzero(userlist[myid].mesg,65535);
                    }
                }
                exit(0);
            }
            if( (PID2 = fork()) < 0 )
            {
                perror("Error: Unable to create child process.");
            }
            else if( PID2 == 0 )
            {
/*
                int fdptr = clisockfd;
//                shmid = shmget(shkey, 1024*sizeof(user), SHM_R|SHM_W);
//                userlist = shmat(shmid, NULL ,0);
                useridxptr = shmat(shmid2, NULL ,0);
                printf("uidx %d\n",useridx);
                printf("online %d\n",userlist[1].status);
                printf("Client Accepted : %d\n",clisockfd);
                for( i = 1 ; i < useridx ; i++ )
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
                        int j ;
                        for( j = 0 ; j < 10 ; j ++ )
                        {
                            userlist[i].mpbuff[j].from = -1;
                            bzero(userlist[i].mpbuff[j].message,1024);
                        }
                        userlist[i].pipemesnum = 0;
                        break;
                    }
                    else printf("fd %d\n",userlist[i].fd);
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
                    int j ;
                    for( j = 0 ; j < 10 ; j ++ )
                    {
                        userlist[i].mpbuff[j].from = -1;
                        bzero(userlist[i].mpbuff[j].message,1024);
                    }
                    userlist[i].pipemesnum = 0;
                    useridx++;
                    printf("fd %d\n",userlist[i].fd);
                }
                */
                usersetenv(userlist,i,"PATH","bin:.");
                send(clisockfd,"****************************************\n",41,0);
                send(clisockfd,"** Welcome to the information server. **\n",41,0);
                send(clisockfd,"****************************************\n",41,0);
                welcomeMessage(servsockfd, clisockfd, &cli_addr, userlist);
                while(1)
                {
                    send(clisockfd,"% ",2,0);
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
                        GlobalMessage(servsockfd,exitmessage,userlist);
                        exit(0);
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
                                    if( userlist[i].status == 1 && userlist[i].ID == myid)
                                    {
                                        sprintf(exitmessage,"*** User '%s' left. ***\n",userlist[i].name);
                                        userlist[i].status = 0;
                                        break;
                                    }
                                }
                                close(fdptr);
                                GlobalMessage(servsockfd,exitmessage,userlist);
                                exit(0);
                                break;
                            }
                        }
                        linebuff[lenght-1] = 0;
//                        if( linebuff[0] == 0 )
//                                send(fdptr,"% ",2,0);
                        if( recvret <= 0 )
                        {
                            if( linebuff[lenght-2] == '\r' ) linebuff[lenght-2] = 0;
                            printf("Recv : %s \n", linebuff);
                            if( !strncmp(linebuff,"exit",4) )
                            {
                                char exitmessage[256];
                                printf("A\n");
                                for( i = 0 ; i < useridx ; i++ )
                                {
                                    if( userlist[i].status == 1 && userlist[i].ID == myid )
                                    {
                                        sprintf(exitmessage,"*** User '%s' left. ***\n",userlist[i].name);
                                        GlobalMessage(servsockfd,exitmessage,userlist);
                                        userlist[i].status = 0;
                                        break;
                                    }
                                }
                                for( i = 0 ; i < useridx ; i++ )
                                {
                                    if( userlist[i].status == 1 )
                                    {
                                        int j;
                                        for ( j = 0 ; j < 10 ; j ++ )
                                        {
                                            if( userlist[i].mpbuff[j].from == myid )
                                            {
                                                userlist[i].mpbuff[j].from = -1;
                                                bzero(userlist[i].mpbuff[j].message,1024);
                                            }
                                        }
                                    }
                                } 
                                printf("B\n");
                                close(fdptr);
                                printf("C\n");
                                exit(0);
                            }
                            else 
                            {
                                if( !strcmp(linebuff,"who") )
                                {
                                    char whomessage[102400];
                                    sprintf(whomessage,"<ID>\t<nickname>\t<IP/port>\t<indicate me>\n");
                                    printf("useridx %d\n",useridx);
                                    for( i = 0 ; i < useridx ; i++ )
                                    {
                                        if( userlist[i].status == 1 && userlist[i].ID == myid )
                                        {
                                            sprintf(whomessage,"%s%d\t%s\t%s/%d\t<-me\n",whomessage,userlist[i].ID,userlist[i].name,userlist[i].IP,userlist[i].port);
                                        }
                                        else if( userlist[i].status == 1 )
                                        {
                                            sprintf(whomessage,"%s%d\t%s\t%s/%d\n",whomessage,userlist[i].ID,userlist[i].name,userlist[i].IP,userlist[i].port);
                                        }
                                    }
                                    send( fdptr, whomessage, strlen(whomessage), 0 );
                                    char **input = malloc( sizeof(char*));
                                    *input = NULL;
                                    pop_plist(pipelist[fdptr],input) ;
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
                                            if( userlist[i].status == 1 && userlist[i].ID == myid )
                                            {
                                                strcpy( userlist[i].name , &linebuff[5] );
                                                break;
                                            }
                                        }
                                        sprintf(namemessage,"*** User from %s/%d is named '%s'. ***\n",userlist[i].IP,userlist[i].port,userlist[i].name);
                                        char **input = malloc( sizeof(char*));
                                        *input = NULL;
                                        pop_plist(pipelist[fdptr],input) ;
                                        GlobalMessage(servsockfd,namemessage,userlist);
                                        while( strlen(userlist[myid].mesg) != 0 );
                                    }
                                }
                                else if( !strncmp(linebuff,"yell ",5) )
                                {
                                    char yellmessage[102400];
                                    for( i = 0 ; i < useridx ; i++ )
                                    {
                                        if( userlist[i].status == 1 && userlist[i].ID == myid )
                                        {
                                            sprintf(yellmessage,"*** %s yelled ***: %s\n",userlist[i].name,&linebuff[5]);
                                            break;
                                        }
                                    }
                                    char **input = malloc( sizeof(char*));
                                    *input = NULL;
                                    pop_plist(pipelist[fdptr],input) ;
                                    GlobalMessage(servsockfd,yellmessage,userlist);
                                        while( strlen(userlist[myid].mesg) != 0 );
                                }
                                else if( !strncmp(linebuff,"tell ",5) )
                                {
                                    char tellmessage[102400];
                                    for( i = 0 ; i < useridx ; i++ )
                                    {
                                        if( userlist[i].status == 1 && userlist[i].ID == myid )
                                        {
                                            break;
                                        }
                                    }
                                    int fromid = i;
                                    for( i = 5 ; linebuff[i] != ' ' ; i++ );
                                    linebuff[i] = 0;
                                    i++;
                                    int tellid = atoi(&linebuff[5]) ;
                                    if( userlist[tellid].status )
                                    {
                                        sprintf(tellmessage,"*** %s told you ***: %s\n",userlist[fromid].name,&linebuff[i]);
                                        //send(userlist[tellid].fd,tellmessage,strlen(tellmessage),0);
                                        strcat(userlist[tellid].mesg,tellmessage);
                                        char **input = malloc( sizeof(char*));
                                        *input = NULL;
                                        pop_plist(pipelist[fdptr],input) ;
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
                                        if( userlist[i].status == 1 && userlist[i].ID == myid )
                                        {
                                            break;
                                        }
                                    }
                                    int uid = i;
                                    for( i = 7 ; linebuff[i] != ' ' ; i++ );
                                    linebuff[i] = 0;
                                    i++;
                                    usersetenv(userlist,uid,&linebuff[7],&linebuff[i]);
                                    char **input = malloc( sizeof(char*));
                                    *input = NULL;
                                    pop_plist(pipelist[fdptr],input) ;
                                }
                                else if( !strncmp(linebuff,"printenv ",9) )
                                {
                                    char printenvmessage[1024];
                                    for( i = 0 ; i < useridx ; i++ )
                                    {
                                        if( userlist[i].status == 1 && userlist[i].ID == myid )
                                        {
                                            break;
                                        }
                                    }
                                    int uid = i;
                                    sprintf(printenvmessage,"%s=%s\n",&linebuff[9],usergetenv(userlist,uid,&linebuff[9]));
                                    send(fdptr,printenvmessage,strlen(printenvmessage),0);
                                    char **input = malloc( sizeof(char*));
                                    *input = NULL;
                                    pop_plist(pipelist[fdptr],input) ;
                                }
                                else
                                {
                                    for( i = 1 ; i < useridx ; i++ )
                                    {
                                        if( userlist[i].status == 1 && userlist[i].ID == myid )
                                        {
                                            break;
                                        }
                                    }
                                    parsingCommand(userlist,pipelist[fdptr],linebuff,NULL,servsockfd,fdptr,i);
                                }
                                free(linebuff);
                                //send(fdptr,"% ",2,0);
                            }
                        }
                    }
                }
            }
            else
            {
                exit(0);
            }
        }
        else
        {
            close(clisockfd);
            waitpid(PID,&status,NULL);
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

int parsingCommand( user* ulist, plist* plptr, char *instr, char** out, int serversock, int sock, int myuid )
{
    printf("start parsing\n");
    int instrL = strlen(instr);
    int i;
    int args = 0;
    int tokL = 1;
    char *tok = malloc(2*sizeof(char));
    char **argv = malloc(sizeof(char));
    char **retmessage = malloc(sizeof(char*));
    *retmessage = NULL;
    tok[0] = 0;
    argv[0] = NULL;
    
    for( i = 0 ; i <= instrL ; i++ )
    {
       if( i == instrL )
       {
            printf("end parsing\n");
           //if( tok  NULL && tok[0] == 0 )
           char **output = malloc(sizeof(char*));
           *output = NULL;
           if( tok != NULL && tok[0] != 0 )
           {
               args++;
               argv = realloc ( argv , (args+1) * sizeof( char * ) );
               argv[args - 1] = malloc(sizeof(char)*tokL);
               argv[args] = NULL;
               memcpy(argv[args - 1],tok,tokL*sizeof(char));
           }
           
           int tmpsock = sock;

           if( *retmessage != NULL ) free(*retmessage);
           *retmessage = NULL;
           if(1)
           {
               if( execCommand(ulist,plptr,argv,args,output,sock,myuid,retmessage) != 0 )
               {
                   sock = tmpsock;
                   if(*output != NULL )free(*output);
                   free(tok);
                   free(output);
                   break;
               }
           }
           sock = tmpsock;
           if(*output!=NULL)
           {
               send(sock,*output,strlen(*output),0);
               free(*output);
           }

           if( *retmessage != NULL ) GlobalMessage(serversock,*retmessage,ulist);
                                        while( strlen(ulist[myuid].mesg) != 0 );
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
           int tmpsock = sock;
           if( *retmessage != NULL ) free(*retmessage);
           *retmessage = NULL;
           if( execCommand(ulist,plptr,argv,args,output,sock,myuid,retmessage) != 0 )
           {
               sock = tmpsock;
               while( args > 0 )
               {
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
           
           if( *retmessage != NULL ) GlobalMessage(serversock,*retmessage,ulist);
                                        while( strlen(ulist[myuid].mesg) != 0 );

           while( args > 0 )
           {
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
           if( i+1  == instrL ) break;
       }
       else if( instr[i] == ' ' || instr[i] == '\t' )
       {
           if( tok == NULL || tok[0] == 0 ) continue;
           args++;
           argv = realloc ( argv , (args+1) * sizeof( char * ) );
           argv[args - 1] = malloc(sizeof(char)*tokL);
           memcpy(argv[args - 1],tok,tokL*sizeof(char));
           argv[args] = NULL;
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

int execCommand( user* ulist, plist* plptr, char **argv, int args, char **outstr, const int sock, int myuid, char** retmessage )
{
    char** testarg = argv;
    int testi = 0;
    for( testi = 0 ; testarg[testi] != NULL ; testi ++ )
    {
        printf("%d='%s'\n",testi,testarg[testi]);
    }
    if( !strcmp(argv[0],"setenv") )
    {
        setenv(argv[1],argv[2],1);
        return EXIT_SUCCESS;
    }
    int pipe1[2], pipe2[2];
    int iii , tofile = 0, touser = 0, fromuser = 0, touid, fromuid;
    char filename[256];
    FILE *fptr = NULL;
        tofile = 0;
        touser = 0;
        fromuser = 0;
        for( iii = 0 ; iii < args ; iii++ )
        {
            if( argv[iii] == NULL )
            {
                break;
            }
            else if( argv[iii][0] == '>' && argv[iii][1] == 0 )
            {
                printf("A status\n");
                strcpy(filename,argv[iii+1]);
                argv[iii] = NULL;
                tofile = 1;
                continue;
            }
            else if( argv[iii][0] == '>' )
            {
                printf("B status\n");
                touser = 1;
                touid = atoi(&argv[iii][1]);
                argv[iii] = NULL;
                continue;
            }
            if ( argv[iii][0] == '<' && argv[iii][1] != 0 )
            {
                fromuser = 1;
                fromuid = atoi(&argv[iii][1]);
                argv[iii] = NULL;
            }
        }
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
        int i;
        for( i = 0 ; i < ulist[myuid].envnum ; i++ )
        {
            setenv(ulist[myuid].env[i].key,ulist[myuid].env[i].value,1);
        }
        close(pipe2[1]);
        close(pipe1[0]);

        dup2(pipe2[0],STDIN_FILENO);
        if( tofile )
        {
            fptr = fopen(filename,"w");
            dup2(fileno(fptr),STDOUT_FILENO);
            close(fileno(fptr));
            dup2(sock,STDERR_FILENO);
        }
        else
        {
            dup2(pipe1[1],STDOUT_FILENO);
            if( touser ) dup2(pipe1[1],STDERR_FILENO);
            else dup2(sock,STDERR_FILENO);
        }

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
        int ispop = pop_plist(plptr,input) ;
        if( fromuser )
        {
            printf("Recieving messages from user#%d to #%d\n",fromuid,myuid);
            int i;
            for( i = 0 ; i < 10 ; i++ )
            {
                if( ulist[myuid].mpbuff[i].from == fromuid )
                {
                    *retmessage = malloc(sizeof(char)*1024);
                    sprintf(*retmessage,"*** %s (#%d) just received from %s (#%d) by '",ulist[myuid].name,myuid,ulist[fromuid].name,fromuid);
                    printf("Get : %s\n",ulist[myuid].mpbuff[i].message);
                    write(pipe2[1],ulist[myuid].mpbuff[i].message,1024);
                    ulist[myuid].mpbuff[i].from = -1;
                    bzero(ulist[myuid].mpbuff[i].message,1024);
                    ulist[touid].pipemesnum--;
                    int j ;
                    for( j = 0 ; argv[j] != NULL ; j++ )
                    {
                        strcat(*retmessage,argv[j]);
                        strcat(*retmessage," ");
                    }
                    sprintf(*retmessage,"%s<%d' ***\n",*retmessage,fromuid);
                    break;
                }
            }
            if( i == 10 )
            {
                char errmessage[256];
                sprintf(errmessage,"*** Error: the pipe #%d->#%d does not exist yet. ***\n",fromuid,myuid);
                send(ulist[myuid].fd,errmessage,strlen(errmessage),0);
                close(pipe2[1]);
                wait(&status);
                free(output);
                push_plist(plptr,*input,0);
                inc_counter_plist(plptr);
                return EXIT_FAILURE;
            }
        }
        else if( ispop )
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
        if( touser )
        {
            if( ulist[touid].status )
            {
                printf("piping to user ....\n");
                int i;
                for( i = 0 ; i < 10 ; i++ )
                    if ( ulist[touid].mpbuff[i].from == myuid )
                    {
                        char errmessage[256];
                        sprintf(errmessage,"*** Error: the pipe #%d->#%d already exists. ***\n",myuid,touid);
                        send(ulist[myuid].fd,errmessage,strlen(errmessage),0);
                        free(output);
                        push_plist(plptr,*input,0);
                        inc_counter_plist(plptr);
                        return EXIT_FAILURE;
                    }
                for( i = 0 ; i < 10 ; i++ )
                    if ( ulist[touid].mpbuff[i].from == -1 ) break;
                if( *retmessage == NULL ) *retmessage = malloc(sizeof(char)*1024);
                sprintf(*retmessage,"*** %s (#%d) just piped '",ulist[myuid].name,myuid);
                int j ;
                for( j = 0 ; argv[j] != NULL ; j++ )
                {
                    strcat(*retmessage,argv[j]);
                    strcat(*retmessage," ");
                }
                sprintf(*retmessage,"%s>%d' to %s (#%d) ***\n",*retmessage,touid,ulist[touid].name,touid);
                strncpy(ulist[touid].mpbuff[i].message,output,1024);
                ulist[touid].mpbuff[i].from = myuid;
                ulist[touid].pipemesnum++;
                printf("Completed in %d buffer! %s\n",i,ulist[touid].mpbuff[i].message);
            }
            else
            {
                char errmessage[256];
                sprintf(errmessage,"*** Error: user #%d does not exist yet. ***\n",touid);
                send(ulist[myuid].fd,errmessage,strlen(errmessage),0);
                free(output);
                push_plist(plptr,*input,0);
                inc_counter_plist(plptr);
                return EXIT_FAILURE;
            }
        }
        else
        {
            *outstr = malloc((oL+1)*sizeof(char));
            memcpy(*outstr,output,(oL+1)*sizeof(char));
        }
        printf("output :%d\n%s\n",sock,*outstr);
        free(output);
        return EXIT_SUCCESS;
    }
    return EXIT_SUCCESS;
}

void welcomeMessage( int servsockfd, int clisockfd, struct sockaddr_in* cli_addr, user* ulist )
{
    int fdptr;
    char message[1024];
    sprintf(message,"*** User '(no name)' entered from %s/%d. ***\n",inet_ntoa(cli_addr->sin_addr), cli_addr->sin_port);
    GlobalMessage(servsockfd,message,ulist );
}

void GlobalMessage( int servsockfd, char* message, user* ulist )
{
    int i;
    for( i = 1 ; i < 1024 ; i ++ )
    {
        if( ulist[i].status == 1 && strlen(ulist[i].mesg) == 0 )
        {
            strcat( ulist[i].mesg, message );
        }
        else if ( ulist[i].status == 1 )
        {
            i--;
        }
    }
/*    int fdptr;
    for( fdptr = 0 ; fdptr < FD_SETSIZE ; fdptr++ )
    {
        if( fdptr != servsockfd ) send( fdptr, message, strlen(message), 0);
    }
    */
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
