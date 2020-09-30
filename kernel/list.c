#include "./../include/list.h"
#include "./../include/interrupt.h" 

//实现的是双向链表，不是循环链表

/*
访问公共资源的程序片段称为临界区，临界区通常是指在
不同线程中、修改同一公共资源的指令区域。临界区的代码
应该属于原子操作，要么不执行，要么全部执行完*/
void list_init(struct List*list) {
    list->head.prev=NULL;
    list->head.next=&(list->tail);

    list->tail.next=NULL;
    list->tail.prev=&(list->head);
}
/*将elem插入在before之前*/
void list_insert_before(struct ListNode*before,struct ListNode*elem) {
    //关闭中断，返回关中断之前的状态
    enum intr_status old_status = intr_disable();
    before->prev->next=elem;
    elem->prev=before->prev;
    elem->next=before;
    before->prev=elem;
    intr_set_status(old_status);    //将中断恢复到之前的状态
}
/*将元素添加到队列首部*/
void list_push(struct List* plist,struct ListNode* elem) {
    list_insert_before(plist->head.next,elem);  //队头插入elem
}
/*在链表尾部添加元素*/
void list_append(struct List* plist,struct ListNode* elem) {
    list_insert_before(&plist->tail,elem);
}
/*从链表中删除一个元素*/
void list_remove(struct ListNode* pelem) {
    enum intr_status old_status = intr_disable();

    pelem->prev->next = pelem->next;
    pelem->next->prev = pelem->prev;
    //这里没有回收浪费的空间

    intr_set_status(old_status);
}
/*从队列里面pop出来一个元素*/
struct ListNode* list_pop(struct List* plist) {
    struct ListNode* elem = plist->head.next;
    list_remove(elem);
    return elem;
}
/*从链表中查找obj_elem，成功时返回1，失败时返回0*/
int elem_find(struct List* plist,struct ListNode* obj_elem) {
    struct ListNode*current=plist->head.next;
    while(current != &plist->tail){
        if(current == obj_elem) {
            return 1;
        }
        current = current->next;
    }
    return 0;
}

/*
*遍历链表中的所有元素，逐个判断是否有符合条件的元素
*找到符合条件的元素返回元素指针，否则返回NULL
*/
struct ListNode* list_traverse(struct List* plist, function func,int arg){
    struct ListNode*current = plist->head.next;
    /*如果队列为空，直接返回NULL*/
    if(list_empty(plist)) {
        return NULL;
    }
    while(current != &plist->tail) {
        if(func(current,arg)) {
            return current;
        }
        current = current->next;
    }
    return NULL;
}
/*返回链表的长度*/
uint32_t list_len(struct List* plist) {
    struct ListNode*current = plist->head.next;
    uint32_t length = 0;
    while(current != &plist->tail){
        length++;
        current = current->next;
    }
    return length;
}
/*判断链表是否为空*/
int list_empty(struct List* plist) {
    return plist->head.next == &plist->tail ? 1:0;
}