#include <unistd.h>
#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>


#define PORT 8877 //server port
#define SIZE 1024  //all buff size
#define OUT 0  //directory move out 
#define FROM 1 //directory move from 

int init_sock (void);
void add_file(int c_fd, char* file_name);
void rm_file(char *file_name);
void mk_dir(char *dir_path);
void rm_dir(char *dir_path);
void mk_linkfile(char *file_name);
void mv_dir(int num,char *dir_path);
void hard_file(char *S_path);
