#ifndef OBJPOOL_H
#define OBJPOOL_H

#include "lib.h"
#include "kprobe.h"

/* obj_pool对象池,用于充当buffer */
struct obj_pool {
    unsigned long       pg_addr;	// 内存页的起始地址
    unsigned int        pg_order; // 申请内存的页阶（2^pg_order 页，每页通常 4KB）
    unsigned int        obj_size;// 每个对象的大小（字节）
    unsigned int        max_idx;//最大可存储的对象数量
    atomic_t            next_idx;//下一个可用对象的索引（原子变量）
};

extern struct obj_pool *kp_pool;

extern void alloc_kp_pool(void); 
extern struct kp_info * new_kp_info(struct obj_pool *kp_pool);
extern struct kp_info * get_index_kp_info(struct obj_pool *kp_pool, unsigned int index);
extern void destroy_kp_pool(void);

#endif
