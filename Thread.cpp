#include "Thread.h"
#include "iostream"
using namespace std;
Thread::Thread (address_t pc_counter, int thread_id)
{
  this->quantumes = 0;
  this->thread_id = thread_id;
  this->is_blocked = false;
  this->stack = new char [STACK_SIZE];

  if (this->stack == nullptr){
      cout<< "error with using stack of: "<< this->thread_id << endl;
    }

// Regular thread
  if (this->thread_id != 0){
      address_t pc,sp;

      sp = (address_t) stack + STACK_SIZE - sizeof(address_t);
      pc = (address_t )pc_counter;
      sigsetjmp(env,1);
//
      (this->env->__jmpbuf)[JB_SP] = translate_address(sp);
      (this->env->__jmpbuf)[JB_PC] = translate_address(pc);
      sigemptyset(&env->__saved_mask);


    }
    // Checking validity of thread id:
  else if (this->thread_id == 0){
      sigsetjmp(env,1);

    }
  this->status = READY;

}
