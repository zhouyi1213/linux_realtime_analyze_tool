#include "../include/lib.h"
#include "../include/proc.h"
#include "../include/kprobe.h"
#include "../include/data.h"
#include "../include/xarray.h"
#include "../include/workqueue.h"
#include "../include/kfifo.h"
#include "../include/kthread.h"

struct workqueue_struct  *wq;
struct work_struct       work;
static struct task_struct *datasave_task2[MAX_THREAD];



static int __kprobes datasave_handler(void *data)
{
    int err;
    unsigned int len;
    unsigned long pid, cpu, key, index;
    struct kp_info *kp_info;
    struct task_info *task_info, *task_info_alloc = NULL;

    while (!kthread_should_stop())
    {
        spin_lock_irq(&kfifo_lock);
        len = kfifo_out(&kfifo, &kp_info, sizeof(struct kp_info *));
        spin_unlock_irq(&kfifo_lock);

        if (!len)
            msleep(10);
        else
        {
            if (atomic_cmpxchg(&(kp_info->kp_state), KP_STATE_WAITING, KP_STATE_READING) & KP_STATE_WAITING)
            {
                key = pid = kp_info->task->pid;
                cpu = kp_info->cpu;
                if (!pid)
                    key = (cpu << 32) | pid;

                task_info = xa_load(xa, key);
                if (task_info == NULL)
                {
                    task_info_alloc = kmalloc(sizeof(*task_info_alloc), GFP_KERNEL);
                    init_task_info(task_info_alloc, kp_info);

                    err = xa_err(xa_store(xa, key, task_info_alloc, GFP_KERNEL));
                    if (err)
                        goto remove_read;
                    task_info = task_info_alloc;
                }

                update_task_info(task_info, kp_info);
				//printk("update task info completed");
            remove_read:
                refcount_dec(&(kp_info->task->usage));
                atomic_set(&(kp_info->kp_state), 0);
            }
        }
    }
    return 0;
}
void work_handler(struct work_struct *work)
{
    int current_val;
    // 补充日志级别+换行，符合内核规范
    printk(KERN_INFO "This is work_handler\n");

    // 前置检查：避免count自减后为负数（核心修复）
    if (atomic_read(&count) <= 0) {
        printk(KERN_WARNING "count is 0, skip thread creation\n");
        return;
    }

    // 原子自减并判断：仅当自减后≠0时创建线程
    if (!atomic_dec_and_test(&count)) {
        current_val = atomic_read(&count);

        // 核心：检查数组下标是否越界（修复Oops风险）
        if (current_val < 0 || current_val >= MAX_THREAD) {
            printk(KERN_ERR "current_val %d out of range [0, %d]\n", current_val, MAX_THREAD-1);
            atomic_inc(&count);  // 恢复count，修正计数错误
            return;
        }

        // 避免重复创建线程
        if (datasave_task2[current_val] != NULL) {
            printk(KERN_WARNING "datasave2[%d] already exists\n", current_val);
            return;
        }

        // 创建内核线程
        datasave_task2[current_val] = kthread_run(datasave_handler, NULL, "datasave2[%d]", current_val);
        if (IS_ERR(datasave_task2[current_val])) {
            int err = PTR_ERR(datasave_task2[current_val]);
            printk(KERN_ERR "kthread_run failed for datasave2[%d], err: %d\n", current_val, err);
            datasave_task2[current_val] = NULL;
            atomic_inc(&count);  // 修复：创建失败恢复count
            return;
        }
        printk(KERN_INFO "Success create datasave2[%d]\n", current_val);
    }
}

extern int wq_init(void)
{
    wq = create_singlethread_workqueue("handler_fifo");
    if(DEBUG) printk("alloc_wq!\n");

    INIT_WORK(&work, work_handler);
	memset(datasave_task2,0,sizeof(datasave_task2));
	atomic_set(&count,MAX_THREAD);
    return 0;
}

extern void wq_exit(void)
{
    if(DEBUG) printk("Workqueue is exit!\n");
	int i;
	for(i=0;i<MAX_THREAD;i++)
		{
		if(datasave_task2[i]!=NULL)
			{
			kthread_stop(datasave_task2[i]);
			datasave_task2[i]=NULL;
			}
		}
	atomic_set(&count,MAX_THREAD);
    destroy_workqueue(wq);
}

MODULE_LICENSE("GPL");