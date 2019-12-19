/*
@Project memory allocator
@Author:Runyuan Yan
ruy16@pitt.edu
*/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>
#include "mymalloc.h"

// USE THIS GODDAMN MACRO OKAY
#define PTR_ADD_BYTES(ptr, byte_offs) ((void*)(((char*)(ptr)) + (byte_offs)))

// Don't change or remove these constants.
#define MINIMUM_ALLOCATION  16
#define SIZE_MULTIPLE       8
#define SIZE_HEADER			16

typedef struct Header{//16 -- padding
	unsigned int size;//4
	bool inUse;//1
	struct Header* next;//4B
	struct Header* previous;//4B
}Header;

Header* Heap_tail = NULL;
Header* Heap_head = NULL;
Header* last_allocated = NULL;
Header* biggest_free_block ;

unsigned int round_up_size(unsigned int data_size) {
	if(data_size == 0)
		return 0;
	else if(data_size < MINIMUM_ALLOCATION)
		return MINIMUM_ALLOCATION;
	else
		return (data_size + (SIZE_MULTIPLE - 1)) & ~(SIZE_MULTIPLE - 1);
}

void block_split(unsigned int size,Header* old){
	Header* new_block = PTR_ADD_BYTES(old,size+SIZE_HEADER);
	new_block->size = 0;
	new_block->inUse = false;
	new_block->next = NULL;
	new_block->previous = NULL;

	if(old == Heap_head){
		new_block -> size = old -> size - size - SIZE_HEADER;
		old -> size = size;
		new_block->next = old -> next;
		new_block->next->previous = new_block;
		old -> next = new_block;
		new_block -> previous = old;
	}
	else{
		new_block -> size = old -> size - size - SIZE_HEADER;
		old -> size = size;
		new_block-> next = old->next;
		new_block->next->previous=new_block;
		old -> next = new_block;
		new_block->previous = old;
	}
	biggest_free_block = old;
	/*printf("Head: %d %d bytes",Heap_head->inUse,Heap_head->size);
	printf("->: %d %d bytes",Heap_head->next->inUse,Heap_head->next->size);
	printf("->: %d %d bytes",Heap_head->next->next->inUse,Heap_head->next->next->size);
	printf("-> Tail: %d %d bytes",Heap_tail->inUse,Heap_tail->size);*/
}

