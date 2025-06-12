

#ifndef _THREAD_H_
#define _THREAD_H_

#include <setjmp.h>


#include <stdio.h>
#include <setjmp.h>
#include <signal.h>
#include <unistd.h>
#include <sys/time.h>
#include <stdbool.h>


#define MAX_THREAD_NUM 100 /* maximal number of threads */
#define STACK_SIZE 4096
#define READY 0
#define RUNNING 1
#define BLOCKED 2

typedef unsigned long address_t;



class Thread{



private:

  sigjmp_buf env;
  int quantumes;
  int thread_id;
  bool is_blocked;
  int status;
  char* stack;



#ifdef __x86_64__
/* code for 64 bit Intel arch */

  typedef unsigned long address_t;
#define JB_SP 6
#define JB_PC 7

/* A translation is required when using an address of a variable.
   Use this as a black box in your code. */
  address_t translate_address(address_t addr)
  {
    address_t ret;
    asm volatile("xor    %%fs:0x30,%0\n"
                 "rol    $0x11,%0\n"
    : "=g" (ret)
    : "0" (addr));
    return ret;
  }

#else
  /* code for 32 bit Intel arch */

typedef unsigned int address_t;
#define JB_SP 4
#define JB_PC 5


/* A translation is required when using an address of a variable.
   Use this as a black box in your code. */
address_t translate_address(address_t addr)
{
    address_t ret;
    asm volatile("xor    %%gs:0x18,%0\n"
                 "rol    $0x9,%0\n"
    : "=g" (ret)
    : "0" (addr));
    return ret;
}


#endif


public:

   Thread(address_t pc_counter, int thread_id);
  ~Thread(){delete[] stack;}
  int get_status() const{return this->status;}
  void set_status(int new_status){ this->status = new_status;}
  int get_thread_id() const{return this->thread_id;}
  sigjmp_buf & get_env(){return this->env;}
  bool get_block() const{return this->is_blocked;}
  void set_block(bool block){ this->is_blocked = block;}
  void add_quantum(){ this->quantumes ++;}
  int get_quantum() const{return this->quantumes;}



};

#endif //_THREAD_H_
