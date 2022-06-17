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
#define IP "172.16.163.158"   //service ip
#define MAX_DIR_NUM 1 
#define NICE -5   //nice, 优先级
#define SIZE 1024
#define OUT  0 
#define FROM 1
#define H_LINK 1

#define SYNC_PATH "./../uu"
#define SYNC_FILE "/home/zhengquan/share/git/mession_3/cli/sync.result"

//硬连接链表，就第一次用
struct hard_link_list{
    int    inode;
    char   *path;
    struct hard_link_list *next;
};
//监控目录链表
struct dir_link{
    int    wd;
    char   *dir_path;
    struct dir_link *next;
};

//事件链表
struct event_list{
    int    inode;
    char   *cmd ;
    char   *do_path;
    struct event_list *next;
};

extern struct dir_link *head_link;
extern struct event_list *_event_P ;
extern int _sock;
extern int inotfy_fd;

int   init_tcp(void);
void  mk_file(int flag,char* file_name,const int sock);  //发送同步文件命令
void  rm_file(char* file_name,const int sock);  //发送删除文件命令
void  mk_dir (char *dir_path,const int sock);   //发送创建目录命令
void  rm_dir(char *dir_path,const int sock);      //发送删除目录命令
void  mk_linkfile(char *dir_path,const int sock);//发送链接文件命令
void  mv_dir(int num,char *dir_path,const int sock);
void  hard_link(const int sock,char *O_path,char *S_path);
void  hard_list(int inode,char *path);

void  first_event (int i_fd,char *dir_path,int num,struct dir_link *path_link,const int sock); //程序运行的第一件事：同步指定路径的文件夹

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
int   if_hard_first(int flag,const int sock,char * file_name);
char* seek_h_Source_file(char* seek_dir,int inode,char *file_name);


