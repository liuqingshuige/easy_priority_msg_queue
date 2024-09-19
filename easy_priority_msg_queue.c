#include "easy_priority_msg_queue.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <stdint.h>

////>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
// 消息实现
typedef struct __msg_t
{
	void *value_; // 用户数据
	int priority_; // 消息优先级，数字大则优先级高
}msg_t;

// 消息初始化
static void set_msg_vp(msg_t *msg, void *value, int prio)
{
	msg->value_ = value;
	msg->priority_ = prio;
}

// 获取消息
static void *get_msg_value(msg_t *msg)
{
	return msg->value_;
}

// 获取优先级
static int get_msg_priority(msg_t *msg)
{
	return msg->priority_;
}

////>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
// 互斥锁实现
typedef struct __mutex_t
{
	pthread_mutex_t mutex_;
}mutex_t;

// 互斥锁初始化
static void init_mutex(mutex_t *mtx)
{
	pthread_mutex_init(&mtx->mutex_, NULL);
}

// 销毁
static void deinit_mutex(mutex_t *mtx)
{
	pthread_mutex_destroy(&mtx->mutex_);
}

// 上锁
static void lock_mutex(mutex_t *mtx)
{
	pthread_mutex_lock(&mtx->mutex_);
}

// 解锁
static void unlock_mutex(mutex_t *mtx)
{
	pthread_mutex_unlock(&mtx->mutex_);
}

// 取锁
static pthread_mutex_t *get_mutex(mutex_t *mtx)
{
	return &mtx->mutex_;
}

////>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
// 条件变量实现
typedef struct __condition_t
{
	pthread_cond_t cond_;
}condition_t;

// 条件变量初始化
static void init_condition(condition_t *cond)
{
	pthread_cond_init(&cond->cond_, NULL);
}

// 销毁
static void deinit_condition(condition_t *cond)
{
	pthread_cond_destroy(&cond->cond_);
}

// 阻塞等待通知
static void wait_condition(condition_t *cond, mutex_t *mtx)
{
	pthread_cond_wait(&cond->cond_, get_mutex(mtx));
}

// 超时等待通知
static bool timed_wait_condition(condition_t *cond, mutex_t *mtx, unsigned int ms)
{
	struct timespec abstime, now;
	clock_gettime(CLOCK_REALTIME, &now);
	abstime.tv_sec = now.tv_sec + ms/1000;
	uint64_t ns = now.tv_nsec + ms%1000*1000*1000;
	if (ns > 999999999)
	{
		abstime.tv_sec += 1;
		ns -= 1000000000;
	}
	abstime.tv_nsec = ns;
	return (pthread_cond_timedwait(&cond->cond_, get_mutex(mtx), &abstime) == 0);
}

// 单个通知
static void signal_condition(condition_t *cond)
{
	pthread_cond_signal(&cond->cond_);
}

// 广播通知
static void broadcast_condition(condition_t *cond)
{
	pthread_cond_broadcast(&cond->cond_);
}

////>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
// 队列实现
typedef struct __priority_queue_t
{
	mutex_t mutex_;
	int capacity_; // 队列容量
	int size_; // 当前队列大小
	msg_t *array_;
}priority_queue_t;

// 队列初始化
static void init_priority_queue(priority_queue_t *queue, int capacity)
{
	if (capacity <= 0)
		capacity = 64;
	queue->size_ = 0;
	queue->capacity_ = capacity;
	queue->array_ = (msg_t *)calloc(capacity, sizeof(msg_t));
	init_mutex(&queue->mutex_);
}

// 队列销毁
static void deinit_priority_queue(priority_queue_t *queue)
{
	if (queue->array_)
		free(queue->array_);
	queue->array_ = NULL;
	deinit_mutex(&queue->mutex_);
}

// 获取队列大小
static int get_priority_queue_size(priority_queue_t *queue)
{
	lock_mutex(&queue->mutex_);
	int sz = queue->size_;
	unlock_mutex(&queue->mutex_);
	return sz;
}

// 队列是否为空
static bool is_priority_queue_empty(priority_queue_t *queue)
{
	return get_priority_queue_size(queue) == 0;
}

