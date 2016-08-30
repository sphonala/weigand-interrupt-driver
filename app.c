#include<stdio.h>  
#include<sys/types.h>  
#include<sys/stat.h>  
#include<fcntl.h>  
#include<sys/select.h>  
#include<unistd.h>  
#include<signal.h>  
#include<string.h>  
#include "weigand-drv.h"
 int number,fd,fd2,fd3;

void sig_handler(int sig)  
{ 
 read(fd, &number, sizeof(unsigned int));
printf("\ncardno=%x",number);
number=0;
fflush(stdout);
}  





main()
{


int f_flags,a,pid,pid2; 
int gpio=47,errno=-1,gpio2=46;

fd=open("/dev/weigand",O_RDWR);  
    if(fd < 0){  
        printf("/dev/weigand open failed");  
        return -1;  
    }  




ioctl( fd, GPIO_IOCTL_FREE, gpio );
ioctl( fd, GPIO_IOCTL_FREE, gpio2 );

GPIO_Request_t request;
request.gpio = gpio;
 strncpy( request.label,"user-gpio-key", sizeof( request.label ));
 request.label[ sizeof( request.label ) - 1 ] = '\0';

if ( ioctl( fd, GPIO_IOCTL_REQUEST, &request ) != 0 )
{
printf("gpio request error");
return -errno;
}
request.gpio = gpio2;
if ( ioctl( fd, GPIO_IOCTL_REQUEST, &request ) != 0 )
{
printf("gpio request error");
return -errno;
}




 if ( ioctl( fd, GPIO_IOCTL_DIRECTION_INPUT, gpio ) != 0 )
{
 printf("input set error");
}
if ( ioctl( fd, GPIO_IOCTL_DIRECTION_INPUT, gpio2 ) != 0 )
{
 printf("input set error");
}
 

request.gpio = gpio;
if ( ioctl( fd, GPIO_IOCTL_ISR, &request ) != 0 )
{
printf("gpio isr alredy set");
}
request.gpio = gpio2;
if ( ioctl( fd, GPIO_IOCTL_ISR, &request ) != 0 )
{
printf("gpio2 isr alredy set");
}



        signal(SIGCHLD, SIG_IGN);
                signal(SIGIO, sig_handler);
                fcntl(fd, F_SETOWN, getpid());
                f_flags = fcntl(fd, F_GETFL);
                printf("\nwaiting for weigand signal \n");
                fcntl(fd, F_SETFL, FASYNC | f_flags);
                while(1){
                sleep(1);}

}



