#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>

#include "dyn_array.h"
#include "processing_scheduling.h"

// questions
// does the bin file analysis require an overhaul of all functions in order to be sensitive to arrival time?
// does priority get any tests???
// replace sum with time, in case of gapped arrivals


// You might find this handy.  I put it around unused parameters, but you should
// remove it before you submit. Just allows things to compile initially.
#define UNUSED(x) (void)(x)
//#define DEBUG puts
#define DEBUG noput
//#define DEBUG_BRANCH 1
#define DEBUG_BRANCH 0

// Status, error, progress tags make debug output better

void noput(char * str) { 
    UNUSED(str);
}

// private function
void virtual_cpu(ProcessControlBlock_t *process_control_block) 
{
    // decrement the burst time of the pcb
    --process_control_block->remaining_burst_time;
}

// Global var

/* GENERAL HELPER FUNCTIONS*/

void print_dyn_array(void * const a, void * b) {
    int * count = (int*) b;
    ProcessControlBlock_t block = *(ProcessControlBlock_t*) a;
    printf("NUMBER : %d  BURST_TIME : %d  ARRIVAL_TIME : %d  PRIORITY : %d\n", *count,
           block.remaining_burst_time, block.arrival, block.priority);
    (*count)++;
}

void sum_total_burst_time(void * const a, void * b) {
    int * sum = (int*) b;
    ProcessControlBlock_t block = *(ProcessControlBlock_t*) a;
    *sum += block.remaining_burst_time;
}

int compare_arrival_time(const void * a, const void * b) {
    return ((ProcessControlBlock_t*)a)->arrival - ((ProcessControlBlock_t*)b)->arrival;
}

int compare_remaining_burst_time(const void * a, const void * b) {
    return ((ProcessControlBlock_t*)a)->remaining_burst_time - ((ProcessControlBlock_t*)b)->remaining_burst_time;
}

int compare_priority(const void * a, const void * b) {
    return ((ProcessControlBlock_t*)a)->priority - ((ProcessControlBlock_t*)b)->priority;
}

// struct and function to help with calculating wait times without modifying pcb format or adding wrapper
typedef struct {
    float time;
    float interval;
    float total_wait;
} Wait_struct;

void calculate_interupted_wait(void * const element, void * wait_structure) {
    ProcessControlBlock_t * current = (ProcessControlBlock_t*)element;
    Wait_struct * waiter = (Wait_struct*)wait_structure;
    if(current->started) {
        waiter->total_wait = waiter->total_wait + waiter->interval;
    } else {
        // problem here, "unscheduled blocks" are doing this step more than once
        waiter->total_wait = waiter->total_wait + (waiter->time - (float)current->arrival);
        current->started = true;
    }
}

