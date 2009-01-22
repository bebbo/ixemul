#define _KERNEL
#include "ixemul.h"
#include <user.h>

void ixnewlist(struct ixlist *list)
{
  list->head = list->tail = NULL;
}

void ixaddtail(struct ixlist *list, struct ixnode *node)
{
  ixinsert(list, node, list->tail);
}

void ixaddhead(struct ixlist *list, struct ixnode *node)
{
  ixinsert(list, node, NULL);
}

void ixremove(struct ixlist *list, struct ixnode *node)
{
  if (node)
  {
    if (node->prev)
      node->prev->next = node->next;
    else
      list->head = node->next;
    if (node->next)
      node->next->prev = node->prev;
    else
      list->tail = node->prev;
  }
}

struct ixnode *ixremhead(struct ixlist *list)
{
  if (list)
  {
    struct ixnode *node = list->head;
  
    ixremove(list, node);
    return node;
  }
  return NULL;
}

void ixinsert(struct ixlist *list, struct ixnode *node, struct ixnode *after)
{
  if (list && node)
  {
    if (after)
    {
      node->prev = after;
      node->next = after->next;
      after->next = node;
    }
    else
    {
      node->next = list->head;
      node->prev = NULL;
      list->head = node;
    }
    if (node->next)
      node->next->prev = node;
    else
      list->tail = node;
  }
}
