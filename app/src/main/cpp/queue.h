#include <stdio.h>
extern "C" {

typedef struct queue_node {
	struct queue_node* prev;
	struct queue_node* next;
	void *p;//节点的值
} node;

typedef struct Queue{
	node *phead;
	int count;
	Queue(){
		phead=NULL;
		count=0;
	}
};

// 表头。注意，表头不存放元素值！！！
//static node *phead = NULL;
//static int count = 0;

static node* create_node(void *pval) {
	node *pnode = new node;
	if (pnode) {
		// 默认的，pnode的前一节点和后一节点都指向它自身
		pnode->prev = pnode->next = pnode;
		// 节点的值为pval
		pnode->p = pval;
	}
	return pnode;
}
//// 新建“双向链表”。成功，返回0；否则，返回-1。
Queue* create_queue() {
	node *phead = create_node(NULL);
	if (!phead) {
		return NULL;
	}
	Queue *queue=new Queue;
    queue->phead=phead;
	return queue;
}

// “双向链表是否为空”
int queue_is_empty(Queue *queue) {
	return queue->count==0;
}

// 返回“双向链表的大小”
int queue_size(Queue *queue) {
	return queue->count;
}

// 获取“双向链表中第index位置的节点”
static node* get_node(int index,Queue *queue) {
	if (index < 0 || index >= queue->count) {
		return NULL;
	}
	if (index <= (queue->count)) {
		int i = 0;
		node *pnode = queue->phead->next;
		while ((i++) < index)
			pnode = pnode->next;
		return pnode;
	}
	int j = 0;
	int rindex = queue->count - index - 1;
	node *rnode = queue->phead->prev;
	while ((j++) < rindex)
		rnode = rnode->prev;
	return rnode;
}

// 获取“第一个节点”
static node* get_first_node(Queue *queue) {
	return get_node(0,queue);
}
// 获取“最后一个节点”
static node* get_last_node(Queue *queue) {
	return get_node(queue->count - 1,queue);
}
// 获取“双向链表中第index位置的元素”。成功，返回节点值；否则，返回-1。
void* queue_get(int index,Queue *queue) {
	node *pindex = get_node(index,queue);
	if (!pindex) {
		return NULL;
	}
	return pindex->p;
}

// 获取“双向链表中第1个元素的值”
void* queue_get_first(Queue *queue) {
	return queue_get(1,queue);
}

void* queue_get_last(Queue *queue) {
	return queue_get(queue->count - 1,queue);
}

// 将“pval”插入到表头位置
int queue_insert_first(void *pval,Queue *queue) {
    node *pnode = create_node(pval);
    if (!pnode)
        return -1;
    pnode->prev = queue->phead;
    pnode->next = queue->phead->next;
    queue->phead->next->prev = pnode;
    queue->phead->next = pnode;
    queue->count++;
    return 0;
}

// 将“pval”插入到index位置。成功，返回0；否则，返回-1。
int queue_insert(int index, void* pval,Queue *queue) {
	// 插入表头
	if (index == 0)
		return queue_insert_first(pval,queue);
	// 获取要插入的位置对应的节点
	node *pindex = get_node(index,queue);
	if (!pindex)
		return -1;
// 创建“节点”
	node *pnode = create_node(pval);
	if (!pnode)
		return -1;
	pnode->prev = pindex->prev;
	pnode->next = pindex;
	pindex->prev->next = pnode;
	pindex->prev = pnode;
	// 节点个数+1
    queue->count++;
	return 0;
}

// 将“pval”插入到末尾位置
int queue_append_last(void *pval,Queue *queue) {
	node *pnode = create_node(pval);
	if (!pnode)
		return -1;
	pnode->next = queue->phead;
	pnode->prev = queue->phead->prev;
    queue->phead->prev->next = pnode;
    queue->phead->prev = pnode;
    queue->count++;
	return 0;
}
// 删除“双向链表中index位置的节点”。成功，返回0；否则，返回-1。
int queue_delete(int index,Queue *queue) {
	node *pindex = get_node(index,queue);
	if (!pindex) {
		return -1;
	}
	pindex->next->prev = pindex->prev;
	pindex->prev->next = pindex->next;
	delete pindex;
    queue->count--;
	return 0;
}
// 删除第一个节点
int queue_delete_first(Queue *queue) {
	return queue_delete(0,queue);
}
// 删除组后一个节点
int queue_delete_last(Queue *queue) {
	return queue_delete(queue->count - 1,queue);
}
// 撤销“双向链表”。成功，返回0；否则，返回-1。
int destroy_queue(Queue *queue) {
	if (!queue->phead) {
		return -1;
	}
	node *pnode = queue->phead->next;
	node *ptmp = NULL;
	while (pnode != queue->phead) {
		ptmp = pnode;
		pnode = pnode->next;
		delete ptmp;
	}
	delete queue->phead;
    queue->phead = NULL;
    queue->count = 0;
	return 0;
}

};