/**
    This function implements first come first serve scheduling
    There is error checking and debug functions
    Nothing too special, a do while loop is used to calc scheduling metrics

*/
/*
bool first_come_first_serve(dyn_array_t *ready_queue, ScheduleResult_t *result) {
    UNUSED(ready_queue);
    UNUSED(result);
    return false;
}
*/
bool first_come_first_serve(dyn_array_t *ready_queue, ScheduleResult_t *result) 
{
    static int counter = 1;  // this is a debug variable only
    if(ready_queue == NULL || result == NULL) {  // check for bad params
        DEBUG("error: passed invalid param");
        return false;
    }
    int size = dyn_array_size(ready_queue);
    if(size <= 0) {  // check for empty dyn array
        DEBUG("error: passed ready q size 0 or below");
        return false;
    }
    if(DEBUG_BRANCH) {  // debug print info
        int count = 1;
        puts("Printing initial state");
        dyn_array_for_each(ready_queue, print_dyn_array, (void*)&count);
        puts("");
    }

    // initializing variables
    int sum = 0;
    dyn_array_for_each(ready_queue, sum_total_burst_time, (void*)&sum);
    dyn_array_sort(ready_queue, compare_arrival_time);
    float time = 0;
    float total_wait = 0;
    float total_turnaround = 0;

    if(DEBUG_BRANCH) {  // debug print info
        int count = 1;
        puts("Printing post sort state");
        dyn_array_for_each(ready_queue, print_dyn_array, (void*)&count);
        puts("");
    }

    do {  // q starts from back of dyn array, loop summing up wait times and turnaround times
        ProcessControlBlock_t current; 
        dyn_array_extract_back(ready_queue, (void * const)&current);
        total_wait = total_wait + time;
        total_turnaround = total_turnaround + time + current.remaining_burst_time;
        time += current.remaining_burst_time;
        if(DEBUG_BRANCH) {
            printf("Round %d  Time %f total_turnaround %f total wait %f\n", 
            counter, time, total_turnaround, total_wait);
            counter++;
        }
    } while(!dyn_array_empty(ready_queue));

    // sending metrics to result
    result->average_waiting_time = total_wait / size;
    result->average_turnaround_time = total_turnaround / size;
    result->total_run_time = sum;

    if(DEBUG_BRANCH) {  // more debug stuff
        printf("Total runtime = %d\n", sum);
        printf("Size of dyn array = %d\n", size);
        int a = 1;
        dyn_array_for_each(ready_queue, print_dyn_array, (void*)&a);
        printf("Avg wait = %f\n", result->average_waiting_time);
        printf("Avg turnaround = %f\n", result->average_turnaround_time);
    }

    return true;
}

/**
    This function takes the "ready queue" and transfers process control blocks to the "wait queue"
    as they arrive according to the time count.
    This terminology is backwards, but I work with what I got.
    Also, because of my janky implementation, it has an option to transfer to the front or the back of the queue
    because in RR case it matters. push_to_back
*/
bool transfer_to_wait_queue_arrival(dyn_array_t * ready_queue, dyn_array_t * wait_queue, float time, bool push_to_back) {
    if(ready_queue == NULL || wait_queue == NULL || time < 0) {
        DEBUG("Error: invalid params at transfer to wait queue");
        return false;
    }
    if(dyn_array_empty(ready_queue)) {
        DEBUG("Status: passed empty ready queue to transfer to wait queue");
        return false;
    }

    ProcessControlBlock_t hold;
    void * hold_ptr = &hold;
    bool time_check = true;
    if(push_to_back) {
        do {
        if(!dyn_array_empty(ready_queue)){
            dyn_array_extract_front(ready_queue, hold_ptr);
        } else {
            hold_ptr = NULL;
        }
        
        if(DEBUG_BRANCH && hold_ptr != NULL) {
            printf("Transfer_to_wait_queue compare -> Time : %f  Arrival : %d\n", time, ((ProcessControlBlock_t*)hold_ptr)->arrival);
        } else if (DEBUG_BRANCH) {
            puts("Error: passed hold null ptr to transfer to wait queue time compare");
        }
        if(hold_ptr != NULL && (int)((ProcessControlBlock_t*)hold_ptr)->arrival <= (int)time) {
            dyn_array_push_back(wait_queue, hold_ptr);
            DEBUG("PROGRESS: pushed block into wait queue");
        } else {
            dyn_array_push_front(ready_queue, hold_ptr);
            DEBUG("PROGRESS: did not push additional block");
            time_check = false;
        }
        } while (time_check);
    } else {
        do {
            if(!dyn_array_empty(ready_queue)){
                dyn_array_extract_front(ready_queue, hold_ptr);
            } else {
                hold_ptr = NULL;
            }
            
            if(DEBUG_BRANCH && hold_ptr != NULL) {
                printf("Transfer_to_wait_queue compare -> Time : %f  Arrival : %d\n", time, ((ProcessControlBlock_t*)hold_ptr)->arrival);
            } else if (DEBUG_BRANCH) {
                puts("Error: passed hold null ptr to transfer to wait queue time compare");
            }
            if(hold_ptr != NULL && (int)((ProcessControlBlock_t*)hold_ptr)->arrival <= (int)time) {
                dyn_array_push_front(wait_queue, hold_ptr);
                DEBUG("PROGRESS: pushed block into wait queue");
            } else {
                dyn_array_push_front(ready_queue, hold_ptr);
                DEBUG("PROGRESS: did not push additional block");
                time_check = false;
            }
        } while (time_check);
    }
    return true;
}
/*
bool shortest_job_first(dyn_array_t *ready_queue, ScheduleResult_t *result) {
    UNUSED(ready_queue);
    UNUSED(result);
    return false;
}
*/

