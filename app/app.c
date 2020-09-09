#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<syscall.h>

#include<sys/types.h>
#include<sys/stat.h>
#include<sys/ioctl.h>
#include<fcntl.h>

#define DEV_NAME "/dev/stopwatch"

int main(int argc, char *argv[]){


    int fd; 

    fd = open(DEV_NAME, O_WRONLY);

    if(fd<0){
        fprintf(stderr, "Device open failed!!\n");
    }

    write(fd, NULL, 0);
    close(fd);

    return 0;
}
