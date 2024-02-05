#include <stdio.h>
#include "list.h"
#include <stdlib.h>

//This data structure serves as a linked list to track free list heads and nodes (think a stack with FILO, where the last node in the list is popped for use)
typedef struct listOfFree_s {
    struct listOfFree_s* next;
} listOfFree;

//Static arrays will store list heads and nodes
static List headArr[LIST_MAX_NUM_HEADS];
static Node nodeArr[LIST_MAX_NUM_NODES];

//Pointers to keep track of free nodes and heads in a singly-linked list structure
static listOfFree* freeLists = NULL;
static listOfFree* freeNodes = NULL;

//This function pushes a free/available list head into our freeLists list/stack
void pushFreeList(List* list) {
    struct listOfFree_s* newNode = (struct listOfFree_s*)list;
    newNode -> next = freeLists;
    freeLists = newNode;
}

//This function pops a free/available list head out of the freeLists list/stack if one exists, otherwise it returns NULL
List* popFreeList() {
    if (freeLists == NULL) {
        return NULL;
    }
    List* newList = (List*)freeLists;
    freeLists = freeLists -> next;
    return newList;
}

//This function pushes a free/available node into the freeNodes list/stack
void pushFreeNode(Node* node) {
    struct listOfFree_s* newNode = (struct listOfFree_s*)node;
    newNode -> next = freeNodes;
    freeNodes = newNode;
}

//This function pops a free/available node from the freeNodes list/stack if one exists, otherwise it returns NULL
Node* popFreeNode() {
    if (freeNodes == NULL) {
        return NULL;
    }
    Node* newNode = (Node*)freeNodes;
    freeNodes = freeNodes -> next;
    return newNode;
}

//This function creates a new list and initializes the data structures necessary for future list initialization
//Will run O(N) for the initial setup, and then create lists in constant O(1) time
List* List_create() {
    static int arraysInit = 0; //used to track if the arrays have been initialized 0 for false 1 for true
    
    if(!arraysInit) { 
        //Initialize array of list heads
        for (int i = 0; i < LIST_MAX_NUM_HEADS; i++) {
            headArr[i].current = NULL;
            headArr[i].first = NULL;

            pushFreeList(&headArr[i]); 
        }
        //Initialize array of list nodes
        for (int j = 0; j < LIST_MAX_NUM_NODES; j++) {
            nodeArr[j].data = NULL;
            nodeArr[j].next = NULL;
            nodeArr[j].prev = NULL;

            pushFreeNode(&nodeArr[j]);
        }
        arraysInit = 1; //Tracks the arrays as initialized to avoid step for future list creation
    }
    //If no more list heads are free, return NULL
    if (freeLists == NULL) {
        return NULL;
    }
    //Pop a free list from the free list/stack and initialize the values
    List* newList = popFreeList();
    if (newList != NULL) {
        newList -> current = NULL;
        newList -> first = NULL;
        newList -> last = NULL;
        newList -> count = 0;
    }
    return newList; //Return pointer to list
}


// Returns the number of items in pList.
int List_count(List* pList) {
    return pList -> count;
}

// Adds item to the end of pList, and makes the new item the current one. 
// Returns 0 on success, -1 on failure.
int List_append(List* pList, void* pItem) {
    //If the item or list are null or there are no more free nodes, return -1
    if (pList == NULL || pItem == NULL || freeNodes == NULL) {
        return -1;
    }
    //Grab a free node from the list/stack
    Node* newNode = popFreeNode();
    if (newNode == NULL) {
        return -1;
    }
        //Initialize all node values and add it to the end of the list
        newNode -> data = pItem;
        newNode -> next = NULL;
        //If the list is empty
        if (pList -> last == NULL) {
            pList -> first = newNode;
            pList -> last = newNode;
        } else { //if the list is populated add the node to the end of the list
            pList -> last -> next = newNode;
            newNode -> prev = pList -> last;
            pList -> last = newNode;  
        }
        //update current and count
        pList -> current = newNode;

        pList -> count++;
        return 0;
}

