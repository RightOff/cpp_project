#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>

void error_handling(char* message);

void test01()
{
	int fd;
	char buf[]="Let's go!\n";
	
	fd=open("data.txt", O_CREAT|O_WRONLY|O_TRUNC);
	if(fd==-1)
		error_handling("open() error!");
	printf("file descriptor: %d \n", fd);

	if(write(fd, buf, sizeof(buf))==-1)
		error_handling("write() error!");

	close(fd);
}
//用底层IO编写
void test02()
{
	int fd1,fd2;
	int count = 0;
	char buf[20];
	fd1 = open("data.txt",O_RDONLY);
	if(fd1==-1)
		error_handling("open() error!");
	fd2 = open("data_copy.txt",O_CREAT | O_WRONLY);
	if (fd2 == -1)
    error_handling("open() error!");
	while((count=read(fd1,buf,20)) > 0)
	{
		write(fd2,buf,count);
	}
	close(fd1);
	close(fd2);
}
//用ANSI标准IO编写
void test03()
{
	FILE* fin;
	FILE* fout;
	char buf[20];
	int count;

	fin = fopen("data.txt", "r");
	if (fin == NULL)   
    	error_handling("open() error!");
	fout =fopen("data_copy2","aw");
	if (NULL == fout)  
    error_handling("open() error!");

	while((count=fread(buf,1,20,fin))>0)
		fwrite(buf,1,count,fout);
	fclose(fin);
	fclose(fout);
}

int main(void)
{

	test01();
	test02();
	test03();
	

	return 0;
}

void error_handling(char* message)
{
	fputs(message, stderr);
	fputc('\n', stderr);
	exit(1);
}

/*
root@com:/home/swyoon/tcpip# gcc low_open.c -o lopen
root@com:/home/swyoon/tcpip# ./lopen
file descriptor: 3 
root@com:/home/swyoon/tcpip# cat data.txt
Let's go!
root@com:/home/swyoon/tcpip# 
*/