// This is similar to FCFS except a sort for job length is added
// also a timer is added in this function to check for late arriving pcbs
// this is done by adding a "wait queue," that gets pcbs from the ready queue
// as they arrive
bool shortest_job_first(dyn_array_t *ready_queue, ScheduleResult_t *result) 
{
    static int counter = 1;  // this is a debug variable only
    if(ready_queue == NULL || result == NULL) {  // check for bad params
        DEBUG("error: passed invalid param");
        return false;
    }
    int size = dyn_array_size(ready_queue);
    if(size <= 0) {  // check for empty dyn array
        DEBUG("error: passed ready q size 0 or below");
        return false;
    }
    if(DEBUG_BRANCH) {  // debug print info
        int count = 1;
        puts("Printing initial state");
        dyn_array_for_each(ready_queue, print_dyn_array, (void*)&count);
        puts("");
    }

    // initializing variables
    int sum = 0;
    dyn_array_for_each(ready_queue, sum_total_burst_time, (void*)&sum);
    dyn_array_sort(ready_queue, compare_arrival_time); // the added sort
    float time = 0;
    float total_wait = 0;
    float total_turnaround = 0;
    // wait queue here
    dyn_array_t * wait_queue = dyn_array_create(dyn_array_size(ready_queue), sizeof(ProcessControlBlock_t), NULL);
    if(wait_queue == NULL) {
        DEBUG("Error: malloc error at wait queue");
        return false;
    }

    if(DEBUG_BRANCH) {  // debug print info
        int count = 1;
        puts("Printing post sort state");
        dyn_array_for_each(ready_queue, print_dyn_array, (void*)&count);
        puts("");
    }

    do {  
        if(!dyn_array_empty(ready_queue)) { // ready -> wait
            transfer_to_wait_queue_arrival(ready_queue, wait_queue, time, false);
            dyn_array_sort(wait_queue, compare_remaining_burst_time);
        }

        if(DEBUG_BRANCH) {
            int count = 1;
            puts("Status: printing wait queue");
            dyn_array_for_each(wait_queue, print_dyn_array, (void*)&count);
        }
        
        if(!dyn_array_empty(wait_queue)) {
            ProcessControlBlock_t current; 
            dyn_array_extract_front(wait_queue, (void * const)&current);
            total_wait = total_wait + time - current.arrival;
            total_turnaround = total_turnaround + time + current.remaining_burst_time - current.arrival;
            time += current.remaining_burst_time;
        } else { // if no job in wait queue, time++
            time++;
        }
        if(DEBUG_BRANCH) {
            printf("Round %d  Time %f total_turnaround %f total wait %f\n", 
            counter, time, total_turnaround, total_wait);
            counter++;
        }
    } while(!(dyn_array_empty(ready_queue) && dyn_array_empty(wait_queue)));

    // sending metrics to result
    result->average_waiting_time = total_wait / size;
    result->average_turnaround_time = total_turnaround / size;
    result->total_run_time = sum;

    dyn_array_destroy(wait_queue);

    if(DEBUG_BRANCH) {  // more debug stuff
        printf("Total runtime = %d\n", sum);
        printf("Size of dyn array = %d\n", size);
        int a = 1;
        dyn_array_for_each(ready_queue, print_dyn_array, (void*)&a);
        printf("Avg wait = %f\n", result->average_waiting_time);
        printf("Avg turnaround = %f\n", result->average_turnaround_time);
    }

    return true; 
}

