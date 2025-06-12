#include "uthreads.h"
#include <queue>
#include <iostream>
#include "list"
#include "Thread.h"
#include <signal.h>
#include <sys/time.h>
#include <map>

#define SECONDS_TO_MICRO 1000000
#define SUCCESS 0
#define FAIL -1
#define READY 0
#define RUNNING 1
#define BLOCKED 2

using namespace std;

int threads_amount = 0;
list<int> ready_threads;
int current_thread = 0;
int id_counter = 0;
int quantumes_overall = 0;
struct itimerval quantum_timer;
struct sigaction signal_handler;
priority_queue<int, vector<int>, greater<int> > min_threads;
map<int,Thread*> thread_by_id;

int error_handler_print (std::string error_msg, int error_type);
void signal_handler_time (int sig);
void time_reset ();
void make_switch_threads ();
void set_begin_time (int quantum_usecs);
void push_running_thread ();
/***
 * function returns minimal free id
 * @return
 */
int get_min_free_id(){

  int _id = 0;
  // If empty gives the counter
  if(min_threads.empty()){
      _id = id_counter;
      _id ++;
      id_counter++;
    }
    // Removing top(min) of queue value and increment by 1.
  else{
      _id = min_threads.top();
      min_threads.pop();
    }
  return _id;


}

/**
 * @brief initializes the thread library.
 *
 * You may assume that this function is called before any other thread library function, and that it is called
 * exactly once.
 * The input to the function is the length of a quantum in micro-seconds.
 * It is an error to call this function with non-positive quantum_usecs.
 *
 * @return On success, return 0. On failure, return -1.
*/
int uthread_init(int quantum_usecs){


  // Validation :
  if (quantum_usecs <= 0){
      return error_handler_print ("usecs time most be positive", 0);
    }
  // declaring signal handler and checking functionality
  signal_handler.sa_handler = &signal_handler_time;
  if (sigemptyset(&signal_handler.sa_mask) == FAIL){
      error_handler_print ("Set is not empty", 0);
    }
  if (sigaddset(&signal_handler.sa_mask,SIGVTALRM)){
      error_handler_print ("Add to set failed", 0);
    }
  if (sigaction(SIGVTALRM,&signal_handler, nullptr) == FAIL){
      error_handler_print ("sigaction failed", 0);
    }
  set_begin_time (quantum_usecs);


  // Start a virtual timer. It counts down whenever this process is executing.
  if (setitimer(ITIMER_VIRTUAL,&quantum_timer, nullptr)){
      error_handler_print ("setting timer failed", 0);
    }

  // Set main thread
  Thread * main_thread = new(nothrow) Thread((address_t) nullptr, 0);
  thread_by_id[0] = main_thread;
  // Add quantum counter
  quantumes_overall +=1;
  threads_amount++;
  // Set in dictionary and add main thread quantum running.
  thread_by_id[0]->add_quantum();
  thread_by_id[0]->set_status(RUNNING);
  return SUCCESS;

}
/**
 * Fucntion set timer interval for running
 * @param quantum_usecs
 */
void set_begin_time (int quantum_usecs)
{
  // Set timer for running:

  quantum_timer.it_value.tv_sec = quantum_usecs/SECONDS_TO_MICRO;
  quantum_timer.it_value.tv_usec = quantum_usecs%SECONDS_TO_MICRO;
  // Set timer expiration:
  quantum_timer.it_interval.tv_sec = quantum_usecs/SECONDS_TO_MICRO;
  quantum_timer.it_interval.tv_usec = quantum_usecs%SECONDS_TO_MICRO;
}
/**
 * Function handle the signal SIGVTALRM which happens when time interval over.
 * prepare for switching between threads
 * @param sig
 */
void signal_handler_time (int sig)
{
  // Close interfear in process:
  if (sigprocmask(SIG_BLOCK,&signal_handler.sa_mask, nullptr) == FAIL){
      error_handler_print ("sigprocmask process fail", 0);
    }
  time_reset();
  // Save current state of running thread:
  if (sigsetjmp(thread_by_id[current_thread]->get_env(), 1)){
      //unblocking
      if (sigprocmask(SIG_UNBLOCK,&signal_handler.sa_mask, nullptr)== FAIL){
          error_handler_print ("sigprocmask process fail", 0);
        }
      return;
    }
  // pushing running thread to back of ready list
  ready_threads.push_back(current_thread);
  // Change it state to ready from running:
  thread_by_id[current_thread]->set_status(READY);
  // Make a switch
  make_switch_threads ();

}

