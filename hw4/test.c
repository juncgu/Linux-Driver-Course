#include <linux/ioctl.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>

// A sample userapp. Adapt the code to be used for reverse lseek for using reverse and out of bound checking.

#define MYCDRV_IOC_MAGIC 'Z'
#define ASP_CHGACCDIR  _IO(MYCDRV_IOC_MAGIC, 1)

int f0, f1, f2;
int nTHread = 20;
pthread_t threads[nTHread];

void *test(void *arg)
{
    int size = 20;
	char buf[size];
    int id = (int)arg;

    memset(buf, 0, size);
    sprintf(buf, "Thread id is %2d\n", id);
    write(f0, buf, 16);
    sprintf(buf, "Thread id is %2d\n", id);
    write(f1, buf, 16);
    sprintf(buf, "Thread id is %2d\n", id);
    write(f2, buf, 16);

    if (id == 0) {
    lseek(f0, 0, SEEK_SET);
    puts("\nphase2");
    }

    if (id == 1) {
    lseek(f1, 0, SEEK_SET);
    puts("\nphase2");
    }

    if (id == 2) {
    lseek(f2, 0, SEEK_SET);
    puts("\nphase2");
    }

    memset(buf, 0, size);
    read(f0, buf, size);
    printf("Data read from mycdrv0 : %s", buf);
    memset(buf, 0, size);
    read(f0, buf, size);
    printf("Data read from mycdrv0 : %s", buf);
    memset(buf, 0, size);
    read(f0, buf, size);
    printf("Data read from mycdrv0 : %s", buf);

    // out of bounds check
    memset(buf, 0, size);
    lseek(f0, 2000, SEEK_SET);
    read(f0, buf, size);
    printf("%s", buf);

	return 0;
}

int main(int argc, char *argv[]) {

    int i,rc;
    char buf[1024] = {0};
    char buf2[1024] = {0};
    int dir = 0;

    f0 = open("/dev/mycdrv0", O_RDWR);
    if (f0 < 0)
    	{ perror("mycdrv0 could not be opened");
    	  return -1;}
    f1 = open("/dev/mycdrv1", O_RDWR);
    if (f1 < 0)
    	{ perror("mycdrv1 could not be opened");
    	close(f0);
    	return -1;}
    f2 = open("/dev/mycdrv2", O_RDWR);
    if (f2 < 0)
    	{ perror("mycdrv2 could not be opened");
    	close(f0); close(f1);
    	return -1;}

        rc = ioctl(f0, ASP_CHGACCDIR, dir);
        if (rc == -1)
        	{ perror("\n***error in ioctl***\n");
        	  return -1; }

        if (write(f0, buf, 1024) != 1024)
        { perror ("Error writing mycdrv0");
        close(f0); close(f1); close(f2);
        return -1;}

        if (write(f1, buf, 1024) != 1024)
        { perror ("Error writing mycdrv1");
        close(f0); close(f1); close(f2);
        return -1;}

        if (write(f2, buf, 1024) != 1024)
        { perror ("Error writing mycdrv2");
        close(f0); close(f1); close(f2);
        	return -1;}

        for (i=0; i<1024; i++)
            buf[i] = rand()%256;

        lseek(f0, 0, SEEK_SET);
        if (write(f0, buf, 1024) != 1024)
        { perror ("Error writing mycdrv0");
        close(f0); close(f1); close(f2);
        	return -1;}

        lseek(f1, 0, SEEK_SET);
        if (write(f1, buf, 1024) != 1024)
        { perror ("Error writing mycdrv1");
        close(f0); close(f1); close(f2);
        	return -1;}

        lseek(f2, 0, SEEK_SET);
        if (write(f2, buf, 1024) != 1024)
        { perror ("Error writing mycdrv2");
        close(f0); close(f1); close(f2);
        	return -1;}

        lseek(f0, 0, SEEK_SET);
        for (i=0; i<nTHread; i++)
            pthread_create(&(threads[i]), NULL, test, (void*)i);

        for (i=0; i<nTHread; i++)
            pthread_join(threads[i], NULL);

    close(f0);
    close(f1);
    close(f2);

    return 0;
}