// Missing tests for priority
/*
bool priority(dyn_array_t *ready_queue, ScheduleResult_t *result) {
    UNUSED(ready_queue);
    UNUSED(result);
    return false;
}
*/

// I did this one too :)
bool priority(dyn_array_t *ready_queue, ScheduleResult_t *result) 
{
    static int counter = 1;  // this is a debug variable only
    if(ready_queue == NULL || result == NULL) {  // check for bad params
        DEBUG("error: passed invalid param");
        return false;
    }
    int size = dyn_array_size(ready_queue);
    if(size <= 0) {  // check for empty dyn array
        DEBUG("error: passed ready q size 0 or below");
        return false;
    }
    if(DEBUG_BRANCH) {  // debug print info
        int count = 1;
        puts("Printing initial state");
        dyn_array_for_each(ready_queue, print_dyn_array, (void*)&count);
        puts("");
    }

    // initializing variables
    int sum = 0;
    dyn_array_for_each(ready_queue, sum_total_burst_time, (void*)&sum);
    dyn_array_sort(ready_queue, compare_arrival_time);
    float time = 0;
    float total_wait = 0;
    float total_turnaround = 0;

    dyn_array_t * wait_queue = dyn_array_create(dyn_array_size(ready_queue), sizeof(ProcessControlBlock_t), NULL);
    if(wait_queue == NULL) {
        DEBUG("Error: malloc error at wait queue");
        return false;
    }

    if(DEBUG_BRANCH) {  // debug print info
        int count = 1;
        puts("Printing post sort state");
        dyn_array_for_each(ready_queue, print_dyn_array, (void*)&count);
        puts("");
    }

    do {  // q starts from back of dyn array, loop summing up wait times and turnaround times
        if(!dyn_array_empty(ready_queue)) {
            transfer_to_wait_queue_arrival(ready_queue, wait_queue, time, false);
            dyn_array_sort(wait_queue, compare_priority);
        }

        if(DEBUG_BRANCH) {
            int count = 1;
            puts("Status: printing wait queue");
            dyn_array_for_each(wait_queue, print_dyn_array, (void*)&count);
        }
        
        if(!dyn_array_empty(wait_queue)) {
            ProcessControlBlock_t current; 
            dyn_array_extract_front(wait_queue, (void * const)&current);
            total_wait = total_wait + time - current.arrival;
            total_turnaround = total_turnaround + time + current.remaining_burst_time - current.arrival;
            time += current.remaining_burst_time;
        } else {
            time++;
        }
        if(DEBUG_BRANCH) {
            printf("Round %d  Time %f total_turnaround %f total wait %f\n", 
            counter, time, total_turnaround, total_wait);
            counter++;
        }
    } while(!(dyn_array_empty(ready_queue) && dyn_array_empty(wait_queue)));

    // sending metrics to result
    result->average_waiting_time = total_wait / size;
    result->average_turnaround_time = total_turnaround / size;
    result->total_run_time = sum;

    if(DEBUG_BRANCH) {  // more debug stuff
        printf("Total runtime = %d\n", sum);
        printf("Size of dyn array = %d\n", size);
        int a = 1;
        dyn_array_for_each(ready_queue, print_dyn_array, (void*)&a);
        printf("Avg wait = %f\n", result->average_waiting_time);
        printf("Avg turnaround = %f\n", result->average_turnaround_time);
    }

    return true; 
}

/*
bool round_robin(dyn_array_t *ready_queue, ScheduleResult_t *result, size_t quantum) {
    UNUSED(ready_queue);
    UNUSED(result);
    UNUSED(quantum);
    return false;
}
*/
    
