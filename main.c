#define _GNU_SOURCE    //pthread_getattr_np
#include <fcntl.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/syscall.h>

#define PAGEMAP_LENGTH 8
#define gettid() syscall(SYS_gettid)

// get the first address of initialized data segment
// ref: https://stackoverflow.com/questions/33493600/how-to-get-the-first-address-of-initialized-data-segment
extern char __bss_start;
extern void *_end;

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

unsigned long virt_to_phy_vaild(pid_t pid, unsigned long addr)
{
    char str[20];
    sprintf(str, "/proc/%u/pagemap", pid);
    int fd = open(str, O_RDONLY);
    if(fd < 0)
    {
        printf("open %s failed!\n", str);
        return 0;
    }
    unsigned long pagesize = getpagesize();
    unsigned long offset = (addr / pagesize) * sizeof(uint64_t);
    if(lseek(fd, offset, SEEK_SET) < 0)
    {
        printf("lseek() failed!\n");
        close(fd);
        return 0;
    }
    uint64_t info;
    if(read(fd, &info, sizeof(uint64_t)) != sizeof(uint64_t))
    {
        printf("read() failed!\n");
        close(fd);
        return 0;
    }
    if((info & (((uint64_t)1) << 63)) == 0)
    {
        printf("page is not present!\n");
        close(fd);
        return 0;
    }
    unsigned long frame = info & ((((uint64_t)1) << 55) - 1);
    unsigned long phy = frame * pagesize + addr % pagesize;
    close(fd);
    return phy;
}

void *child_thread_information(void *data) 
{
    // https://man7.org/linux/man-pages/man2/gettid.2.html
    pid_t tid = gettid();
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
        printf("thread-%c: %lx ---> %lx ", thread_number, vir_addr[idx], phy_addr[idx]);
        if (virt_to_phy_vaild(tid, vir_addr[idx]) == phy_addr[idx]) {
            printf("Vaildation[O]\n");
        } else {
            printf("Vaildation[x]\n");
        }
        sleep(3);
    }
    sleep(5);
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
    unsigned long t1_stack_size = 0, t2_stack_size = 0, t3_stack_size = 0;

    pthread_getattr_np(t1, &thread_attr);
    pthread_attr_getstack(&thread_attr, &t1_stack_addr, &t1_stack_size);

    pthread_getattr_np(t2, &thread_attr);
    pthread_attr_getstack(&thread_attr, &t2_stack_addr, &t2_stack_size);

    pthread_getattr_np(t3, &thread_attr);
    pthread_attr_getstack(&thread_attr, &t3_stack_addr, &t3_stack_size);

    /*argv, environ address range*/
    printf("0x%lx <= env_end \n0x%lx <= env_start \n", seg.env_end, seg.env_start);
    printf("0x%lx <= arg_end \n0x%lx <= arg_start \n\n", seg.arg_end, seg.arg_start);

    /*main stack address range*/
    printf("0x%lx <= start_stack \n", seg.start_stack);

    register long rbp asm("rbp");
    printf("%#010lx <= start stack for main thread($RBP)\n", rbp);
    register long rsp asm("rsp");
    printf("%#010lx <= end stack for main thread($RSP)\n\n", rsp);

    /*thread stack address range*/
    printf("%p <= end   stack for thread 1\n", (char *)t1_stack_addr + t1_stack_size);
    printf("%p <= Start stack for thread 1\n", t1_stack_addr);

    printf("%p <= end   stack for thread 2\n", (char *)t2_stack_addr + t2_stack_size);
    printf("%p <= start stack for thread 2\n", t2_stack_addr);

    printf("%p <= end   stack for thread 3\n", (char *)t3_stack_addr + t3_stack_size);
    printf("%p <= start stack for thread 3\n\n", t3_stack_addr);

    /*heap address range*/
    printf("0x%lx <= end_heap (brk)\n", seg.brk);
    printf("0x%lx <= start_heap (brk_start)\n\n", seg.start_brk);

    /*bss address range*/
    printf("%p <= bss_end\n", &_end);
    printf("%p <= bss_start\n\n", &__bss_start);

    /*data address range*/
    printf("0x%lx <= data_end\n", seg.end_data);
    printf("0x%lx <= data_start\n\n", seg.start_data);

    /*code address range*/
    printf("0x%lx <= code_end\n", seg.end_code);
    printf("%p <= main function address\n", &main);
    printf("%p <=  void *child_thread_information(void *data) address\n", child_thread_information);
    printf("0x%lx <= code_start\n\n", seg.start_code);

    pthread_join(t1, NULL);
    pthread_join(t2, NULL);
    pthread_join(t3, NULL);
    
    getchar();
    return 0;
}