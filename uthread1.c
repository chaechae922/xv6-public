#include "types.h"
#include "stat.h"
#include "user.h"

#define FREE      0x0
#define RUNNING   0x1
#define RUNNABLE  0x2
#define STACK_SIZE 8192
#define MAX_THREAD 4

typedef struct thread {
  int sp;
  char stack[STACK_SIZE];
  int state;
} thread_t;

static thread_t all_thread[MAX_THREAD];
thread_t *current_thread;
thread_t *next_thread;

extern void thread_switch(void);
static void thread_schedule(void);
void thread_exit(void);

void thread_init(void) {
  int i;
  for(i = 0; i < MAX_THREAD; i++){
    all_thread[i].state = FREE;
    all_thread[i].sp = 0;  // Free 상태는 sp==0로 표시.
  }
  // uthread_init 시스템 콜을 통해 커널에 사용자 스케줄러 주소를 등록
  // (만약 sys_uthread_init의 cprintf를 제거했다면 메시지도 출력되지 않음)
  uthread_init(thread_schedule);
  current_thread = &all_thread[0];
  current_thread->state = RUNNING;
}

void thread_create(void (*func)()) {
  thread_t *t;
  int i;
  for(t = all_thread; t < all_thread + MAX_THREAD; t++){
    if(t->state == FREE)
      break;
  }
  t->sp = (int)(t->stack + STACK_SIZE);
  // 초기 컨텍스트 구성:
  // 먼저 ret 주소(함수가 시작되어야 하는 주소)로 func를 넣습니다.
  t->sp -= 4;
  *(int *)(t->sp) = (int)func;
  // popal로 복원될 8개 레지스터 공간 확보 (32바이트)
  t->sp -= 32;
  for(i = 0; i < 8; i++){
    ((int *)t->sp)[i] = 0;
  }
  t->state = RUNNABLE;
}

void thread_exit(void) {
  current_thread->state = FREE;
  // 사용자 스레드가 모두 종료되면 반복해서 스케줄링을 호출합니다.
  thread_schedule();
  exit();
}

static void thread_schedule(void) {
  int i, curr_index;
  thread_t *next = 0;
  
  curr_index = current_thread - all_thread;
  // 라운드로빈 방식: 현재 스레드 이후부터 순회하여 RUNNABLE 스레드 찾기
  for(i = 1; i < MAX_THREAD; i++){
    next = &all_thread[(curr_index + i) % MAX_THREAD];
    if(next->state == RUNNABLE)
      break;
    next = 0;
  }
  if(next == 0) {
    printf(1, "thread_schedule: no runnable threads\n");
    exit();
  }
  next->state = RUNNING;
  if(current_thread->state != FREE)
    current_thread->state = RUNNABLE;
  next_thread = next;
  thread_switch();
}

static void mythread(void) {
  int i;
  printf(1, "my thread running\n");
  // yield 호출을 넣지 않음 – 선점형 스케줄링에 맡김.
  for(i = 0; i < 100; i++){
    printf(1, "my thread 0x%x, iteration %d\n", (int)current_thread, i);
  }
  printf(1, "my thread: exit\n");
  thread_exit();
}

int main(int argc, char *argv[]) {
  thread_init();
  thread_create(mythread);
  thread_create(mythread);
  // 메인에서는 한번만 스케줄러 호출하여, 이후 타이머 인터럽트에 의해 yield()가 자동 호출되도록 함.
  thread_schedule();
  exit();
}