// Adds item to the front of pList, and makes the new item the current one. 
// Returns 0 on success, -1 on failure.
int List_prepend(List* pList, void* pItem) {
//If the item or list are null or there are no more free nodes, return -1
   if (pList == NULL || pItem == NULL || freeNodes == NULL) {
        return -1;
    }
    //Grab a free node from the list/stack
    Node* newNode = popFreeNode();
    if (newNode == NULL) {
        return -1;
    }
    //Initialize all node values and add it to the end of the list
    newNode -> data = pItem;
    newNode -> next = NULL;
    //If the list is empty
    if (pList -> first == NULL) {
        pList -> first = newNode;
        pList -> last = newNode;
        pList -> current = newNode;
    } else { //if the list is populated set the node at the beginning of the list
        pList -> first -> prev = newNode;
        newNode -> next = pList -> first;
        pList -> first = newNode;
        pList -> current = newNode;
    }
    //update current and count
    pList -> count++;
    return 0;
}

// Returns a pointer to the first item in pList and makes the first item the current item.
// Returns NULL and sets current item to NULL if list is empty.
void* List_first(List* pList) {
    //If the list is empty return NULL
    if (pList == NULL || pList -> first == NULL) {
        pList -> current = NULL;
        return NULL;
    }

    pList -> current = pList -> first;
    return pList -> first -> data;
}

// Returns a pointer to the last item in pList and makes the last item the current item.
// Returns NULL and sets current item to NULL if list is empty.
void* List_last(List* pList) {

    if (pList == NULL || pList -> first == NULL) {
        pList -> current = NULL;
        return NULL;
    }

    pList -> current = pList -> last;
    return pList -> last -> data;
}

// Advances pList's current item by one, and returns a pointer to the new current item.
// If this operation advances the current item beyond the end of the pList, a NULL pointer 
// is returned and the current item is set to be beyond end of pList.
void* List_next(List* pList) {
    //Sets the out of bounds flag and pointer to NULL if it goes out of bounds
    if (pList -> current == NULL) {
        pList -> boundsFlag = LIST_OOB_END;
        return NULL;
    }
    //Do the same as above if the move takes you out of bounds
    if (pList -> current -> next == NULL) {
        pList -> boundsFlag = LIST_OOB_END;
        pList -> current = NULL;
        return NULL;
    }

    pList -> current = pList -> current -> next;
    return pList -> current -> data;
}

// Backs up pList's current item by one, and returns a pointer to the new current item. 
// If this operation backs up the current item beyond the start of the pList, a NULL pointer 
// is returned and the current item is set to be before the start of pList.
void* List_prev(List* pList) {
    //Sets the out of bounds flag and pointer to NULL if it goes out of bounds
    if (pList -> current == NULL) {
        pList -> boundsFlag = LIST_OOB_START;
        return NULL;
    }
    //Do the same as above if the move takes you out of bounds
    if (pList -> current -> prev == NULL) {
        pList -> boundsFlag = LIST_OOB_START;
        pList -> current = NULL;
        return NULL;
    }

    pList -> current = pList -> current -> prev;
    return pList -> current -> data;
}

// Returns a pointer to the current item in pList.
void* List_curr(List* pList) {
    if (pList != NULL && pList -> current != NULL) {
        return pList -> current -> data;
    }
    return NULL;
}

// Adds the new item to pList directly after the current item, and makes item the current item. 
// If the current pointer is before the start of the pList, the item is added at the start. If 
// the current pointer is beyond the end of the pList, the item is added at the end. 
// Returns 0 on success, -1 on failure.
int List_insert_after(List* pList, void* pItem) {
    //If the item or list are null or there are no more free nodes, return -1
    if (pList == NULL || pItem == NULL || freeNodes == NULL) {
        return -1;
    }
    //Grab a free node from the list/stack of free nodes
    Node* newNode = popFreeNode();
    if (newNode == NULL) {
        return -1;
    }
    //Set the node to have your item's value
    newNode -> data = pItem;
    //If out of bounds (before start) then we add the item to the start
    if (pList -> current == NULL) {
        if (pList -> boundsFlag == LIST_OOB_START) {
            newNode -> next = pList -> first;
            pList -> first -> prev = newNode;
            pList -> first = newNode;
            pList -> current = newNode;
        } else { //Otherwise if it is past the end OOB we set the node at the end of the list
            newNode -> prev = pList -> last;
            pList -> last -> next = newNode;
            pList -> last = newNode;
        }
    } else { //If it is within bounds, then we insert the node and update accordingly
        newNode -> prev = pList -> current;
        if (pList -> current -> next != NULL) {
            newNode -> next = pList -> current -> next;
        }
        pList -> current -> next = newNode;
        pList -> current = newNode;
    }

    pList -> count++;
    return 0;
}

