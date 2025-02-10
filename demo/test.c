#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <stdlib.h>

#define POINTER 0x7ffd496e08e0


int ch1 = 123;
int *ch3;

// 信息SIGUSER1的处理函数
void siguser1_handler(int signum)
{
    printf("SIGUSER1 received\n");

    int *p = (int *)POINTER;
    printf("int value at %p: %d\n", p, *p);
}


int main()
{

    int ch2;
    ch3 = (int *)malloc(sizeof(int));

    printf("ch1 at %p\n", &ch1);
    printf("ch2 at %p\n", &ch2);
    printf("ch3 at %p\n", ch3);
    
    signal(SIGUSR1, siguser1_handler);
    
    pause();

    return 0;
}