// 清空队列
static void clear_priority_queue(priority_queue_t *queue)
{
	lock_mutex(&queue->mutex_);
	queue->size_ = 0;
	unlock_mutex(&queue->mutex_);
}

// 取得消息
static msg_t top_priority_queue(priority_queue_t *queue)
{
	lock_mutex(&queue->mutex_);
	msg_t msg = queue->array_[0];
	unlock_mutex(&queue->mutex_);
	return msg;
}

// 弹出消息
static void pop_priority_queue(priority_queue_t *queue)
{
	lock_mutex(&queue->mutex_);
	if (queue->size_ > 0)
	{
		memmove(&queue->array_[0], &queue->array_[1], (queue->size_-1)*sizeof(msg_t));
		queue->size_--;
	}
	unlock_mutex(&queue->mutex_);
}

// 添加消息
static void push_priority_queue(priority_queue_t *queue, msg_t *key)
{
	lock_mutex(&queue->mutex_);

	if (queue->size_ >= queue->capacity_) // 扩容
	{
		msg_t *new_arr = (msg_t *)realloc(queue->array_, (queue->capacity_<<1)*sizeof(msg_t));
		if (new_arr)
		{
			queue->capacity_ <<= 1;
			queue->array_ = new_arr;
		}
		else
		{
			unlock_mutex(&queue->mutex_);
			return;
		}
	}

	if (queue->size_ == 0) // 首个消息
	{
		memcpy(&queue->array_[0], key, sizeof(msg_t));
		queue->size_++;
		unlock_mutex(&queue->mutex_);
		return;	
	}

	int idx = 0;
	for (; idx<queue->size_; idx++) // 降序
	{
		if (get_msg_priority(&queue->array_[idx]) < get_msg_priority(key))
			break;
	}

	if (idx == queue->size_) // 没有找到，则说明key的优先级最低
	{
		memcpy(&queue->array_[queue->size_], key, sizeof(msg_t));
		queue->size_++;
	}
	else
	{
		memmove(&queue->array_[idx+1], &queue->array_[idx], (queue->size_-idx)*sizeof(msg_t));
		memcpy(&queue->array_[idx], key, sizeof(msg_t));
		queue->size_++;
	}

	unlock_mutex(&queue->mutex_);
}

////>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
// 优先级消息队列实现
typedef struct PriorityMsgQueue
{
    priority_queue_t queue_; // 队列：保存消息
    mutex_t mutex_; // 锁
    condition_t cv_;
    int queueSize_; // 队列容量
}PriorityMsgQueue;

/*
 * 创建一个消息队列
 * capacity：队列最大容量
 * return：成功返回队列句柄，失败返回NULL
 */
PMQHandle CreatePriorityMsgQueue(int capacity)
{
	PriorityMsgQueue *queue = (PriorityMsgQueue *)malloc(sizeof(PriorityMsgQueue));
	if (!queue)
		return NULL;

	if (capacity < 0)
		capacity = 64;

	queue->queueSize_ = capacity;
	init_priority_queue(&queue->queue_, capacity);
	init_mutex(&queue->mutex_);
	init_condition(&queue->cv_);

	return queue;
}

/*
 * 销毁消息队列
 * queue：CreatePriorityMsgQueue()返回的消息句柄
 */
void DestroyPriorityMsgQueue(PMQHandle q)
{
	PriorityMsgQueue *queue = (PriorityMsgQueue *)q;
	if (!queue)
		return;

	deinit_condition(&queue->cv_);
	deinit_mutex(&queue->mutex_);
	deinit_priority_queue(&queue->queue_);
	
	free(queue);
}

/*
 * 添加一个消息到消息队列
 * queue：CreatePriorityMsgQueue()返回的消息句柄
 * val：用户数据
 * prio：消息优先级，数值越大优先级越高
 * return：成功true，失败返回false
 */
bool PushPriorityMsgQueue(PMQHandle q, void *val, int prio)
{
	PriorityMsgQueue *queue = (PriorityMsgQueue *)q;
	if (!queue)
		return false;

	lock_mutex(&queue->mutex_);

	if (get_priority_queue_size(&queue->queue_) >= queue->queueSize_) // 队列已经满了
	{
		unlock_mutex(&queue->mutex_);
		return false;
	}

	msg_t msg;
	set_msg_vp(&msg, val, prio);
	push_priority_queue(&queue->queue_, &msg);
	signal_condition(&queue->cv_);

	unlock_mutex(&queue->mutex_);

	return true;
}

