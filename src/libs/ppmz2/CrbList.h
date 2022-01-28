#ifndef LIST_H
#define LIST_H

#define __declspec(X)

#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct LinkNode LinkNode;

struct LinkNode {
	LinkNode *Next,*Prev;
};

#define zLN_InitList(List)			do { (List)->Next = List; (List)->Prev = List; } while(0)
#define zLN_Cut(Node)				do { (Node)->Prev->Next = (Node)->Next; (Node)->Next->Prev = (Node)->Prev; zLN_InitList(Node); } while(0)
#define zLN_Fix(Node)				do { (Node)->Prev->Next = Node; (Node)->Next->Prev = Node; } while(0)
#define zLN_AddAfter(Node,List)		do { (Node)->Prev = List; (Node)->Next = (List)->Next; LN_Fix(Node); } while(0)
#define zLN_AddBefore(Node,List)	do { (Node)->Next = List; (Node)->Prev = (List)->Prev; LN_Fix(Node); } while(0)

#define LN_AddBefore(Node,List)		zLN_AddBefore((LinkNode *)Node,(LinkNode *)List)
#define LN_Cut(Node)				zLN_Cut((LinkNode *)Node)
#define LN_Fix(Node)				zLN_Fix((LinkNode *)Node)
#define LN_AddAfter(Node,List)		zLN_AddAfter((LinkNode *)Node,(LinkNode *)List)
#define LN_InitList(List)			zLN_InitList((LinkNode *)List)
#define LN_Next(Node)				(void *)(((LinkNode *)Node)->Next)
#define LN_Null(node)	LN_InitList(node)
__declspec(dllexport) LinkNode *	LN_CutTail(LinkNode *pList);
#define LN_AddHead(list,node)	LN_AddAfter(node,list)
#define LN_AddTail(list,node)	LN_AddBefore(node,list)

#ifdef __cplusplus
}
#endif

#endif  // LIST_H

