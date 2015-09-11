/*************************
 *EEL5934 ADV.SYSTEM.PROG
 *Juncheng Gu 5191-0572
 *Assignment2 Multi-thread
 *Feb 1, 2015
 *************************/
#include<stdlib.h>
#include<stdio.h>
#include<string.h>
#include<unistd.h>
#include<fcntl.h>
#include<pthread.h>
#include<linux/ioctl.h>
#include<sys/ioctl.h>
#include<time.h>
#include<signal.h>
#include<semaphore.h>

#define CDRV_IOC_MAGIC 'Z'
#define E2_IOCMODE1 _IOWR(CDRV_IOC_MAGIC, 1, int)
#define E2_IOCMODE2 _IOWR(CDRV_IOC_MAGIC, 2, int)

/**************************** global variable *******************************/
pthread_t pth1, pth2; 
/*************************** end of global variable ************************/
/****************************** worker ************************************/
void *pth1_handler(void *t){
    int fd_1;
    printf("***pth1 starts\n");
    printf("***pth1 wants to open device, e2_open()\n");
    fd_1 = open("/dev/a5", O_RDWR);
    printf("***pth1 opens device, e2_open()\n");
    if (fd_1<0){
        return 0;
    }
    sleep(5);

    printf("***pth1 wants to close device, e2_release()\n");
    close(fd_1);
    printf("***pth1 closes device, e2_release()\n");

    printf("***pth1... exit!\n");
    pthread_exit(NULL);
}


void *pth2_handler(void *t){
    int fd_2;
    int rc;
    sleep(2);
    printf("---pth2 starts\n");
    printf("---pth2 wants to open device, e2_open()\n");
    fd_2 = open("/dev/a5", O_RDWR);
    printf("---pth2 opens device, e2_open()\n");
    if (fd_2<0){
        return 0;
    }
    printf("---pth2 wants to do IOCMODE1, e2_ioctl()\n");
    rc = ioctl(fd_2, E2_IOCMODE1, 1);
    printf("---pth2 finishs IOCMODE1, e2_ioctl()\n");

    sleep(5); 
    close(fd_2);

    printf("---pth2... exit!\n");
    pthread_exit(NULL);
}

/****************************** end of worker *****************************/

int main(int argc, char *argv[]){
    void *status1, *status2;
    int fd, ctl, failed;
    int data1=0, data2=0;
    fd = open("/dev/a5", O_RDWR);
    ctl = ioctl(fd, E2_IOCMODE2, 2);
    close(fd);
    printf("/dev/a5 is switched to MODE2\n");

    failed = pthread_create(&pth1, NULL, pth1_handler, &data1);
    if (failed) {
        printf("thread_create failed!\n");
        return -1;
    }
    failed = pthread_create(&pth2, NULL, pth2_handler, &data2);
    if (failed) {
        printf("thread_create failed!\n");
        return -1;
    }
   
    pthread_join(pth1, &status1);
    pthread_join(pth2, &status2);
    pthread_exit(NULL);
}

