#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
//#include "libhardware-linux-x86-r128/include/hardware.h"
#include "libhardware-core-linux-m32-r186/include/hardware.h"
#define CTX_MAGIC 10
#define TIMER_IRQ 2

typedef void(func_t)(void*);

enum state {READY, ACTIVABLE, TERMINATED, BLOCKED};

struct ctx_s{

		void *esp;
		void *ebp;
		func_t *f;
		void *args;
		enum state state;
		unsigned char *stack;
		unsigned int magic;
		struct ctx_s *next;
		struct ctx_s *waitinglist;

};

struct sem_s{
	int cpt;
	struct ctx_s *waitinglist;
};

struct ctx_s *current_ctx=0;

void sem_init(struct sem_s *sm,unsigned int val){
 sm->cpt = val;
 sm->waitinglist = NULL;
}
	
static struct ctx_s *ctx_list=NULL;

void sem_down(struct sem_s *sem){
 _mask(15);
 sem->cpt--;
 if (sem->cpt < 0){
  if (sem->waitinglist == NULL) {
   sem->waitinglist = current_ctx;
   (sem->waitinglist)->state = BLOCKED;
  }
else {
 current_ctx->state = BLOCKED;
 struct ctx_s *tmp = sem->waitinglist;
 while(tmp->waitinglist != NULL) 
  tmp = tmp->waitinglist;
 tmp->waitinglist = current_ctx;
}
}
yield();
_mask(1);
}



void sem_up(struct sem_s *sem) {
	_mask(15);
	sem->cpt++;
	if (sem->cpt >= 0 ) {
	struct ctx_s *temp = sem->waitinglist;
	temp->state = READY;
	sem->waitinglist = temp->waitinglist;
	temp->waitinglist = NULL;
}
_mask(1);
}






int create_ctx(int stack_size, func_t f, void *args){

		_mask(15);
		struct ctx_s *new_ctx=malloc(sizeof(struct ctx_s));
		assert(!(new_ctx==NULL));
		new_ctx -> stack = malloc(stack_size);
		assert(!(new_ctx->stack==NULL));
		new_ctx -> esp = new_ctx->stack+stack_size-4;
		new_ctx -> ebp = new_ctx->stack+stack_size-4;
		new_ctx -> f=f;
		new_ctx -> state = READY;
		new_ctx -> args = args;
		new_ctx -> magic = CTX_MAGIC;
		new_ctx -> next = ctx_list;

		if (ctx_list == NULL){
			ctx_list = new_ctx;
			ctx_list->next = new_ctx;
		}

		else{
			new_ctx->next = ctx_list->next;
			ctx_list->next=new_ctx;
		}
		_mask(1);

		return 0;
}






void start_ctx(){

		current_ctx -> state=ACTIVABLE;
		current_ctx -> f(current_ctx -> args);
		current_ctx -> state=TERMINATED;
		
		yield();
}





void switch_to_ctx(struct ctx_s *ctx){

		_mask(15);
		assert(CTX_MAGIC);
		assert(ctx->state==READY||ctx->state==ACTIVABLE);

		if (!(current_ctx==0)){

			asm ("mov %%esp, %0" "\n\t"
			     "mov %%ebp, %1"
			     :"=r"(current_ctx -> esp)
			     ,"=r"(current_ctx -> ebp) );
		}

		current_ctx=ctx;

		asm ("mov %0, %%esp" "\n\t"
		     "mov %1, %%ebp"
		     ::"r"(current_ctx -> esp)
		     ,"r"(current_ctx -> ebp) );

		while(ctx->state == TERMINATED){
			if (ctx->next == current_ctx) exit(0);
			current_ctx->next = ctx->next;
			if (ctx==ctx_list) ctx_list=ctx->next;
			free(ctx->stack);
			free(ctx);
			ctx=current_ctx->next;
		}

		if (current_ctx -> state==READY){

			start_sched();

		_mask(1);
		}
}

void yield(){

		if (current_ctx == NULL) current_ctx=ctx_list;

		switch_to_ctx(current_ctx->next);
}

void start_sched(){
		
		init_hardware("/home/m1/berveglieri/TP_ASE/hw_config.ini");
		IRQVECTOR[TIMER_IRQ]= yield;
		while(1);

}
void f_ping(void *args){

		while(1) {
			printf("A") ;
			
			printf("B") ;
			
			printf("C") ;
			
		}
}

void f_pong(void *args){

		while(2) {
			printf("1") ;
			
			printf("2") ;
			
		}
}



int main(int argc, char *argv[]){

	create_ctx(16384, f_ping, NULL);
	create_ctx(16384, f_pong, NULL);
	start_sched();
	exit(EXIT_SUCCESS);
}