// Adds item to pList directly before the current item, and makes the new item the current one. 
// If the current pointer is before the start of the pList, the item is added at the start. 
// If the current pointer is beyond the end of the pList, the item is added at the end. 
// Returns 0 on success, -1 on failure.
int List_insert_before(List* pList, void* pItem) {
    //If the item or list are null or there are no more free nodes, return -1
    if (pList == NULL || pItem == NULL || freeNodes == NULL) {
        return -1;
    }
    //Grab a free node from the list/stack of free nodes
    Node* newNode = popFreeNode();
    if (newNode == NULL) {
        return -1;
    }
    //Set the node to have your item's value
    newNode -> data = pItem;
    //If out of bounds (before start) then we add the item to the start
    if (pList -> current == NULL) {
        if (pList -> boundsFlag == LIST_OOB_START) {
            newNode -> next = pList -> first;
            pList -> first -> prev = newNode;
            pList -> first = newNode;
            pList -> current = newNode;
        } else { //Otherwise if it is past the end OOB we set the node at the end of the list
            newNode -> prev = pList -> last;
            pList -> last -> next = newNode;
            pList -> last = newNode;
        }
    } else { //If it is within bounds, then we insert the node and update accordingly
        newNode -> next = pList -> current;
        if (pList -> current -> prev != NULL) {
            newNode -> prev = pList -> current -> prev;
            pList -> current -> prev -> next = newNode;
        }
        pList -> current -> prev = newNode;
        pList -> current = newNode;
    }

    pList -> count++;
    return 0;
}

// Return current item and take it out of pList. Make the next item the current one.
// If the current pointer is before the start of the pList, or beyond the end of the pList,
// then do not change the pList and return NULL.
void* List_remove(List* pList) {
    //return if beyond or before list, or if list is empty
    if (pList == NULL || pList -> current == NULL) {
        return NULL;
    }

    void* current = pList -> current -> data;
    Node* removeThisNode = pList -> current;
    //if a single item is left, then empty the list
    if(pList -> count == 1) {
        pList -> current = NULL;
        pList -> boundsFlag = LIST_OOB_END;
        pList -> first = NULL;
        pList -> last = NULL;
    } else if (pList -> current == pList -> first) { //if current is set on the first item do this
        pList -> first = pList -> current -> next;
        pList -> current -> next -> prev = NULL;
        pList -> current = pList -> first;
    } else if (pList -> current == pList -> last) { //if current is set on the last item do this
        pList -> last = pList -> current -> prev;
        pList -> current -> prev -> next = NULL;
        pList -> current = pList -> last;
    } else { //if current is somewhere in between first and last then do this
        pList -> current -> prev -> next = pList -> current -> next;
        pList -> current -> next -> prev = pList -> current -> prev;
        pList -> current = pList -> current -> next;
    }
    //free the node for future use
    pushFreeNode(removeThisNode);

    pList -> count--;
    return current;
}

// Return last item and take it out of pList. Make the new last item the current one.
// Return NULL if pList is initially empty.
void* List_trim(List* pList) {
    //if the list is empty return NULL
    if (pList == NULL || pList -> count == 0) {
        return NULL;
    }
    //select the last item + node
    void* lastItem = pList -> last -> data;
    Node* removeNode = pList -> last;

    if (pList -> count > 1) { //if there are multiple nodes do this
        pList -> last -> prev -> next = NULL;
        pList -> last = pList -> last -> prev;
    } else { //if there is a single node then reset the list and set out of bounds 
        pList -> last = NULL;
        pList -> first = NULL;
        pList -> current = NULL;
        pList -> boundsFlag = LIST_OOB_END;
    }
    pushFreeNode(removeNode);
    pList -> count--;
    return lastItem;
}

