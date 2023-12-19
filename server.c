#include "chat.h"

//Global Variables
static _Atomic unsigned int cli_count = 0;
static _Atomic unsigned int msg_count = 0;
static int uid = 10;
static char    messagebox[MSG_CAPACITY][MAX_MSG];
client_t *clients[MAX_CLIENTS];
pthread_mutex_t clients_mutex = PTHREAD_MUTEX_INITIALIZER;

//Trims \n
void str_trim_lf (char* arr, int length) {
  int i;
  for (i = 0; i < length; i++) {
    if (arr[i] == '\n') {
      arr[i] = '\0';
      break;
    }
  }
}

void    error(char *msg) {
    fprintf(stderr, "%s", msg);
    exit (1);
}

//Crops recipient
char    *take_msg(char *msg) {
    char    *temp;
    int i = 0, j = 0;

    temp = malloc(MAX_MSG);
    if (!temp)
        return 0;
    while (msg[i] != ' ' && msg[i])
        i++;
    i++;
    while (msg[i + j]) {
        temp[j] = msg[i + j];
        j++;
    }
    temp[j] = '\0';
    free (msg);
    return (temp);
}

//Crops reciever from message
char    *take_reciever(char *msg) {
    int i = 0, j = 0;
    char    *reciever;

    reciever = malloc(MAX_USER);
    if (!reciever)
        return (0);
    while (msg[i] != ' ' && msg[i]) {
        reciever[i] = msg[i];
        i++;
    }
    reciever[i] = '\0';
    return (reciever);
}

//Fetches recievers name for messagebox
char    *find_user(const char *msg) {
    int i=0, j=0;
    char    *user;

    user = (char *)malloc(MAX_USER);
    if (!user)
        return (0);
    while (msg[i] && j != 8) {
        if (msg[i++] == ' ')
            j++;
    }
    j = 0;
    while (msg[i + j] != ':') {
        user[j] = msg[i + j];
        j++;
    }
    user[j] = '\0';
    return (user);
}

//Takes current time and date
char    *current_time() {
    char    *buffer;

    buffer = calloc(200, sizeof(char));
    if (!buffer)
        return (0);
    time_t t = time(NULL);
    struct tm tm = *localtime(&t);
    strftime(buffer, 200, "%c", &tm);
    return (buffer);
}

//Add clients to queue
void queue_add(client_t *cl) {
	pthread_mutex_lock(&clients_mutex);

	for(int i=0; i < MAX_CLIENTS; ++i){
		if(!clients[i]){
			clients[i] = cl;
			break;
		}
	}

	pthread_mutex_unlock(&clients_mutex);
}

//Remove clients from queue
void queue_remove(int uid){
	pthread_mutex_lock(&clients_mutex);

	for(int i=0; i < MAX_CLIENTS; ++i){
		if(clients[i]){
			if(clients[i]->uid == uid){
				clients[i] = NULL;
				break;
			}
		}
	}

	pthread_mutex_unlock(&clients_mutex);
}

//Send message to wished client
void send_message(char *s, int uid){
	pthread_mutex_lock(&clients_mutex);

	for(int i=0; i<MAX_CLIENTS; ++i){
		if(clients[i]){
			if(clients[i]->uid == uid){
				if(write(clients[i]->sockfd, s, strlen(s)) < 0){
					perror("Error, writing to descriptor failed\n");
					break;
				}
			}
		}
	}
	pthread_mutex_unlock(&clients_mutex);
}

//Reads messagebox
void read_messages(client_t *cli) {
	int	i = 0, flag = 0;

	while (i < msg_count) {
		if (strcmp(find_user(messagebox[i]), cli->name) == 0) {
        	send_message(messagebox[i], cli->uid);
			flag = 1;
        }
        i++;
    }

	if (flag == 0)
		send_message("\033[0;36mNo new messages.\033[0m\n", cli->uid);
}

