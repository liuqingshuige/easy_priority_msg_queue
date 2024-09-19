#ifndef __FREE_EASY_PRIORITY_MSG_QUEUE_H__
#define __FREE_EASY_PRIORITY_MSG_QUEUE_H__
#include <stdbool.h>

////>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
// 优先级消息队列声明：支持多线程
typedef void *PMQHandle;

/*
 * 创建一个消息队列
 * capacity：队列最大容量
 * return：成功返回队列句柄，失败返回NULL
 */
PMQHandle CreatePriorityMsgQueue(int capacity);

/*
 * 销毁消息队列
 * queue：CreatePriorityMsgQueue()返回的消息句柄
 */
void DestroyPriorityMsgQueue(PMQHandle queue);

/*
 * 添加一个消息到消息队列
 * queue：CreatePriorityMsgQueue()返回的消息句柄
 * val：用户数据
 * prio：消息优先级，数值越大优先级越高
 * return：成功true，失败返回false
 */
bool PushPriorityMsgQueue(PMQHandle queue, void *val, int prio);

/*
 * 消息队列中消息是否为空
 * queue：CreatePriorityMsgQueue()返回的消息句柄
 */
bool IsPriorityMsgQueueEmpty(PMQHandle queue);

/*
 * 消息队列中当前消息数量
 * queue：CreatePriorityMsgQueue()返回的消息句柄
 */
int GetPriorityMsgQueueSize(PMQHandle queue);

/*
 * 阻塞获取消息，若当前队列中无消息，则会阻塞等待
 * queue：CreatePriorityMsgQueue()返回的消息句柄
 * val：保存用户数据
 * prio：保存消息优先级
 */
void PopPriorityMsgQueue(PMQHandle queue, void **val, int *prio);

/*
 * 尝试获取消息，若当前队列中无消息，则立马返回false，不会阻塞
 * queue：CreatePriorityMsgQueue()返回的消息句柄
 * val：保存用户数据
 * prio：保存消息优先级
 * return：取得消息返回true，否则false
 */
bool TryPopPriorityMsgQueue(PMQHandle queue, void **val, int *prio);

/*
 * 获取消息，若当前队列中无消息，则会进入超时等待
 * queue：CreatePriorityMsgQueue()返回的消息句柄
 * val：保存用户数据
 * prio：保存消息优先级
 * timeout：超时时间，单位ms
 * return：取得消息返回true，否则false
 */
bool TimedPopPriorityMsgQueue(PMQHandle queue, void **val, int *prio, unsigned int timeout);



#endif

