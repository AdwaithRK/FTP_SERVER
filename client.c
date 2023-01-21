// gcc client.c -o client
// ./client <ip_add> <port>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <arpa/inet.h>
#include <sys/stat.h>

#include <sys/sendfile.h>
#include <fcntl.h>

int socket_fd = 0, n = 0;
int choice;
char choice_str[1000];
char filename[20], buf[100], ext[20], command[20];
FILE *fp;
int filehandle;
struct stat obj;
int size, status, i = 1;
char *f;
int already_exits = 0;
int overwirte_choice = 1;
char *pos;
int num_lines;

void putInFileServer()
{
    printf("enter the filename to put in server\n");
    scanf("%s", filename); // read the file name
    if (access(filename, F_OK) < 0)
    {
        printf(" %s : not such file to send !!!! \n", filename);
        return;
    }
    filehandle = open(filename, O_RDONLY);
    strcpy(buf, "put ");
    strcat(buf, filename);
    write(socket_fd, buf, 100); // send put command with filename
    read(socket_fd, &already_exits, sizeof(int));
    if (already_exits)
    {
        printf("same name file already exits in server 1. overwirte 2.NO overwirte\n"); // file is already exits
        scanf("%d", &overwirte_choice);
    }

    write(socket_fd, &overwirte_choice, sizeof(int)); // sending overwrite choice

    if (overwirte_choice == 1)
    {
        stat(filename, &obj);
        size = obj.st_size;
        write(socket_fd, &size, sizeof(int));
        sendfile(socket_fd, filehandle, NULL, size); // sending file
        recv(socket_fd, &status, sizeof(int), 0);
        if (status)
            printf("%s File stored successfully\n", filename); // status
        else
            printf("%s File failed to be stored to remote machine\n", filename);
    }
}

void getInFileServer()
{
    printf("Enter filename to get: ");
    scanf("%s", filename);
    strcpy(buf, "get ");
    strcat(buf, filename);

    send(socket_fd, buf, 100, 0); // send the get command with file name

    recv(socket_fd, &size, sizeof(int), 0);
    if (!size)
    {
        printf("No such file on the remote directory\n\n"); // file doesn't exits
        return;
    }

    if (access(filename, F_OK) != -1)
    {
        already_exits = 1;
        printf("same name file already exits in client 1. overwirte 2.NO overwirte\n"); // file already exits
        scanf("%d", &overwirte_choice);
    }
    send(socket_fd, &overwirte_choice, sizeof(int), 0);

    if (already_exits == 1 && overwirte_choice == 1)
    {
        filehandle = open(filename, O_WRONLY | O_CREAT | O_TRUNC, 644); // open file with all clear data
        f = malloc(size);
        recv(socket_fd, f, size, 0);
        write(filehandle, f, size);
        close(filehandle);
    }
    else if (already_exits == 0 && overwirte_choice == 1)
    {
        filehandle = open(filename, O_CREAT | O_EXCL | O_WRONLY, 0666); // open new file
        f = malloc(size);
        recv(socket_fd, f, size, 0);
        write(filehandle, f, size);
        close(filehandle);
    }
}

void mputInFileServer()
{
    printf("enter the extension , you want to put in server:\n");
    scanf("%s", ext); //  take the extionsion

    strcpy(command, "ls *.");
    strcat(command, ext);
    strcat(command, " > temp.txt"); // store all file list
    // printf("%s\n",command);
    system(command);

    char *line = NULL; // intilize file var
    size_t len = 0;
    ssize_t read;
    FILE *fp = fopen("temp.txt", "r");
    while ((read = getline(&line, &len, fp)) != -1) // read input
    {
        if ((pos = strchr(line, '\n')) != NULL)
            *pos = '\0';

        filehandle = open(line, O_RDONLY); // open files line wise
        strcpy(buf, "put ");
        strcat(buf, line);

        send(socket_fd, buf, 100, 0);
        recv(socket_fd, &already_exits, sizeof(int), 0);
        if (already_exits)
        {
            printf("%s file already exits in server 1. overwirte 2.NO overwirte\n", line); // overwrite option for that particular file
            scanf("%d", &overwirte_choice);
        }
        send(socket_fd, &overwirte_choice, sizeof(int), 0); // sending overwrite choice
        if (overwirte_choice == 1)
        {
            stat(line, &obj);
            size = obj.st_size;
            send(socket_fd, &size, sizeof(int), 0);
            sendfile(socket_fd, filehandle, NULL, size);
            recv(socket_fd, &status, sizeof(int), 0);
            if (status) // status
                printf("%s stored successfully\n", line);
            else
                printf("%s failed to be stored to remote machine\n", line);
        }
        overwirte_choice = 1; // re-assign overwrite choice
    }                         // end of while
    fclose(fp);               // close the file
    remove("temp.txt");
}

