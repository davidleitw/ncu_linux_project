#include <linux/kernel.h>
#include <linux/string.h>
#include <linux/uaccess.h>
#include <linux/init_task.h>

struct segment {
    unsigned long start_code, end_code, start_data, end_data;
    unsigned long start_brk, brk;
    unsigned long start_stack;
    unsigned long arg_start, arg_end, env_start, env_end;
};

asmlinkage int get_mm_struct_info(pid_t pid, void *__user user_address) 
{
    struct task_struct *task;
    for_each_process(task) {
        if (task->pid == pid) {
            struct segment seg = {
                .start_code = task->mm->start_code,
                .end_code = task->mm->end_code,
                .start_data = task->mm->start_data,
                .end_data = task->mm->end_data,
                .start_brk = task->mm->start_brk,
                .brk = task->mm->brk,
                .start_stack = task->mm->start_stack,
                .arg_start = task->mm->arg_start,
                .arg_end = task->mm->arg_end,
                .env_start = task->mm->env_start,
                .env_end = task->mm->env_end  
            };

            copy_to_user(user_address, &seg, sizeof(struct segment));
            return 0;
        }
    }
    return -1;
}

