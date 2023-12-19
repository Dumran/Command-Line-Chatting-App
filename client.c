#include "chat.h"

// Global variables
volatile sig_atomic_t flag = 0;
int sockfd = 0;
char name[MAX_USER];

void    error(char *msg) {
    fprintf(stderr, "%s", msg);
    exit (1);
}

//Trims \n
void str_trim_lf (char* arr, int length) {
  int i;
  for (i = 0; i < length; i++) { // trim \n
    if (arr[i] == '\n') {
      arr[i] = '\0';
      break;
    }
  }
}

void catch_ctrl_c_and_exit(int sig) {
    flag = 1;
}

//Reads socket and sends messages to server
void send_msg_handler() {
  char message[MAX_MSG] = {};
	char buffer[MAX_MSG] = {};

  while(1) {
    fgets(message, MAX_MSG, stdin);
    str_trim_lf(message, MAX_MSG);

    if (strcmp(message, "exit") == 0) {
		  break;
    } else {
      sprintf(buffer, "%s", message);
      send(sockfd, buffer, strlen(buffer), 0);
    }
		bzero(message, MAX_MSG);
    bzero(buffer, MAX_MSG);
  }
  catch_ctrl_c_and_exit(2);
}

//Recieves and prints messages from server
void recv_msg_handler() {
	char message[MAX_MSG] = {};

  while (1) {
		int receive = recv(sockfd, message, MAX_MSG, 0);
    if (receive > 0) {
      printf("%s", message);
    } else if (receive == 0) {
			break;
    }
		bzero(message, sizeof(message));
  }
}

int main(int argc, char **argv){
	if(argc != 2)
		error("Please enter port.\n");

	char *ip = "127.0.0.1";
	int port = atoi(argv[1]);
	struct sockaddr_in server_addr;

	signal(SIGINT, catch_ctrl_c_and_exit);

	//Socket settings
  sockfd = socket(AF_INET, SOCK_STREAM, 0);
  server_addr.sin_family = AF_INET;
  server_addr.sin_addr.s_addr = inet_addr(ip);
  server_addr.sin_port = htons(port);

  //Connect to Server
  int err = connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr));
  if (err == -1)
		error("Error accured while connecting\n");

	printf("\033[0;36mWelcome to chat. To login, type your username.\nUsername: \033[0m");

	pthread_t send_msg_thread;
  if(pthread_create(&send_msg_thread, NULL, (void *) send_msg_handler, NULL) != 0)
		error("Error on pthread\n");

	pthread_t recv_msg_thread;
  if(pthread_create(&recv_msg_thread, NULL, (void *) recv_msg_handler, NULL) != 0)
		error("Error on pthread\n");

	while (1){
		if(flag){
			printf("\033[0;36mSession ended. Goodbye! :)\033[0m\n");
			break;
    }
	}

	close(sockfd);
	return 0;
}