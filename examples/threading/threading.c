#include "threading.h"
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

// Optional: use these functions to add debug or error prints to your application
//#define DEBUG_LOG(msg,...)
#define DEBUG_LOG(msg,...) printf("threading: " msg "\n" , ##__VA_ARGS__)
#define ERROR_LOG(msg,...) printf("threading ERROR: " msg "\n" , ##__VA_ARGS__)

void* threadfunc(void* thread_param)
{

    // TODO: wait, obtain mutex, wait, release mutex as described by thread_data structure
    // hint: use a cast like the one below to obtain thread arguments from your parameter
    
    int lock;	
    int unlock;
    struct thread_data* thread_func_args = (struct thread_data *) thread_param;
    
    usleep(thread_func_args->wait_to_obtain_ms * 1000);
    lock = pthread_mutex_lock(thread_func_args->mutex);

    if (lock != 0) {
        ERROR_LOG("Failed mutex");
	thread_func_args->thread_succ = false;
	return thread_param;
    }
    
    DEBUG_LOG("Mutex Success");

    usleep(thread_func_args->wait_to_release_ms * 1000);
    unlock = pthread_mutex_unlock(thread_func_args->mutex);
    
    if (unlock != 0) {
        ERROR_LOG("Failed mutex");
	pthread_mutex_unlock(thread_func_args->mutex);
	thread_func_args->thread_succ = false;
	return thread_param;
    
    }

    DEBUG_LOG("Release success");
    thread_func_args->thread_succ = true;

    return thread_param;
}


bool start_thread_obtaining_mutex(pthread_t *thread, pthread_mutex_t *mutex,int wait_to_obtain_ms, int wait_to_release_ms)
{
    /**
     * TODO: allocate memory for thread_data, setup mutex and wait arguments, pass thread_data to created thread
     * using threadfunc() as entry point.
     *
     * return true if successful.
     *
     * See implementation details in threading.h file comment block
     */

     int create;
     struct thread_data *thread_args = malloc(sizeof(struct thread_data));
     
     if (thread_args == NULL) {
         ERROR_LOG("Allocation Error");
	 return false;
     
     }

     thread_args->mutex = mutex;
     thread_args->wait_to_obtain_ms = wait_to_obtain_ms;
     thread_args->wait_to_release_ms = wait_to_release_ms;
     thread_args->thread_succ = false;

     create = pthread_create(thread, NULL, threadfunc, (void *)thread_args);
     if (create != 0) {
         ERROR_LOG("Thread Failed");
	 free(thread_args);
	 return false;
     
     }


    return true;
}

