#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "dyn_array.h"
#include "processing_scheduling.h"

#define FCFS "FCFS"
#define P "P"
#define RR "RR"
#define SJF "SJF"
#define SRTF "SRTF"
#define ALL "ALL"

int get_function_enum(char * string);

// Add and comment your analysis code in this function.
// THIS IS NOT FINISHED.
// 0 - executable
// 1 - file name
// 2 - algorithm
// 3 - quantum
int main(int argc, char **argv) 
{
    if (argc < 3) 
    {
        printf("%s <pcb file> <schedule algorithm> [quantum]\n", argv[0]);
        return EXIT_FAILURE;
    }
    dyn_array_t * ready_queue = NULL;
    dyn_array_t *ready_array[5] = {NULL, NULL, NULL, NULL, NULL}; // This is for printing all at same time
    if((ready_queue = load_process_control_blocks(argv[1])) == NULL) {
        puts("ERROR: Invalid input file");
        return EXIT_FAILURE;
    }
    if(!strcmp(argv[2], ALL)) {  // ALL command
        for(int i = 0; i < 5; ++i) {
            ready_array[i] = load_process_control_blocks(argv[1]);
        }
    }
    
    ScheduleResult_t result_data;
    ScheduleResult_t * result = &result_data;
    switch(get_function_enum(argv[2])) {  // Printing is done in a switch statement
        case 0:
            if(first_come_first_serve(ready_queue, result) == false) {
                puts("Error: scheduling error FCFS");
                return EXIT_FAILURE;
            } else {
                printf("\n%-30s%-15s\n%-30s%-10s\n", "Filename = ", argv[1], "Algorithm = ", argv[2]);
                printf("%-30s%-10ld\n%-30s%-10f\n%-30s%-10f\n", 
                "total_run_time = ", result->total_run_time,
                "average_waiting_time = ", result->average_waiting_time, 
                "average_turn_around_time = ", result->average_turnaround_time);
                break;
            }
        case 1:
            if(priority(ready_queue, result) == false) {
                puts("Error: scheduling error P");
                return EXIT_FAILURE;
            } else {
                printf("\n%-30s%-15s\n%-30s%-10s\n", "Filename = ", argv[1], "Algorithm = ", argv[2]);
                printf("%-30s%-10ld\n%-30s%-10f\n%-30s%-10f\n", 
                "total_run_time = ", result->total_run_time,
                "average_waiting_time = ", result->average_waiting_time, 
                "average_turn_around_time = ", result->average_turnaround_time);
                break;
            }
        case 2:
            if(argc < 4) {
                puts("Error: did not include time quantum for RR");
                return EXIT_FAILURE;
            }
            else if(round_robin(ready_queue, result, atoi(argv[3])) == false) {
                puts("Error: scheduling error RR");
                return EXIT_FAILURE;
            } else {
                printf("\n%-30s%-15s\n%-30s%-10s\n", "Filename = ", argv[1], "Algorithm = ", argv[2]);
                printf("%-30s%-10ld\n%-30s%-10f\n%-30s%-10f\n", 
                "total_run_time = ", result->total_run_time,
                "average_waiting_time = ", result->average_waiting_time, 
                "average_turn_around_time = ", result->average_turnaround_time);
                break;
            }
        case 3:
            if(shortest_job_first(ready_queue, result) == false) {
                puts("Error: scheduling error SJF");
                return EXIT_FAILURE;
            } else {
                printf("\n%-30s%-15s\n%-30s%-10s\n", "Filename = ", argv[1], "Algorithm = ", argv[2]);
                printf("%-30s%-10ld\n%-30s%-10f\n%-30s%-10f\n", 
                "total_run_time = ", result->total_run_time,
                "average_waiting_time = ", result->average_waiting_time, 
                "average_turn_around_time = ", result->average_turnaround_time);
                break;
            }
        case 4:
            if(shortest_remaining_time_first(ready_queue, result) == false) {
                puts("Error: scheduling error SRJF");
                return EXIT_FAILURE;
            } else {
                printf("\n%-30s%-15s\n%-30s%-10s\n", "Filename = ", argv[1], "Algorithm = ", argv[2]);
                printf("%-30s%-10ld\n%-30s%-10f\n%-30s%-10f\n", 
                "total_run_time = ", result->total_run_time,
                "average_waiting_time = ", result->average_waiting_time, 
                "average_turn_around_time = ", result->average_turnaround_time);
                break;
            }
        case 5:  // all case
            if(first_come_first_serve(ready_array[0], result) == false) {
                puts("Error: scheduling error FCFS");
            } else {
                printf("\n%-30s%-15s\n%-30s%-10s\n", "Filename = ", FCFS, "Algorithm = ", argv[2]);
                printf("%-30s%-10ld\n%-30s%-10f\n%-30s%-10f\n", 
                "total_run_time = ", result->total_run_time,
                "average_waiting_time = ", result->average_waiting_time, 
                "average_turn_around_time = ", result->average_turnaround_time);
            }
            if(priority(ready_array[1], result) == false) {
                puts("Error: scheduling error P");
            } else {
                printf("\n%-30s%-15s\n%-30s%-10s\n", "Filename = ", P, "Algorithm = ", argv[2]);
                printf("%-30s%-10ld\n%-30s%-10f\n%-30s%-10f\n", 
                "total_run_time = ", result->total_run_time,
                "average_waiting_time = ", result->average_waiting_time, 
                "average_turn_around_time = ", result->average_turnaround_time);
            }
            if(argc < 4) {
                puts("Error: did not include time quantum for RR");
            } else if(round_robin(ready_array[2], result, atoi(argv[3])) == false) {
                puts("Error: scheduling error RR");
            } else {
                printf("\n%-30s%-15s\n%-30s%-10s\n", "Filename = ", RR, "Algorithm = ", argv[2]);
                printf("%-30s%-10ld\n%-30s%-10f\n%-30s%-10f\n", 
                "total_run_time = ", result->total_run_time,
                "average_waiting_time = ", result->average_waiting_time, 
                "average_turn_around_time = ", result->average_turnaround_time);
            }
            if(shortest_job_first(ready_array[3], result) == false) {
                puts("Error: scheduling error SJF");
            } else {
                printf("\n%-30s%-15s\n%-30s%-10s\n", "Filename = ", SJF, "Algorithm = ", argv[2]);
                printf("%-30s%-10ld\n%-30s%-10f\n%-30s%-10f\n", 
                "total_run_time = ", result->total_run_time,
                "average_waiting_time = ", result->average_waiting_time, 
                "average_turn_around_time = ", result->average_turnaround_time);
            }
            if(shortest_remaining_time_first(ready_array[4], result) == false) {
                puts("Error: scheduling error SRJF");
            } else {
                printf("\n%-30s%-15s\n%-30s%-10s\n", "Filename = ", SRTF, "Algorithm = ", argv[2]);
                printf("%-30s%-10ld\n%-30s%-10f\n%-30s%-10f\n", 
                "total_run_time = ", result->total_run_time,
                "average_waiting_time = ", result->average_waiting_time, 
                "average_turn_around_time = ", result->average_turnaround_time);
            }
            break;

        default:
            puts("Error: scheduling algorithm not found");
            return EXIT_FAILURE;
    }  // reformat output

    if(!strcmp(argv[2], ALL)) {
        for(int i = 0; i < 5; ++i) {
            dyn_array_destroy(ready_array[i]);
        }
    }
    dyn_array_destroy(ready_queue);
    return EXIT_SUCCESS;
}

int get_function_enum(char * string) { // clunky function to help with switch statement
    if(!strcmp(string, FCFS)) return 0;
    else if(!strcmp(string, P)) return 1;
    else if(!strcmp(string, RR)) return 2;
    else if(!strcmp(string, SJF)) return 3;
    else if(!strcmp(string, SRTF)) return 4;
    else if(!strcmp(string, ALL)) return 5;
    else return -1;
}
