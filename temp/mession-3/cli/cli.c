#include "dameon.h"

#define buf_len  (MAX_DIR_NUM*(sizeof(struct inotify_event) + NAME_MAX + 1))

//char readbuf[buf_len] __attribute__ ((aligned(8))); 
char readbuf[buf_len] __attribute__ ((aligned(__alignof__(struct inotify_event))));


int _sock;

//主程序
int main(void)
{   //自定义信号
    int inotfy_fd = inotify_init1(IN_NONBLOCK);
    if (inotfy_fd == -1) {
        perror("inotify init error");
        exit(-1);
    }

    if ((_sock = init_tcp()) == -1 ) {
        perror("sock error");
        exit (-1);
    }

    signal(SIGUSR1, signal_do);

    struct dir_link *head_link = NULL;  //头指针
    struct dir_link *path_link = (struct dir_link *)malloc(sizeof(struct dir_link));//头节点
    (void)memset(path_link, 0, sizeof(struct dir_link));
    head_link = path_link;//指向首元节点
    printf("\nwaiting for first synchronization complete............. \n");
    first_event(inotfy_fd, SYNC_PATH, 1, head_link, _sock);
    printf("\nfirst ok\n");
    while (1) {
        int dir_read = read(inotfy_fd, readbuf, sizeof(readbuf));
        for (char *p = readbuf ; p < (readbuf+dir_read);) {   
            struct inotify_event *event;
            event = (struct inotify_event*)p;
            p+= sizeof(struct inotify_event) + event->len;
            if (strstr(event -> name, ".swx")) {
                continue;
            } else if (strstr(event -> name, ".swp")) {
                continue;
            } else if (!strcmp(event -> name, "4913")) {
                continue;
            } else if (strstr(event -> name, "~")) {
                continue;
            }
#if 0            
            if (event -> mask & (IN_ISDIR & IN_CREAT)) {
                char dir[SIZE] = {'\0'};
            }
#endif            
            do_event(inotfy_fd,event, head_link, _event_P);
        } 
        memset(readbuf, 0, sizeof(readbuf));
    }
    return 0;
}