/**
 * Function switch between threads
 */
void make_switch_threads ()
{
  if (sigprocmask(SIG_BLOCK,&signal_handler.sa_mask, nullptr) == FAIL){
      error_handler_print ("sigprocmask failed", 0);
    }
  // Save running thread state:
//  ready_threads.push_back(current_thread);

  current_thread = ready_threads.front();
  ready_threads.pop_front();

  thread_by_id[current_thread]->set_status(RUNNING);
  // add time quantum
  thread_by_id[current_thread]->add_quantum();

  // Jump to relevant code.
  sigprocmask(SIG_UNBLOCK, &signal_handler.sa_mask, nullptr);
  siglongjmp(thread_by_id[current_thread]->get_env(), 1);


}
/**
 * Function called when need to make switch between threads
 * reset next thread quantum time
 */
void time_reset ()
{
  if(sigprocmask(SIG_BLOCK,&signal_handler.sa_mask, nullptr) == FAIL){
      error_handler_print ("sigprocmask error", 0);
    }
  // Save next thread time interval
  quantum_timer.it_value = quantum_timer.it_interval;
  quantumes_overall += 1;
  if (sigprocmask(SIG_UNBLOCK,&signal_handler.sa_mask, nullptr) == FAIL){
      error_handler_print ("sigprocmask failed", 0);
    }


}
/**
 * Function print error to user
 * @param error_msg
 * @param error_type Type of error : library of system
 * @return
 */
int error_handler_print (string error_msg, int error_type)
{
  if (error_type == -1){
      cerr << "uthreads Library error: " << error_msg << endl;
      exit(1);
    }
  else{
      cerr << "system error: " << error_msg << endl;
      return FAIL;
    }
}

/**
 * @brief Creates a new thread, whose entry point is the function entry_point with the signature
 * void entry_point(void).
 *
 * The thread is added to the end of the READY threads list.
 * The uthread_spawn function should fail if it would cause the number of concurrent threads to exceed the
 * limit (MAX_THREAD_NUM).
 * Each thread should be allocated with a stack of size STACK_SIZE bytes.
 *
 * @return On success, return the ID of the created thread. On failure, return -1.
*/
int uthread_spawn(thread_entry_point entry_point){

  // Blocking this critical area
  if (sigprocmask(SIG_BLOCK,&signal_handler.sa_mask, nullptr) == FAIL){
      return error_handler_print ("sigprocmask problem", 0);
    }
  // Checking validity:
  if (threads_amount >= MAX_THREAD_NUM){
      // unblock signals
      if (sigprocmask(SIG_UNBLOCK,&signal_handler.sa_mask, nullptr) == FAIL){
          error_handler_print ("sigprocmask problem", 0);
        }
      return error_handler_print ("error: to many threads to manage", 0);
    }
  if (entry_point == nullptr){
      return error_handler_print("spawn can not get null ptr",0);
    }
  // find new minimal thread id
  int new_min_id =  get_min_free_id();
  // creating new thread:

  Thread * my_thread = new(nothrow) Thread((address_t)(*entry_point), new_min_id);
  // validity check:
  if (my_thread == nullptr){
      //unblock
      if (sigprocmask(SIG_UNBLOCK,&signal_handler.sa_mask, nullptr) == FAIL){
          error_handler_print ("sigprocmask problem", 0);
          return FAIL;
        }
    }
  // Add to dictionary,and ready list
  thread_by_id[new_min_id] = my_thread;
  ready_threads.push_back(new_min_id);
  threads_amount ++;
  //unblocking
  if (sigprocmask(SIG_UNBLOCK,&signal_handler.sa_mask, nullptr) == FAIL){
      return error_handler_print ("sigprocmask problem", 0);
    }
  return new_min_id;
}