// Adds pList2 to the end of pList1. The current pointer is set to the current pointer of pList1. 
// pList2 no longer exists after the operation; its head is available
// for future operations.
void List_concat(List* pList1, List* pList2) {
    //if the second list is empty
    if (pList2 -> count == 0) {
        pushFreeList(pList2);
        return;
    }
    //if the first list is empty do this
    if (pList1 -> count == 0) {
        pList1 -> first = pList2 -> first;
        pList1 -> last = pList2 -> last;
        pList1 -> count = pList2 -> count;
    } else { //if the lists are both populated, add them together
            pList1 -> last -> next = pList2 -> first;
            pList2 -> first -> prev = pList1 -> last;
            pList1 -> last = pList2 -> last;
            pList1 -> count += pList2 -> count;
    }
    //Free the unused list head
    pushFreeList(pList2);
}

// Delete pList. pItemFreeFn is a pointer to a routine that frees an item. 
// It should be invoked (within List_free) as: (*pItemFreeFn)(itemToBeFreedFromNode);
// pList and all its nodes no longer exists after the operation; its head and nodes are 
// available for future operations.
typedef void (*FREE_FN)(void* pItem);
void List_free(List* pList, FREE_FN pItemFreeFn) {
    //Until we reach the last item (we will be moving first up the list until we reach last)
    while (pList -> first != NULL) {
        //select the next node in the list
        Node* nextNode = pList -> first -> next;
        //if dynamically allocated then use the provided freeing function
        if (pItemFreeFn != NULL) {
            (*pItemFreeFn)(pList->first->data);
        } else {
        //Free the node manually if it wasnt dynamically allocated
        pList -> first -> data = NULL;
        pList -> first -> next = NULL;
        pList -> first -> prev = NULL;
        }
        //push the free node back for future use
        pushFreeNode(pList -> first);
        pList -> first = nextNode;
    }
    //Free up the list metadata and the free head back for future use
    pList -> last = NULL;
    pList -> first = NULL;
    pList -> current = NULL;
    pList -> count = 0;
    pushFreeList(pList);
}

// Search pList, starting at the current item, until the end is reached or a match is found. 
// In this context, a match is determined by the comparator parameter. This parameter is a
// pointer to a routine that takes as its first argument an item pointer, and as its second 
// argument pComparisonArg. Comparator returns 0 if the item and comparisonArg don't match, 
// or 1 if they do. Exactly what constitutes a match is up to the implementor of comparator. 
// 
// If a match is found, the current pointer is left at the matched item and the pointer to 
// that item is returned. If no match is found, the current pointer is left beyond the end of 
// the list and a NULL pointer is returned.
// 
// If the current pointer is before the start of the pList, then start searching from
// the first node in the list (if any).
typedef bool (*COMPARATOR_FN)(void* pItem, void* pComparisonArg);

void* List_search(List* pList, COMPARATOR_FN pComparator, void* pComparisonArg) {
    //If the list is empty, or the item to compare to is NULL or the provided comparator function is NULL then return
    if (pList == NULL || pComparisonArg == NULL || pComparator == NULL) {
        return NULL;
    }
    //If out of bounds then bring back to start
    if (pList -> current == NULL && pList -> boundsFlag == LIST_OOB_START) {
        pList -> current = pList -> first;
    }
    //While current hasn't reached the end
    while (pList -> current != NULL) {
        //Compare them with the provided function
        if ((*pComparator)(pList -> current -> data, pComparisonArg) == 1) {
            return(pList -> current -> data);
        } else {
            //move on to the next item in the list
            pList -> current = pList -> current -> next;
        }
    }
    //at the end set it out of bounds
    pList -> boundsFlag = LIST_OOB_END;
    return NULL;
}
