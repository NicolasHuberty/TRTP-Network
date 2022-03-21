#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>

typedef struct node{
  struct node* next;
  pkt_t* pkt;
} node_t;

typedef struct fifo{
  struct node* tail;
  int size;
} fifo_t  ;

fifo_t* myFifo;

int enqueue(fifo_t* my_fifo, pkt_t* new_pkt){

	node_t *new_node = malloc(sizeof(node_t));
	new_node->pkt = new_pkt;
	if(my_fifo->size == 0){
		new_node->next = NULL;
		my_fifo->tail = new_node;
	}else{
		new_node->next = my_fifo->tail;
		my_fifo->tail = new_node;
	}
	my_fifo->size++;
	printf("enqueue: size = %d\n", my_fifo->size);
	printf("tail = %d\n", my_fifo->tail->pkt->type);
	return my_fifo->size;
}

pkt_t* dequeue(fifo_t* my_fifo){
	pkt_t* ret = malloc(sizeof(pkt_t));
	if(my_fifo->size == 0){
		return NULL;
	}
	else if(my_fifo->size == 1){
		ret = my_fifo->tail->pkt;
		printf("type dequeued = %d\n", ret->type);
		my_fifo->tail = NULL;
		my_fifo->size--;
		free(ret);
	}
	else{
		node_t* last = malloc(sizeof(node_t));
		last = my_fifo->tail;
		while(last->next->next != NULL){
			last = last->next;
		}
		ret = last->next->pkt;
		printf("type dequeued = %d\n", ret->type);
		my_fifo->size--;
		last->next = NULL;
	}
	printf("dequeue: size = %d\n", my_fifo->size);
	return ret;
}
