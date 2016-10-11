#include <stdio.h>
#include <stdlib.h>
#include <setjmp.h>
#include <assert.h>
#include <stdint.h>

#define CTX_MAGIC 0x1111


typedef int (func_t)(int);

static jmp_buf buf;
static struct ctx_s *current_ctx;
static struct ctx_s *ctx_list = NULL;
enum state {READY,ACTIVABLE,TERMINATED};

struct ctx_s{
  void *ebp;
  void *esp;
  func_t *f;
  enum state state;
  void * args;
  unsigned char *stack;
  unsigned int magic;
  struct ctx_s *next;
};


struct ctx_s ctx_ping;
struct ctx_s ctx_pong;

int create_ctx (int stack_size,func_t *f,void *args){

  struct ctx_s *new_ctx = malloc(sizeof(struct ctx_s));
  assert((new_ctx->magic) == CTX_MAGIC);
  
  new_ctx->stack = malloc (stack_size);
  assert(new_ctx->stack > 0); 
  new_ctx->f = f;
  new_ctx->args = args;
  new_ctx->state = READY;
  new_ctx->esp = (new_ctx->stack)+ stack_size -4;
  new_ctx->ebp = (new_ctx->stack)+ stack_size -4;
  new_ctx->magic = CTX_MAGIC;

  if (ctx_list == NULL) {
    ctx_list = new_ctx;
    ctx_list -> next = new_ctx;
  }
  else {
    new_ctx -> next = ctx_list -> next;
    ctx_list -> next = new_ctx;
  }
  return 1;

}

struct ctx_s ctx_ping;
struct ctx_s pong;

void f_ping(void *arg);
void f_pong(void *arg);


void yield() {
  if (current_ctx == NULL) {
	current_ctx = ctx_list;
	}
	switch_to_ctx(current_ctx -> next);
}	


void start_ctx(){
  current_ctx->state = ACTIVABLE;
  (current_ctx->f) (current_ctx->args);
  current_ctx->state = TERMINATED;
  yield();
}


void switch_to_ctx(struct ctx_s *ctx){

  if (current_ctx == NULL){
    while (ctx -> state == TERMINATED) {
      if (ctx -> next == current_ctx) {
	return 0;
      }
      current_ctx -> next = ctx -> next;
      if (ctx == ctx_list) {
	ctx_list = ctx -> next;
      }
      free(ctx -> stack);
      free(ctx);
    }
		
    assert(ctx->magic = CTX_MAGIC);
    if (current_ctx > 0) {
      asm("mov %%ebp,%0" "\n\t"
	  "mov %%esp,%1"
	  :"=r"(current_ctx->ebp),
	   "=r"(current_ctx->esp)
	  );
    }
    current_ctx = ctx;
    asm("mov %0,%%ebp" "\n\t"
	"mov %1,%%esp"
	::"r"(current_ctx->ebp),
	 "r"(current_ctx->esp)
	);

    if (current_ctx -> state == READY) {
      start_ctx();
    }
  }
}

void f_ping(void *args){
  while(1) {
    printf("A");
    switch_to_ctx(&ctx_pong);
    printf("B");
    switch_to_ctx(&ctx_pong);
    printf("C");
    switch_to_ctx(&ctx_pong);
  }
}

void f_pong(void *args){
  while(1){
    printf("1");
    switch_to_ctx(&ctx_ping);
    printf("2");
    switch_to_ctx(&ctx_ping);
  }
}
    

int main(int argc, char * argv[]){
  return 1;
}