// This works by maintaining a wait queue and ready queue
// when processes arrive they are pushed in order into the wait queue
// then every tick of the function the burst time is decremented and the 
// ready queue is checked for new pcbs
// when the time quantum expires the current pcb is pushed onto the queue and 
// the next is transfered to the current variable
bool round_robin(dyn_array_t *ready_queue, ScheduleResult_t *result, size_t quantum) 
{
    static int counter = 1;  // this is a debug variable only
    if(ready_queue == NULL || result == NULL || quantum == 0) {  // check for bad params
        DEBUG("error: passed invalid param");
        return false;
    }
    int size = dyn_array_size(ready_queue);
    if(size <= 0) {  // check for empty dyn array
        DEBUG("error: passed ready q size 0 or below");
        return false;
    }
    if(DEBUG_BRANCH) {  // debug print info
        int count = 1;
        puts("Printing initial state");
        dyn_array_for_each(ready_queue, print_dyn_array, (void*)&count);
        puts("");
    }

    // initializing variables
    int sum = 0;
    dyn_array_for_each(ready_queue, sum_total_burst_time, (void*)&sum);
    dyn_array_sort(ready_queue, compare_arrival_time);
    Wait_struct waiter;  // I used waiter struct here to calculate wait times
    waiter.time = 0;
    waiter.total_wait = 0;
    float total_turnaround = 0;

    dyn_array_t * wait_queue = dyn_array_create(dyn_array_size(ready_queue), sizeof(ProcessControlBlock_t), NULL);
    if(wait_queue == NULL) {
        DEBUG("Error: malloc error at wait queue");
        return false;
    }

    if(DEBUG_BRANCH) {  // debug print info
        int count = 1;
        puts("Printing post sort state");
        dyn_array_for_each(ready_queue, print_dyn_array, (void*)&count);
        puts("");
    }

    transfer_to_wait_queue_arrival(ready_queue, wait_queue, waiter.time, false);

    
    do {  
        if(DEBUG_BRANCH) {
            int count = 1;
            puts("Status: printing wait queue");
            dyn_array_for_each(wait_queue, print_dyn_array, (void*)&count);
        }
    
        if(!dyn_array_empty(wait_queue)) {
            ProcessControlBlock_t current; 
            size_t i = 0;
            dyn_array_extract_front(wait_queue, (void * const)&current);
            for(size_t j = 0; j <quantum; ++j) {  // run burst time
                current.started = true;
                virtual_cpu(&current);
                i++;
                if(current.remaining_burst_time == 0) {
                    break;
                }         
            }
            waiter.time += (float)i;
            waiter.interval = (float)i;
            if(!dyn_array_empty(ready_queue)) {  // transfer new arrivals
                transfer_to_wait_queue_arrival(ready_queue, wait_queue, waiter.time, true);  
            }
            if(current.remaining_burst_time) {
                dyn_array_for_each(wait_queue, calculate_interupted_wait, &waiter);  // wait for each pcb in queue is calculated here
                dyn_array_push_back(wait_queue, (void * const)&current);
            } else {
                total_turnaround = total_turnaround + waiter.time - current.arrival; 
                dyn_array_for_each(wait_queue, calculate_interupted_wait, &waiter); 
            }
        } else {  // for waiting for processes to arrive
            waiter.time++;
            if(!dyn_array_empty(ready_queue)) {
                transfer_to_wait_queue_arrival(ready_queue, wait_queue, waiter.time, true);  //
            }
        }
        if(DEBUG_BRANCH) {
            printf("Round %d  Time %f total_turnaround %f total wait %f\n", 
            counter, waiter.time, total_turnaround, waiter.total_wait);
            counter++;
        }
    } while(!(dyn_array_empty(ready_queue) && dyn_array_empty(wait_queue)));

    // sending metrics to result
    result->average_waiting_time = waiter.total_wait / size;
    result->average_turnaround_time = total_turnaround / size;
    result->total_run_time = sum;

    dyn_array_destroy(wait_queue);

    if(DEBUG_BRANCH) {  // more debug stuff
        printf("Total runtime = %d\n", sum);
        printf("Size of dyn array = %d\n", size);
        int a = 1;
        dyn_array_for_each(ready_queue, print_dyn_array, (void*)&a);
        printf("Avg wait = %f\n", result->average_waiting_time);
        printf("Avg turnaround = %f\n", result->average_turnaround_time);
    }

    return true; 
}