void mgetInFileServer()
{
    printf("enter the extension , you want to get from server:\n");
    scanf("%s", ext); // take input the files extension
    strcpy(buf, "mget ");
    strcat(buf, ext);
    send(socket_fd, buf, 100, 0);                // sending buffer with choice and extension
    recv(socket_fd, &num_lines, sizeof(int), 0); // get number of files

    while (num_lines > 0)
    {

        recv(socket_fd, filename, 20, 0);       // recv file name
        recv(socket_fd, &size, sizeof(int), 0); // recv the size of file
        if (!size)
        {
            printf("No such file on the remote directory\n\n"); // error handling
            break;
        }

        if (access(filename, F_OK) != -1) // checking if already exits or not
        {
            already_exits = 1;
            printf("%s file already exits in client 1. overwirte 2.NO overwirte\n", filename);
            scanf("%d", &overwirte_choice); // taking overwirte choice
        }
        send(socket_fd, &overwirte_choice, sizeof(int), 0); // sending overwrite choice

        if (already_exits == 1 && overwirte_choice == 1) // option according to the choice
        {
            filehandle = open(filename, O_WRONLY | O_CREAT | O_TRUNC, 644); // clear all the file
            f = malloc(size);
            recv(socket_fd, f, size, 0);
            write(filehandle, f, size); // send file
            close(filehandle);
        }
        else if (already_exits == 0 && overwirte_choice == 1)
        {
            filehandle = open(filename, O_CREAT | O_EXCL | O_WRONLY, 0666); // open new file
            f = malloc(size);
            recv(socket_fd, f, size, 0);
            write(filehandle, f, size);
            close(filehandle);
        }
        overwirte_choice = 1;
        already_exits = 0;
        num_lines--;
    }
}

void removeConnection()
{
    strcpy(buf, "quit");
    send(socket_fd, buf, 100, 0); // sending quit command for closing both server and client
    recv(socket_fd, &status, 100, 0);
    if (status)
    {
        printf("Server closed\nQuitting..\n");
        exit(0);
    }
    printf("Server failed to close connection\n"); // faild to quit
    return;
}

int main(int argc, char *argv[])
{
    // char recvBuff[1024];
    struct sockaddr_in server_address;

    if (argc != 3)
    {
        // printf("\n Usage: %s <ip of server> \n", argv[0]); // checking argument
        printf("\n proper ip and port required!!!\n");
        return 1;
    }

    // memset(recvBuff, '0', sizeof(recvBuff));
    if ((socket_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        printf("\n Client socket creation failed \n"); // socket create
        return 1;
    }

    // memset(&server_address, '0', sizeof(server_address)); // assigning  server address

    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(atoi(argv[2])); // assigning port

    if (inet_pton(AF_INET, argv[1], &server_address.sin_addr) <= 0)
    {
        printf("\n inet_pton error occured\n");
        return 1;
    }

    if (connect(socket_fd, (struct sockaddr *)&server_address, sizeof(server_address)) < 0) // connect to the server
    {
        printf("\n Can't connect with server!!! \n");
        return 1;
    }

    printf("here here");

    while (1)
    {
        printf("Enter a choice:\n1- put\n2- get\n 3- mput\n 4-mget\n 5-quit\n");

        fgets(choice_str, 1000, stdin);

        choice_str[strlen(choice_str) - 1] = '\0';
        if (strlen(choice_str) > 1)
        {
            printf("error\n");
            continue;
        }
        choice = atoi(choice_str);
        // scanf("%d", &choice);

        switch (choice)
        {

            //--------put file in server----------------//
        case 1:
            putInFileServer();
            break;
            //----------------get file from server-------------------//
        case 2:
            getInFileServer();
            break;
            //------------------quit the server-------------------------//
        case 5:
            removeConnection();
            //------------------mput server ----------------------------------//
        case 3:
            mputInFileServer();
            break;
            //----------------mget server------------------------------------//
        case 4:
            mgetInFileServer();
            break;

            //------------- default choice --------------------//
        default:
            printf("choose the vaild option\n");
            break;

        } // end of switch

    } // end of while

    return 0;
} // end of main