/**
 * @brief Terminates the thread with ID tid and deletes it from all relevant control structures.
 *
 * All the resources allocated by the library for this thread should be released. If no thread with ID tid exists it
 * is considered an error. Terminating the main thread (tid == 0) will result in the termination of the entire
 * process using exit(0) (after releasing the assigned library memory).
 *
 * @return The function returns 0 if the thread was successfully terminated and -1 otherwise. If a thread terminates
 * itself or the main thread is terminated, the function does not return.
*/
int uthread_terminate(int tid){

  // Block critical area:
  if (sigprocmask(SIG_BLOCK,&signal_handler.sa_mask, nullptr) == FAIL){
      error_handler_print ("sigprocmask problem", 0);
      return FAIL;
    }
  // Checks if main thread needed to be terminated:
  if (tid == 0){
      exit(0);
    }
  // removing thread from lst

  // Checking validity of input:
  if ((tid <= 0 || tid >= 100)||thread_by_id.find(tid) == thread_by_id.end()){
      //unlock thread signals
      if (sigprocmask(SIG_UNBLOCK,&signal_handler.sa_mask, nullptr) == FAIL){
          error_handler_print ("sigprocmask failed", 0);
        }
      return error_handler_print ("Such thread with tid is not exist ", 0);
    }
  Thread* thread_to_terminate = thread_by_id[tid];
  if (thread_to_terminate == nullptr){
      if (sigprocmask(SIG_UNBLOCK,&signal_handler.sa_mask, nullptr) == FAIL){
          return error_handler_print ("sigprocmask failed", 0);
        }
    }
  if (thread_to_terminate->get_status() == RUNNING){
      delete thread_by_id[tid];

      thread_by_id.erase(tid);
      time_reset();
      make_switch_threads();
    }
  else if (thread_to_terminate->get_status() == READY){
      ready_threads.remove(tid);
      thread_by_id.erase(tid);
//      delete thread_to_terminate;

    }
  else if (thread_to_terminate->get_status() == BLOCKED){
      thread_by_id.erase(tid);
//      delete thread_to_terminate;
    }

  // Add tid to available min heap and update amount of threads:
  min_threads.push(tid);
  threads_amount -= 1;

  // Unmask signals
  if (sigprocmask(SIG_UNBLOCK,&signal_handler.sa_mask, nullptr) == FAIL){
      error_handler_print ("sigprocmask error", 0);
    }
  return SUCCESS;


}
/**
 * @brief Blocks the thread with ID tid. The thread may be resumed later using uthread_resume.
 *
 * If no thread with ID tid exists it is considered as an error. In addition, it is an error to try blocking the
 * main thread (tid == 0). If a thread blocks itself, a scheduling decision should be made. Blocking a thread in
 * BLOCKED state has no effect and is not considered an error.
 *
 * @return On success, return 0. On failure, return -1.
*/
int uthread_block(int tid)
{
  int success1;
  int success2;
  if (sigprocmask (SIG_BLOCK, &signal_handler.sa_mask, nullptr) == FAIL)
    {
      success1 = error_handler_print ("sigprocmask failed", 0);
    }
  // Check validation
  // Check if tid not exist:
  if (thread_by_id.find (tid) == thread_by_id.end ()
      || (tid <= 0 || tid > MAX_THREAD_NUM))
    {
      success2 = error_handler_print ("thread with input tid is not valid", 0);
    }
  // Check if is it main thread
  if (tid == 0)
    {
      return error_handler_print ("can not block main thread", 0);
    }
  if (success2 == -1)
    {
      // Unblock
      if (sigprocmask (SIG_UNBLOCK, &signal_handler.sa_mask, nullptr) == FAIL)
        {
          error_handler_print ("sigprocmask failure", 0);
          return FAIL;
        }
    }
  // Check if trying to block thread which is already blocked
  if (thread_by_id[tid]->get_block ())
    {
      return 0;
    }
  // Case of ready thread need to be blocked.
  if (thread_by_id[tid]->get_status () == READY)
    {
      thread_by_id[tid]->set_block (true);
      thread_by_id[tid]->set_status (BLOCKED);
      ready_threads.remove (tid);
      if (sigprocmask (SIG_UNBLOCK, &signal_handler.sa_mask, nullptr) == FAIL)
        {
          return error_handler_print ("sigprocmask failed", 0);
        }

    }
  else if(thread_by_id[tid]->get_status () == RUNNING)
    {
      thread_by_id[tid]->set_status (BLOCKED);
      thread_by_id[tid]->set_block (true);
      if (sigsetjmp (thread_by_id[tid]->get_env (), 1))
        {
          if (sigprocmask (SIG_UNBLOCK, &signal_handler.sa_mask, nullptr)
              == FAIL)
            {
              return error_handler_print ("sigprocmask error", 0);
            }
        }
      time_reset ();
      make_switch_threads ();
    }


  // Check if in running state:
  return SUCCESS;

}