/**
    This function takes a input binary file, reads process control block info
    and writes it to a dyn array struct
    sys/stat.h is added to use fstat function
    extensive error checking is used, returns null on failure
*/
dyn_array_t *load_process_control_blocks(const char *input_file) //dont forget close file, free buffer
{
    if(input_file == NULL) {
        DEBUG("error: passed null ptr");
        return NULL;  // check params
    }
    int fd = open(input_file, O_RDONLY);
    if(fd == -1) {
        DEBUG("error: file open failure");
        return NULL;  // check file exists
    }
    struct stat file_info;
    if(fstat(fd, &file_info) == -1) {
        DEBUG("error: fstat failure");
        return NULL;  // check for fstat failure
    }
    int object_count;
    if((int)read(fd, (void*)&object_count, sizeof(uint32_t)) < (int)sizeof(uint32_t)) {
        DEBUG("error: read size failure");
        return NULL;  // read size
    }
    //printf("size = %d\n", object_count);
    int byte_count = object_count * (sizeof(uint32_t) * 3);  // get size in bytes
    if(byte_count + (int)sizeof(uint32_t) != file_info.st_size) { // compare byte count to file count
        DEBUG("error: calculated byte_count != file byte_count");
        //printf("calc byte_count = %d\nfile byte_count = %d\n", (int)byte_count + (int)sizeof(uint32_t), (int)file_info.st_size);
        return NULL;  
    }
    uint32_t * burst_array = malloc(byte_count);  // create buffer for burst times
    if(burst_array == NULL) {
        DEBUG("error: burst array malloc failure");
        return NULL;
    }
    if((int)read(fd, (void*)burst_array, byte_count) < byte_count) {
        DEBUG("error: read to burst array failure");
        return NULL;  // read bursts to buffer
    }
    close(fd);  // close file
    DEBUG("entering dyn array pcb push");  
    dyn_array_t * darray = dyn_array_create(object_count, sizeof(ProcessControlBlock_t),NULL);  // create dyn array
    if(darray == NULL) {
        DEBUG("error: darray malloc failure");
        return NULL;
    }
    for(int i = 0; i < (int)object_count; ++i) {  // push burst times into dyn array
        ProcessControlBlock_t hold;  // local process control block to be copied into dyn arraty
        //printf("%d\n", *(burst_array + i * 3));
        hold.remaining_burst_time = *(burst_array + i * 3);
        hold.priority = *(burst_array + i * 3 + 1);
        hold.arrival = *(burst_array + i * 3 + 2);
        hold.started = false;
        dyn_array_push_back(darray, (const void * const)&hold);  // push hold onto dyn array
    } 
    free(burst_array);  // free buffer
    return darray;
}