void worst_find(unsigned int size){//
	//worst fit algorithm
	int biggest_size = 0;//reset it to 0
	
	for(Header* n = Heap_head;n != NULL;n = n -> next){
		if(!n->inUse && n->size > biggest_size){
			biggest_free_block = n;
			biggest_size = n ->size;
		}
		//printf("biggest free block is: %d",biggest_free_block->size);
	}
	if(biggest_size < size){
		biggest_free_block = NULL;
	}
	int size_differnce = biggest_size - (size + SIZE_HEADER);
	//printf("size difference: %d\n",size_differnce);
	if( size_differnce > SIZE_HEADER + 16){
		//printf("we found a block we can split:%d\n",biggest_size);
		block_split(size,biggest_free_block);
		}
}
//prints the heap in both order
void print_heap(){
	for(Header* n = Heap_head;n != NULL;n = n -> next){
		int inUseOrNot;
		switch(n->inUse){
			case true: inUseOrNot = 1;
			break;
			case false: inUseOrNot = 0;
			break;
		}
		printf("[%d bytes, inuse: %d] -> ",n->size,n->inUse);

	}
	printf("\n");
	/*printf("Backwards:\n");
	for(Header* n = Heap_tail;n != NULL;n = n -> previous){
		int inUseOrNot;
		switch(n->inUse){
			case true: inUseOrNot = 1;
			break;
			case false: inUseOrNot = 0;
			break;
		}
		printf("[%d bytes, inuse: %d] -> ",n->size,n->inUse);*/
	
	//printf("\n");
}
void* my_malloc(unsigned int size) {//only return the data part
	if(size == 0)
		return NULL;
	
	size = round_up_size(size);

	// ------- Don't remove anything above this line. -------
	// Here's where you'll put your code!
	Header* new_block;//makes a new heap block pointer
	worst_find(size);
	new_block = biggest_free_block;
	if(new_block == NULL ){//couldn't find a block, expand.
		new_block = sbrk(size + SIZE_HEADER); 
		new_block->size = size;
		new_block->inUse = true;
		Heap_tail = new_block;
		if(Heap_head != NULL){//if the heap is not empty
			last_allocated -> next = new_block;
			new_block->previous = last_allocated;
		}
		//if this is the first allocation,make this block the head
		if(Heap_head == NULL){
			Heap_head = new_block;
			Heap_tail = new_block;
		}
		last_allocated = new_block;//update global variable
		
		//printf("we couldn't find a free block so we made a new one, here is how the heap looks like now:\n");
		//print_heap();
		return PTR_ADD_BYTES(new_block,SIZE_HEADER);	
	}
	else{
		new_block->inUse = true;
		//printf("We found an aviliable block with size of %d, here it is\n",new_block->size);
		//print_heap();
		return PTR_ADD_BYTES(new_block,SIZE_HEADER);
	}	
}
Header* coalesce(Header* current_block)
{
	if(current_block->previous == NULL){
		if(current_block -> next != NULL){//case when this is the head block
			if(!current_block -> next -> inUse){
				//printf("We found empty neighboring block, size of %d on the right,coalesce it\n",current_block -> next -> size);
				current_block -> size = current_block -> size + current_block -> next ->size +SIZE_HEADER;
				current_block -> next = current_block -> next -> next;
				current_block -> next ->previous = current_block;
				Heap_head = current_block;
				return current_block;
			}
		}
		//else this must be the only block,do nothing
		return current_block;
	}
	else{
		if(current_block -> next == NULL){//case where this is the tail 
			if(!current_block -> previous ->inUse)
			{
				current_block -> previous ->size += current_block->size + SIZE_HEADER;
				Heap_tail = current_block-> previous;
				Heap_tail -> next = NULL;
				return(Heap_tail);
			}
			return current_block;
		}
		else{//case where the block is in the middle
			if(!current_block->previous->inUse || !current_block->next->inUse){
				if(!current_block->previous->inUse && current_block->next->inUse){//left is free
					current_block->previous->size += current_block->size + SIZE_HEADER;
					current_block->previous->next = current_block->next;
					current_block->next->previous = current_block->previous;
					return current_block->previous;
				}
				if(current_block->inUse && !current_block->next->inUse){//right is free
					current_block->size += current_block->next->size + SIZE_HEADER;
					current_block->next = current_block->next->next;
					current_block->next->previous = current_block;
					return current_block; 
				}
				else{//both are free
					current_block->previous->size += current_block->size+current_block->next->size + SIZE_HEADER*2;
					current_block->previous->next = current_block->next->next;
					current_block->previous->next->previous = current_block->previous;
					return current_block->previous;
				}
			}
			return current_block;
		}
	}
}
void my_free(void* ptr) {
	if(ptr == NULL)
		return;
	Header* h = ptr - SIZE_HEADER;//slide back by size of header to point at its header
	h -> inUse = false;
	h = coalesce(h);
	if(h == Heap_tail){
		if(h == Heap_head){//case if this is the only block 
			Heap_head = NULL;
			last_allocated = NULL;
		}
		else{//case where it s just the tail
			
			Heap_tail = Heap_tail -> previous;
			last_allocated = Heap_tail;
			Heap_tail -> next = NULL;
		}
		sbrk(-(h->size + SIZE_HEADER));//move the break down by size of the block(tail)
		//printf("The heap is contracted !\n");
	}
	//printf("freed:\n");
	//print_heap();
	// and here's where you free stuff.

}
