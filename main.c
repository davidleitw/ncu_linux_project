#define _GNU_SOURCE    //pthread_getattr_np
#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <stdlib.h>

// get the first address of initialized data segment
// ref: https://stackoverflow.com/questions/33493600/how-to-get-the-first-address-of-initialized-data-segment
extern char __bss_start;

int a;
int b = 48763;
int *ptr;

char msg[][250] = {
    "code segment: void *child_thread_information(void *data) address",
    "bss segment: int a, a global varibale without initialization",
    "data segment: int b = 48763, a global variable with initialization",
    "heap segment: int *ptr = malloc(10 * sizeof(int)), a pointer malloc in main function",
    "lib: printf function address",
    "data segment: static int si, a static variable in thread function",
    "stack segment: static __thread int sti, a static __thread variable in thread function",
    "thread local storages: int idx, a variable in thread function"
};

struct segment {
    unsigned long start_code, end_code, start_data, end_data;
    unsigned long start_brk, brk;
    unsigned long start_stack;
    unsigned long arg_start, arg_end, env_start, env_end;
};

void *child_thread_information(void *data) {
    char thread_number = ((char *)data)[0];

    int idx;
    static int si;
    static __thread int sti;
    unsigned long vir_addr[8];
    unsigned long phy_addr[8];
    
    sleep(5);

    vir_addr[0] = (unsigned long)child_thread_information;
    vir_addr[1] = (unsigned long)&a;
    vir_addr[2] = (unsigned long)&b;
    vir_addr[3] = (unsigned long)ptr;
    vir_addr[4] = (unsigned long)printf;
    vir_addr[5] = (unsigned long)&si;
    vir_addr[6] = (unsigned long)&sti;
    vir_addr[7] = (unsigned long)&idx;

    int err = syscall(335, vir_addr, 8, phy_addr, 8);
    if (err < 0) {
        printf("System call no.335 error, error code = %d\n", err);
        pthread_exit(NULL);
    }

    for (idx = 0; idx < 8; ++idx) {
        if (thread_number == '1') {
            printf("\n%s\n", msg[idx]);
        }
        printf("thread-%c: %lx ---> %lx\n", thread_number, vir_addr[idx], phy_addr[idx]);
        sleep(3);
    }

    pthread_exit(NULL);    
}

int main() {
    ptr = (int*)malloc(10 * sizeof(int));

    struct segment seg;
    syscall(334, getpid(), &seg);

    printf("pid = %d\n", getpid());    
    pthread_attr_t thread_attr;
    pthread_t t1, t2, t3;

    pthread_create(&t1, NULL, child_thread_information, "1");
    sleep(1);
    pthread_create(&t2, NULL, child_thread_information, "2");
    sleep(1);
    pthread_create(&t3, NULL, child_thread_information, "3");
    
    void *t1_stack_addr, *t2_stack_addr, *t3_stack_addr;
    size_t t1_stack_size = 0, t2_stack_size = 0, t3_stack_size = 0;

    pthread_getattr_np(t1, &thread_attr);
    pthread_attr_getstack(&thread_attr, &t1_stack_addr, &t1_stack_size);

    pthread_getattr_np(t2, &thread_attr);
    pthread_attr_getstack(&thread_attr, &t2_stack_addr, &t2_stack_size);

    pthread_getattr_np(t3, &thread_attr);
    pthread_attr_getstack(&thread_attr, &t3_stack_addr, &t3_stack_size);

    pthread_join(t1, NULL);
    pthread_join(t2, NULL);
    pthread_join(t3, NULL);
    
    getchar();
    return 0;
}