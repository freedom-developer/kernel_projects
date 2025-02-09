#include <stdio.h>
#include <unistd.h>
#include <signal.h>

#define N 1
#define POINTER 0x7ffd496e08e0

typedef struct {
    char ch;
    short sh;
    // int i;
    // long l;
    // float f;
    // double d;
    // long double ld;
} MyStruct __attribute__((__aligned__(16)));


// 信息SIGUSER1的处理函数
void siguser1_handler(int signum)
{
    printf("SIGUSER1 received\n");

    int *p = (int *)POINTER;
    printf("int value at %p: %d\n", p, *p);
}

int main()
{
    // MyStruct s1, s2;

    // printf("struct size: %ld\n", sizeof(MyStruct));
    // printf("s1: %p\n", &s1);
    // printf("s2: %p\n", &s2);
    signal(SIGUSR1, siguser1_handler);
    
    pause();

    return 0;
}