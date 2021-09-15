/*
 * main.cpp
 *
 * Serial version
 *
 * Compile with -O2
 */

#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <iostream>
#include <pthread.h>

//#include "skiplist2.h"
#include "skiplist.h"

using namespace std;

void *init_thread(void *param){
	return NULL ;
}
int main(int argc, char* argv[])
{
    int count=0;
    struct timespec start, stop;

    skiplist<int, int> list(0,1000000);
    
	// check and parse command line options
    if (argc < 3) {
        printf("Usage: %s <infile> <number_of_threads>\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    char *fn = argv[1];
	int num_threads= *argv[2] - '0';
	// 첫번째 인자: 입력 파일, 두번째 인자: 스레드 수

    clock_gettime( CLOCK_REALTIME, &start);
    
	// create Threads
	pthread_t tid[num_threads]; // array of thread IDs
	int err;
	for( int i=0;i<num_threads;i++)
		err = pthread_create(&tid[i], NULL, init_thread, NULL);
	cout<<"Threads Created"<<endl;
	
	// load input file
    FILE* fin = fopen(fn, "r");
    char action;
    long num;
    while (fscanf(fin, "%c %ld\n", &action, &num) > 0) {
        if (action == 'i') {            // insert
            list.insert(num,num);
        }else if (action == 'q') {      // qeury
            if(list.find(num)!=num)
		cout << "ERROR: Not Found: " << num << endl;
        } else if (action == 'w') {     // wait
            usleep(num);
        } else if (action == 'p') {     // wait
	    cout << list.printList() << endl;
        } else {
            printf("ERROR: Unrecognized action: '%c'\n", action);
            exit(EXIT_FAILURE);
        }
	count++;
    }
    fclose(fin);
    clock_gettime( CLOCK_REALTIME, &stop);

    // print results
    double elapsed_time = (stop.tv_sec - start.tv_sec) + ((double) (stop.tv_nsec - start.tv_nsec))/BILLION ;

    cout << "Elapsed time: " << elapsed_time << " sec" << endl;
    cout << "Throughput: " << (double) count / elapsed_time << " ops (operations/sec)" << endl;

    return (EXIT_SUCCESS);
}

