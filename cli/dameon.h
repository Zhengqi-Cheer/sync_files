#include <unistd.h>
#include <dirent.h>
#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/inotify.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <error.h>
#include <errno.h>

#define PORT 8877 
#define IP "172.16.163.146"   //service ip
#define MAX_DIR_NUM 30
#define SIZE 1024
#define OUT  0 
#define FROM 1
#define H_LINK 1

#define SYNC_PATH "./../test"
#define SYNC_FILE "/home/zhengquan/share/git/mession_3/cli/sync.result"

struct hard_link_list{
    int inode;
    char *path;
    struct hard_link_list *next;
};

struct dir_link{
    int wd;
    char *dir_path;
    struct dir_link *next;
};
struct event_list{
    int inode;
    char *cmd ;
    char *do_path;
    struct event_list *next;
};

extern  struct event_list *_event_P ;
extern  int _sock;

int   init_tcp(void);
void  mk_file(char* file_name,int sock);  //发送同步文件命令
void  rm_file(char* file_name,int sock);  //发送删除文件命令
void  mk_dir (char *dir_path,int sock);   //发送创建目录命令
void  rm_dir(char *dir_path,int sock);      //发送删除目录命令
void  mk_linkfile(char *dir_path,int sock);//发送链接文件命令
void  mv_dir(int num,char *dir_path,int sock);
void  hard_link(int sock,char *O_path,char *S_path);
void  hard_list(int inode,char *path);

void  first_event (int i_fd,char *dir_path,int num,struct dir_link *path_link,int sock); //程序运行的第一件事：同步指定路径的文件夹

void  add_link (struct dir_link *head,int wd, char *add_path);
int   delete_link( struct dir_link *head,char *dir_path);

void  add_list(int flag ,struct event_list *head ,char *cmd,char *do_path);
void  delete_list(struct event_list *head,char *do_path);

void  do_event(int fd,struct inotify_event *event,struct dir_link *head,struct event_list *event_head);;
void  signal_do(int num);
void  p_list();
void  p_dir(struct dir_link *dir_head);
void  journal_write(char *write_buf);
int   _if_linkfile(char *filename);
char* seek_h_Source_file(char* seek_dir,int inode,char *file_name);