/*
 * 消息队列中消息是否为空
 * queue：CreatePriorityMsgQueue()返回的消息句柄
 */
bool IsPriorityMsgQueueEmpty(PMQHandle q)
{
	PriorityMsgQueue *queue = (PriorityMsgQueue *)q;
	if (!queue)
		return false;

	lock_mutex(&queue->mutex_);
	bool ret = is_priority_queue_empty(&queue->queue_);
	unlock_mutex(&queue->mutex_);
	return ret;
}

/*
 * 消息队列中当前消息数量
 * queue：CreatePriorityMsgQueue()返回的消息句柄
 */
int GetPriorityMsgQueueSize(PMQHandle q)
{
	PriorityMsgQueue *queue = (PriorityMsgQueue *)q;
	if (!queue)
		return 0;

	lock_mutex(&queue->mutex_);
	int ret = get_priority_queue_size(&queue->queue_);
	unlock_mutex(&queue->mutex_);
	return ret;
}

// 取出一个消息
static void __GetOneMsgInternal(PriorityMsgQueue *queue, void **val, int *prio)
{
	msg_t msg = top_priority_queue(&queue->queue_); // 取出消息
	pop_priority_queue(&queue->queue_); // 从队列中删除该消息

	if (val)
		*val = get_msg_value(&msg);

	if (prio)
		*prio = get_msg_priority(&msg);
}

/*
 * 阻塞获取消息，若当前队列中无消息，则会阻塞等待
 * queue：CreatePriorityMsgQueue()返回的消息句柄
 * val：保存用户数据
 * prio：保存消息优先级
 */
void PopPriorityMsgQueue(PMQHandle q, void **val, int *prio)
{
	PriorityMsgQueue *queue = (PriorityMsgQueue *)q;
	if (!queue)
		return;

	lock_mutex(&queue->mutex_);

	while (is_priority_queue_empty(&queue->queue_))
	{
		wait_condition(&queue->cv_, &queue->mutex_);
	}

	__GetOneMsgInternal(queue, val, prio);

	unlock_mutex(&queue->mutex_);
}

/*
 * 尝试获取消息，若当前队列中无消息，则立马返回false，不会阻塞
 * queue：CreatePriorityMsgQueue()返回的消息句柄
 * val：保存用户数据
 * prio：保存消息优先级
 * return：取得消息返回true，否则false
 */
bool TryPopPriorityMsgQueue(PMQHandle q, void **val, int *prio)
{
	PriorityMsgQueue *queue = (PriorityMsgQueue *)q;
	if (!queue)
		return false;

	lock_mutex(&queue->mutex_);

	if (is_priority_queue_empty(&queue->queue_))
	{
		unlock_mutex(&queue->mutex_);
		return false;
	}

	__GetOneMsgInternal(queue, val, prio);

	unlock_mutex(&queue->mutex_);

	return true;
}

/*
 * 获取消息，若当前队列中无消息，则会进入超时等待
 * queue：CreatePriorityMsgQueue()返回的消息句柄
 * val：保存用户数据
 * prio：保存消息优先级
 * timeout：超时时间，单位ms
 * return：取得消息返回true，否则false
 */
bool TimedPopPriorityMsgQueue(PMQHandle q, void **val, int *prio, unsigned int timeout)
{
	PriorityMsgQueue *queue = (PriorityMsgQueue *)q;
	if (!queue)
		return false;

	lock_mutex(&queue->mutex_);

	if (is_priority_queue_empty(&queue->queue_)) // 当前无消息，则进入超时等待
	{
		bool ret = timed_wait_condition(&queue->cv_, &queue->mutex_, timeout);
		if (ret)
		{
			__GetOneMsgInternal(queue, val, prio);
		}
		unlock_mutex(&queue->mutex_);
		return ret;
	}

	// 当前队列已经有消息，则直接取出消息
	__GetOneMsgInternal(queue, val, prio);

	unlock_mutex(&queue->mutex_);
	return true;
}