//Fetches reciever's id
int find_reciever(const char *name) {
    int i=0;

    i = 0;
    while (clients[i]) {
        if (clients[i]) {
            if (strcmp(clients[i]->name, name) == 0) {
                return (clients[i]->uid);
            }
        }
        i++;
    }
    return (0);
}

//Handle all communication with the client
void *handle_client(void *arg) {
	char *buff_out;
	char message[MAX_MSG];
	char name[MAX_USER];
	char *reciever;
	int leave_flag = 0;

	cli_count++;
	client_t *cli = (client_t *)arg;
	buff_out = malloc(MAX_MSG);
	if (!buff_out)
		error("Malloc\n");
	// Name
	if (recv(cli->sockfd, name, MAX_USER, 0) <= 0 || strlen(name) <  2 || strlen(name) >= MAX_USER-1){
		printf("Didn't enter the name.\n");
		leave_flag = 1;
	} else {
		strcpy(cli->name, name);
		sprintf(buff_out, "%s has joined\n", cli->name);
		printf("%s", buff_out);
		send_message("\033[0;36mLogin succesfull.\nChecking inbox:\033[0m\n", cli->uid);
		read_messages(cli);
		send_message("\033[0;36mInbox got checked.\nTo send a message, enter recipient's name and your message.\nTo exit, type exit.\033[0m\n", cli->uid);
	}

	bzero(buff_out, MAX_MSG);

	while(1){
		if (leave_flag) {
			break;
		}

		int receive = recv(cli->sockfd, buff_out, MAX_MSG, 0);
		if (receive > 0){
			if(strlen(buff_out) > 0) {
				str_trim_lf(buff_out, strlen(buff_out));
				reciever = take_reciever(buff_out);
				buff_out = take_msg(buff_out);
				sprintf(message, "%s %s to %s: %s\n", current_time(), cli->name, reciever, buff_out);
				send_message("\033[0;36mMessage successfuly sent\033[0m\n", cli->uid);
				printf("%s", message);
				if (find_reciever(reciever) == 0) {
					strcpy(messagebox[msg_count++], message);
				} else
					send_message(message, find_reciever(reciever));
			}
		} else if (receive == 0 || strcmp(buff_out, "exit") == 0) {
			sprintf(buff_out, "%s has left\n", cli->name);
			printf("%s", buff_out);
			leave_flag = 1;
		} else {
			printf("Error\n");
			leave_flag = 1;
		}
		bzero(message, MAX_MSG);
		bzero(buff_out, MAX_MSG);
	}

	close(cli->sockfd);
	queue_remove(cli->uid);
	free(cli);
	cli_count--;
	pthread_detach(pthread_self());

	return NULL;
}

int main(int argc, char **argv){
	if(argc != 2)
		error("Please enter port.\n");

	char *ip = "127.0.0.1";
	int port = atoi(argv[1]);
	int listenfd = 0, connfd = 0, i = 0;
	struct sockaddr_in serv_addr, cli_addr;
	pthread_t tid;

	//Socket settings
	listenfd = socket(AF_INET, SOCK_STREAM, 0);
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = inet_addr(ip);
	serv_addr.sin_port = htons(port);


	//Binding
	if(bind(listenfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0)
		error("Socket binding failed\n");

	//Listen
	if (listen(listenfd, 10) < 0)
    	error("Socket listening failed\n");

	printf("Server is online.\n");

	while(1){
		socklen_t clilen = sizeof(cli_addr);
		connfd = accept(listenfd, (struct sockaddr*)&cli_addr, &clilen);

		//Check if max clients is reached
		if((cli_count + 1) == MAX_CLIENTS){
			printf("Max clients reached. Rejected: ");
			close(connfd);
			continue;
		}

		// Client settings
		client_t *cli = (client_t *)malloc(sizeof(client_t));
		cli->address = cli_addr;
		cli->sockfd = connfd;
		cli->uid = uid++;

		// Add client to the queue and fork thread
		queue_add(cli);
		pthread_create(&tid, NULL, &handle_client, (void*)cli);
		
		sleep(1);
	}

	return 0;
}