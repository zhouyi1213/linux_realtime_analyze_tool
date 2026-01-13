#ifndef KPROBE_H
#define KPROBE_H

#include "lib.h"
#include "percpu.h"

#define NUM_STACK_ENTRIES       64
#define MAX_SYMBOL_LEN          64

#define KP_STATE_WAITING        0x1
#define KP_STATE_READING        0x1 << 1
#define KP_STATE_WRITING        0x1 << 2

/* 要保存的结构体kp_info */
struct kp_info {
    atomic_t            kp_state;//同步变量
    struct task_struct  *task;//指向当前进程的任务结构体指针
    unsigned long       lock_addr;//锁的内存地址
    unsigned int        cpu;//CPU 编号
    unsigned long       time_stamp;//锁操作的时间戳（通常为 ktime_get_ns() 返回的纳秒级时间）
    unsigned long       delta;//时间差值
    unsigned int        num_entries;//stack_entries 数组中有效条目的数量
    unsigned long       stack_entries[NUM_STACK_ENTRIES];//存储锁操作发生时的调用栈信息（函数返回地址）
};

//irq
extern int spin_lock_irq_init(void);
extern void spin_lock_irq_exit(void);

//irq_save
extern int spin_lock_irqsave_init(void);
extern void spin_lock_irqsave_exit(void);

#endif