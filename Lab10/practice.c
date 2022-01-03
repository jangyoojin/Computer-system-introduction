#include<stdio.h>
#include<unistd.h>
#include<fcntl.h>
#include<string.h>

int main()
{
    char buf[256];
    int fd1 = open("1.txt", O_RDWR);
    int fd2 = open("2.txt", O_WRONLY | O_CREAT, 0777);
    int fd3 = dup(fd1);
    
    scanf("%s", buf);
    write(4, buf, strlen(buf));
    dup2(1, 4);
    write(fd2, buf, strlen(buf));
    return 0;
}

