#ifndef __LIST_H__
#define __LIST_H__
#include "./../include/global.h"
#include <stdio.h>

/*将节点元素转换成实际元素*/
#define offset(struct_type,member) (int)(&((struct_type*)0)->member)
#define elem2entry(struct_type,struct_member_name,elem_ptr) \
        (struct_type*)((int)elem_ptr-offset(struct_type,struct_member_name))

/*
struct_type:elem_ptr所在结构体的类型
struct_member_name: elem_ptr所在结构体中对应地址的成员名字
elem_ptr:待转换的地址

作用:将elem_ptr指针转换成struct_type对应类型的指针

原理:elem_ptr的地址-elem_ptr在结构体struct_type中的偏移量
地址差即为结构体struct_type的起始地址

*/
/*链表节点，不需要数据元，只需要前驱和后继节点
链表的作用是将已有的数据以一定时序串起来，而不是为了存储，因此
不需要数据成员
*/
struct ListNode {
    struct ListNode* prev;
    struct ListNode* next;
};

struct List {
    struct ListNode head;
    struct ListNode tail;
};

/*自定义函数类型function，用于在list_traverse中做回调函数*/
typedef int (function)(struct ListNode*,int arg);

void list_init(struct List*);
void list_insert_before(struct ListNode*before,struct ListNode*elem);
void list_push(struct List* plist,struct ListNode* elem);
void list_iterate(struct List* plist);
void list_append(struct List* plist,struct ListNode* elem);
void list_remove(struct ListNode* pelem);
struct ListNode* list_pop(struct List* plist);
int list_empty(struct List* plist);
uint32_t list_len(struct List* plist);
struct ListNode* list_traverse(struct List* plist, function func,int arg);
int elem_find(struct List* plist,struct ListNode* obj_elem);
#endif