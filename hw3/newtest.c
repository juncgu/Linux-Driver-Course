#include <unistd.h>
#include <sys/types.h>
#include <errno.h>
#include <sys/wait.h>
#include<stdio.h>
#include<stdlib.h>
#include<fcntl.h>
#include <pthread.h>

#define DEVICE "/dev/mycdrv%d"
	
pthread_t thread1, thread2;
//pthread_mutex_t count_mutex;


void *test(void *arg) 
{
int i,fd, offset=0;
	char ch, write_buf[100], read_buf[100];
    char deviceStr[20];

	int *deviceNum;
	deviceNum=(int *)arg;

    sprintf(deviceStr,DEVICE,*deviceNum);

	fd = open(deviceStr, O_RDWR);
	if(fd == -1)
	{
		printf("File %s either does not exist or has been locked by another process\n", deviceStr);
		exit(-1);
	}
	printf(" r = read from device\n w = write to device \n enter command :");
	scanf("%c", &ch);
	switch(ch) {
		case 'w':
			printf("Enter Data to write: ");
			scanf(" %[^\n]", write_buf);
			write(fd, write_buf, sizeof(write_buf));//, &offset);
			break;

		case 'r':
			read(fd, read_buf, sizeof(read_buf));//,&offset);
			printf("device: %s\n", read_buf);
			break;

		default:
			printf("Doing default read");
			read(fd, read_buf, sizeof(read_buf));//,&offset);
			printf("\ndevice: %s\n", read_buf);
			break;
			break;

	}
	close(fd);
	return 0;
}

int main(int argc, char *argv[])
{
	int deviceNum;

	printf(" Enter the device number: ");
    scanf("%d", &deviceNum);
    getchar(); 

    pthread_create(&thread1, NULL, test, &deviceNum);
    pthread_create(&thread2, NULL, test, &deviceNum);


    pthread_join(thread1, NULL);
    pthread_join(thread2, NULL);


   return EXIT_SUCCESS;
}