/**
 * @brief Blocks the RUNNING thread for num_quantums quantums.
 *
 * Immediately after the RUNNING thread transitions to the BLOCKED state a scheduling decision should be made.
 * After the sleeping time is over, the thread should go back to the end of the READY threads list.
 * The number of quantums refers to the number of times a new quantum starts, regardless of the reason. Specifically,
 * the quantum of the thread which has made the call to uthread_sleep isn’t counted.
 * It is considered an error if the main thread (tid==0) calls this function.
 *
 * @return On success, return 0. On failure, return -1.
*/
int uthread_sleep(int num_quantums){

  if (sigprocmask(SIG_BLOCK,&signal_handler.sa_mask, nullptr) == FAIL){
      error_handler_print ("sigprocmask failed", 0);

    }
  if (thread_by_id[current_thread]->get_thread_id() == 0){
      if (sigprocmask(SIG_UNBLOCK,&signal_handler.sa_mask, nullptr) == FAIL){
          error_handler_print ("sigprocmask failed", 0);
        }
      return error_handler_print ("Main cannot get to sleep", 0);

    }

  // sleep it
  int tid = current_thread;


  int _id = current_thread;
  // Waiting

  int sig;
  int  * ptr = &num_quantums;

  uthread_block(_id);


//  sigwait(&signal_handler.sa_mask,&num_quantums);
  uthread_resume(_id);
  time_reset();

  make_switch_threads ();



  if (thread_by_id[tid]->get_block()){
      if (sigprocmask(SIG_UNBLOCK,&signal_handler.sa_mask, nullptr) == FAIL){
          error_handler_print ("sigprocmask error", 0);
        }
      return SUCCESS;
    }
  thread_by_id[tid]->set_status(READY);
  ready_threads.push_back(_id);
//  thread_by_id[tid]->set_status(READY);
  if (sigprocmask(SIG_UNBLOCK,&signal_handler.sa_mask, nullptr) == FAIL){
      error_handler_print ("sigprocmask error", 0);
    }
  return SUCCESS;

}


/**
 * @brief Resumes a blocked thread with ID tid and moves it to the READY state.
 *
 * Resuming a thread in a RUNNING or READY state has no effect and is not considered as an error. If no thread with
 * ID tid exists it is considered an error.
 *
 * @return On success, return 0. On failure, return -1.
*/
int uthread_resume(int tid){

  // Unlock
  if(sigprocmask(SIG_BLOCK,&signal_handler.sa_mask, nullptr) == FAIL){
      return error_handler_print ("sigprocmask failed", 0);
    }
  // Validation for existing thread id:
  if (thread_by_id.find(tid) == thread_by_id.end()){
      error_handler_print ("Not such thread with tid", 0);
      return FAIL;
    }
  // Check if thread in ready/running position
  if (thread_by_id[tid]->get_status() == READY){
      return SUCCESS;
    }
  if (thread_by_id[tid]->get_status() == RUNNING){
      return SUCCESS;
    }
  thread_by_id[tid]->set_status(READY);
  thread_by_id[tid]->set_block(false);
  ready_threads.push_back(tid);

  if (sigprocmask(SIG_UNBLOCK,&signal_handler.sa_mask, nullptr) == FAIL){
      error_handler_print ("sigprocmask failed", 0);
    }
  return SUCCESS;


}

/**
 * @brief Returns the thread ID of the calling thread.
 *
 * @return The ID of the calling thread.
*/
int uthread_get_tid(){
  return thread_by_id[current_thread]->get_thread_id();
}


/**
 * @brief Returns the total number of quantums since the library was initialized, including the current quantum.
 *
 * Right after the call to uthread_init, the value should be 1.
 * Each time a new quantum starts, regardless of the reason, this number should be increased by 1.
 *
 * @return The total number of quantums.
*/
int uthread_get_total_quantums(){
  return quantumes_overall;
}

/**
 * @brief Returns the number of quantums the thread with ID tid was in RUNNING state.
 *
 * On the first time a thread runs, the function should return 1. Every additional quantum that the thread starts should
 * increase this value by 1 (so if the thread with ID tid is in RUNNING state when this function is called, include
 * also the current quantum). If no thread with ID tid exists it is considered an error.
 *
 * @return On success, return the number of quantums of the thread with ID tid. On failure, return -1.
*/
int uthread_get_quantums(int tid){
  return thread_by_id[tid]->get_quantum();
}