// This function checks for a new arrival every tick
// when a new arrival comes the burst is interrupted
// the queue is resorted for shortest job
// then shortest job is ran
bool shortest_remaining_time_first(dyn_array_t *ready_queue, ScheduleResult_t *result) 
{
    static int counter = 1;  // this is a debug variable only
    if(ready_queue == NULL || result == NULL) {  // check for bad params
        DEBUG("error: passed invalid param");
        return false;
    }
    int size = dyn_array_size(ready_queue);
    if(size <= 0) {  // check for empty dyn array
        DEBUG("error: passed ready q size 0 or below");
        return false;
    }
    if(DEBUG_BRANCH) {  // debug print info
        int count = 1;
        puts("Printing initial state");
        dyn_array_for_each(ready_queue, print_dyn_array, (void*)&count);
        puts("");
    }

    // initializing variables
    int sum = 0;
    dyn_array_for_each(ready_queue, sum_total_burst_time, (void*)&sum);
    dyn_array_sort(ready_queue, compare_arrival_time);
    Wait_struct waiter; // waiter here
    waiter.time = 0;
    waiter.total_wait = 0;
    float total_turnaround = 0;

    dyn_array_t * wait_queue = dyn_array_create(dyn_array_size(ready_queue), sizeof(ProcessControlBlock_t), NULL);
    if(wait_queue == NULL) {
        DEBUG("Error: malloc error at wait queue");
        return false;
    }

    if(DEBUG_BRANCH) {  // debug print info
        int count = 1;
        puts("Printing post sort state");
        dyn_array_for_each(ready_queue, print_dyn_array, (void*)&count);
        puts("");
    }

    transfer_to_wait_queue_arrival(ready_queue, wait_queue, waiter.time, false); // initial transfer

    do { 
        if(DEBUG_BRANCH) {
            int count = 1;
            puts("Status: printing wait queue");
            dyn_array_for_each(wait_queue, print_dyn_array, (void*)&count);
        }

        dyn_array_sort(wait_queue, compare_remaining_burst_time);  // sort
    
        if(!dyn_array_empty(wait_queue)) {
            ProcessControlBlock_t current; 
            waiter.interval = 0;
            dyn_array_extract_front(wait_queue, (void * const)&current);
            while(true) {  // run burst
                current.started = true;
                virtual_cpu(&current);
                waiter.time++;
                waiter.interval++;
                if(DEBUG_BRANCH) {
                    int count = 1;
                    printf("Round %d  Time %f total_turnaround %f total wait %f\n", 
                    counter, waiter.time, total_turnaround, waiter.total_wait);
                    printf("ARRIVAL: %d  BURST_R: %d\n", current.arrival, current.remaining_burst_time);
                    counter++;
                    puts("WAIT QUEUE:");
                    dyn_array_for_each(wait_queue, print_dyn_array, (void*)&count);
                }
                if(!dyn_array_empty(ready_queue)) {
                    if(transfer_to_wait_queue_arrival(ready_queue, wait_queue, waiter.time, true)) {
                        break;  // interupt if new arrival
                    }
                }
                if(current.remaining_burst_time == 0) break;  // interupt if job done
            }        
            if(current.remaining_burst_time) {
                dyn_array_for_each(wait_queue, calculate_interupted_wait, &waiter);  // calc wait times for all in queue
                dyn_array_push_front(wait_queue, (void * const)&current);
            } else {
                total_turnaround = total_turnaround + waiter.time - current.arrival; 
                dyn_array_for_each(wait_queue, calculate_interupted_wait, &waiter);  
            }
        } else {  // for waiting for processes to arrive
            waiter.time++;
            if(!dyn_array_empty(ready_queue)) {
                transfer_to_wait_queue_arrival(ready_queue, wait_queue, waiter.time, true);  
            }
        }
        
    } while(!(dyn_array_empty(ready_queue) && dyn_array_empty(wait_queue)));

    // sending metrics to result
    result->average_waiting_time = waiter.total_wait / size;
    result->average_turnaround_time = total_turnaround / size;
    result->total_run_time = sum;

    dyn_array_destroy(wait_queue);

    if(DEBUG_BRANCH) {  // more debug stuff
        printf("Total runtime = %d\n", sum);
        printf("Size of dyn array = %d\n", size);
        int a = 1;
        dyn_array_for_each(ready_queue, print_dyn_array, (void*)&a);
        printf("Avg wait = %f\n", result->average_waiting_time);
        printf("Avg turnaround = %f\n", result->average_turnaround_time);
    }

    return true; 
}
