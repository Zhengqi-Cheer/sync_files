#include "dameon.h"

#define buf_len  (MAX_DIR_NUM*(sizeof(struct inotify_event) + NAME_MAX + 1))

char readbuf[buf_len] __attribute__ ((aligned(__alignof__(struct inotify_event))));
int _sock = 0;
int inotfy_fd = 0;
struct dir_link *head_link = NULL;  //目录监控头指针

void event_printf(struct inotify_event *event)
{
            /*
                IN_ACCESS
                IN_MODIFY
                IN_ATTRIB
                IN_CLOSE_WRITE
                IN_CLOSE_NOWRITE
                IN_OPEN
                IN_MOVED_FROM
                IN_MOVED_TO
                IN_CREATE
                IN_DELETE
                IN_DELETE_SELF
                IN_MOVE_SELF
                IN_UNMOUNT
                IN_CLOSE
                IN_MOVE
             */
            if (event->mask & IN_ACCESS) {
                printf("IN_ACCESS\n");
            } else if (event->mask & IN_MOVE) {
                printf("IN_MOVE\n");
            } else if (event->mask & IN_CLOSE) {
                printf("IN_CLOSE\n");
            } else if (event->mask & IN_UNMOUNT) {
                printf("IN_UNMOUNT\n");
            } else if (event->mask & IN_MOVE_SELF) {
                printf("IN_MOVE_SELF\n");
            } else if (event->mask & IN_DELETE_SELF) {
                printf("IN_DELETE_SELF\n");
            } else if (event->mask & IN_DELETE) {
                printf("IN_DELETE\n");
            } else if (event->mask & IN_CREATE) {
                printf("IN_CREATE\n");
            } else if (event->mask & IN_MOVED_TO) {
                printf("IN_MOVED_TO\n");
            } else if (event->mask & IN_MOVED_FROM) {
                printf("IN_MOVED_FROM\n");
            } else if (event->mask & IN_OPEN) {
                printf("IN_OPEN\n");
            } else if (event->mask & IN_CLOSE_NOWRITE) {
                printf("IN_CLOSE_NOWRITE\n");
            } else if (event->mask & IN_CLOSE_WRITE) {
                printf("IN_CLOSE_WRITE\n");
            } else if (event->mask & IN_ATTRIB) {
                printf("IN_ATTRIB\n");
            } else if (event->mask & IN_MODIFY) {
                printf("IN_MODIFY\n");
            }

            printf("event-->name:%s\n",event -> name);
            printf("event-->wd:%d\n",event -> wd);
            printf("---------------------------------\n");
}

//主程序
int main(void)
{   
    //自定义信号
    inotfy_fd = inotify_init();
    if (inotfy_fd == -1) {
        perror("inotify init error");
        exit(-1);
    }

    if ((_sock = init_tcp()) == -1 ) {
        perror("sock error");
        exit (-1);
    }

    signal(SIGUSR1, signal_do);

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
            
            event_printf(event);
            do_event(inotfy_fd, event, head_link, _event_P);
        } 
        memset(readbuf, 0, sizeof(readbuf));
    }
    return 0;
}




