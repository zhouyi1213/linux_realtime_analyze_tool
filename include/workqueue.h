#ifndef WORKQUEUE_H
#define WORKQUEUE_H
#define MAX_THREAD 5
#include "lib.h"

extern struct workqueue_struct  *wq;
extern struct work_struct       work;
static atomic_t count;
extern int wq_init(void);
extern void wq_exit(void);
static int __kprobes datasave_handler(void *data);
#endif