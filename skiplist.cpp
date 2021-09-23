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
#include <queue>
#include <vector>
#include <cstring>

// #include "skiplist2.h"
#include "skiplist.h"

using namespace std;
pthread_mutex_t queue_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t thread_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t finish_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t thread_cond = PTHREAD_COND_INITIALIZER;

pthread_barrier_t barrier;

pthread_cond_t main_cond = PTHREAD_COND_INITIALIZER;
pthread_mutex_t main_mutex = PTHREAD_MUTEX_INITIALIZER;

pthread_cond_t skip_cond = PTHREAD_COND_INITIALIZER;
pthread_mutex_t skip_lock= PTHREAD_MUTEX_INITIALIZER;
// task queue
queue <pair<char, int> > task_queue;
// skiplist
skiplist<int, int> list(0,1000000);
bool finished = false;
void* workerThread(void* index){
    pair<char, int> task;
    int idx = (int)(*(int*)index); 

    while(1){

        pthread_mutex_lock(&queue_lock);
        int size = task_queue.size();
        pthread_mutex_unlock(&queue_lock);
        
        if (size >0){
            pthread_mutex_lock(&queue_lock);
            task = task_queue.front();
            task_queue.pop();
            pthread_mutex_unlock(&queue_lock);
            char action = task.first;
            int num = task.second;
            // 일단은 global lock
            pthread_mutex_lock(&skip_lock);
            if (action == 'i') {            // insert
                list.insert(num,num);
            }
            else if (action == 'q') {      // qeury
                if(list.find(num)!=num)
		            cout << "ERROR: Not Found: " << num << endl;
            } 
            else if (action == 'w') {     // wait
                usleep(num);
            }
            else {
                printf("worker thread ERROR: Unrecognized action: '%c'\n", action);
                exit(EXIT_FAILURE);
            }
            pthread_mutex_unlock(&skip_lock);
        }
        else if(size<=0){ // if task queue is empty

            if (finished){ // 모든 과정이 끝나면 
                return NULL;
            }   

            pthread_barrier_wait(&barrier);
    
            pthread_cond_signal(&main_cond);

            pthread_mutex_lock(&thread_mutex);
            pthread_cond_wait(&thread_cond, &thread_mutex);
            pthread_mutex_unlock(&thread_mutex);
        }
    }
    return NULL;
}

int main(int argc, char* argv[])
{
    int count=0;
    struct timespec start, stop;
    
	// check and parse command line options
    if (argc < 3) {
        printf("Usage: %s <infile> <number_of_threads>\n", argv[0]);
        exit(EXIT_FAILURE);
    }
	// 첫번째 인자: 입력 파일, 두번째 인자: 스레드 수
    char *fn = argv[1];
    string s(argv[2]);
	int num_threads= stoi(s);

    clock_gettime( CLOCK_REALTIME, &start);
    
	// create Threads
	// 스레드풀 만들고, 완전히 초기화 될때까지 기다리기. 처음에는 스레드들이 idle하다
    vector <pthread_t> tid; // array of threads
    tid.reserve (num_threads);
    // barrier
    pthread_barrier_init(&barrier, NULL, num_threads);

    for (int i = 0; i < num_threads; i++){
        int *idx =  (int*)malloc(sizeof(int)*4);
        *idx = i;
            if ( pthread_create(&tid[i], NULL, workerThread, (void*)(idx)) < 0){
                perror("thread Create error : ");
                exit(0);
            }
    }
    // 초기화 끝날때까지 기다리기
    pthread_mutex_lock(&main_mutex);
    pthread_cond_wait(&main_cond, &main_mutex);
    pthread_mutex_unlock(&main_mutex);

	// load input file
    FILE* fin = fopen(fn, "r");
    char action;
    long num;

    while (fscanf(fin, "%c %ld\n", &action, &num) > 0) {
        if (action == 'p') {     // wait
            pthread_cond_broadcast(&thread_cond);
            pthread_mutex_lock(&main_mutex);
            pthread_cond_wait(&main_cond, &main_mutex);
            pthread_mutex_unlock(&main_mutex);
	        cout << list.printList() << endl;
        } 
        else if ( action == 'i' || action == 'q' || action == 'w'){
            // task queue에 넣고, idle 워커를 깨워 할당하기
            pthread_mutex_lock(&queue_lock);
            task_queue.push(make_pair(action, num));
            pthread_mutex_unlock(&queue_lock);
            // 백그라운드 스레드 하나 깨우기
            pthread_cond_signal(&thread_cond);
        }
        else {
            printf("main thread ERROR: Unrecognized action: '%c'\n", action);
            exit(EXIT_FAILURE);
        }
	    count++;
    }
    fclose(fin);
    clock_gettime(CLOCK_REALTIME, &stop);
    // clean up
    finished = true;
    pthread_cond_broadcast(&thread_cond);
    for(int i=0;i<num_threads;i++)
        pthread_join(tid[i], NULL);
    // pthread_barrier_destroy(&barrier);
    // pthread_mutex_destroy(&queue_lock);
    // pthread_mutex_destroy(&thread_mutex);
    // pthread_mutex_destroy(&main_mutex);
    // pthread_mutex_destroy(&skip_lock);
    // pthread_cond_destroy(&thread_cond);
    // pthread_cond_destroy(&main_cond);
    // pthread_cond_destroy(&skip_cond);
    double elapsed_time = (stop.tv_sec - start.tv_sec) + ((double) (stop.tv_nsec - start.tv_nsec))/BILLION ;

    cout << "Elapsed time: " << elapsed_time << " sec" << endl;
    cout << "Throughput: " << (double) count / elapsed_time << " ops (operations/sec)" << endl;
    return (EXIT_SUCCESS);
}

