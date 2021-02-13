# User-level thread library Report



## Design Details

### Queue

The core data structure of the queue is a linked list with head and tail 
pointers (this is the basic data structure knowledge)

Be careful of program boundary conditions when writing code

And detect potential bugs by considering the test code as much as possible

### uthread

#### Use the following ways to understand the mechanism of uthread

* Read the reference resources on the webpage
* Read the framework code to try to guess their mechanism
* Search for some API information on the Internet (such as `ucontext.h`)

#### Some specific design details

* Running thread: `tcb_t current_tcb;`

* The scheduling queue of candidate threads: `queue_t candiates;`

The thread's runtime control data is stored in the `struct tcb` structure

Implement thread scheduling queue through `queue.c`,
switch the current working thread according to the FIFO principle,
and the main thread will also be scheduled

* `uthread_start(...)`: Initialize the `uthread` library
  * Initialize the necessary working variables
  * Generate the `tcb` for the current thread (main) for scheduling
* `uthread_stop(...)`: Stop `uthread` and release resources
  * Wait for all worker threads to finish and exit
  * Destroy variables and release resources
* `uthread_create(...)`: Create a new thread, append it to the dispatch queue
  * Create a new `tcb` structure, and allocate thread context and stack to it
  * Enqueue the `tcb` structure
* `uthread_yield(...)`: Perform the next round of thread switching
  * The current thread relinquishes control of the CPU
  * `yield` is responsible for maintaining the thread scheduling queue
    * To find a suitable candidate thread, it needs to meet:
      * The first thread in the `READY` / `BLOCKED` state
      * It currently does not join any other threads
        * (or the thread it joins have already run over)
    * The thread `t` will be removed from the `candidates` queue
      * `t` has finished running and became a ZOMBIE thread
      * The return value of `t` has been collected
        * (`joining_tid` of `t` will be set to `USHRT_MAX`)
  * `yield` performs the following operations after finding a candidate thread
    * Exchange `tcb_old` (previous worker) and `tcb_new` (candidate worker)
    * Re-enqueue the previous worker thread
    * Switch thread context
* `uthread_self(...)`: Returns the `TID` of the current thread
* `uthread_exit(...)`: Responsible for the finishing work after the thread ends
  * `uthread_ctx_bootstrap` uses `uthread_exit` to wrap the real worker `func`
  * Perform the finishing work at the end of the corresponding thread
  * Set the thread state to `ZOMBIE`
  * And execute the `yield` schedule at the end
* `uthread_join(...)`: Block the current thread until the `tid` thread exited
  * Set the current worker thread to the `BLOCKED` state
  * Call `uthread_yield(...)` to re-enter the scheduling process
  * Wait until the thread corresponding to `tid` exits
    * Collect its return value
    * And set its `joining_tid` to `USHRT_MAX`



## How I tested my project

I mainly use the following methods to test and debug my project:

* **Debug print**：
  * Print relevant variables and data at important locations
  * And use the macro `__DEBUG` switch to control whether they are displayed
* **Unit test**
  * Write and run unit tests located in the apps directory
* **Analyze the running process**
  * Think about the scheduling process of threads
  * and what may happen during the running process
* **Use gdb**
  * Detect pointer errors that are easily overlooked

