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

pthread_mutex_t signal_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t working_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t thread_cond = PTHREAD_COND_INITIALIZER;

pthread_barrier_t barrier;

pthread_cond_t main_cond = PTHREAD_COND_INITIALIZER;
pthread_mutex_t main_mutex = PTHREAD_MUTEX_INITIALIZER;

pthread_cond_t skip_cond = PTHREAD_COND_INITIALIZER;
pthread_mutex_t skip_lock= PTHREAD_MUTEX_INITIALIZER;
pthread_rwlock_t finish_lock = PTHREAD_RWLOCK_INITIALIZER;

pthread_cond_t async_cond = PTHREAD_COND_INITIALIZER;
pthread_mutex_t async_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t *mycond;
// task queue (queue in queue)
queue <queue<pair<char, int> >> task_queue;
// skiplist
unsigned int working = 0;
skiplist<int, int> list(0,1000000);
bool finished = false;
bool stopping = false;
bool exhausted = false;
bool ready = false;
bool wake = false;
void* workerThread(void* index){
    pair<char, int> task;
    int idx = (int)(*(int*)index); 

    pthread_barrier_wait(&barrier);
    pthread_mutex_lock(&main_mutex);    
    ready = true;
    pthread_cond_signal(&main_cond);
    pthread_mutex_unlock(&main_mutex);
    while(1){
        pthread_mutex_lock(&thread_mutex);
        while(task_queue.empty()  ){
            pthread_cond_wait(&thread_cond, &thread_mutex);
        }
        working |= (1UL << idx) ;    
        queue <pair<char, int> > tmp_q;
        if(!task_queue.empty()){
            tmp_q = task_queue.front();
            task_queue.pop();
        }
        int qsize = tmp_q.size();
        if (finished){ // 모든 과정이 끝나면 
            pthread_cond_broadcast(&thread_cond);
            pthread_mutex_unlock(&thread_mutex); 
            return NULL;
        }   
        bool tmp = stopping;
        pthread_mutex_unlock(&thread_mutex); 

        if(qsize>0){
            while(!tmp_q.empty()){
                task = tmp_q.front();
                tmp_q.pop();
                char action = task.first;
                int num = task.second;
                fprintf(stderr, "%c, %d\n", action, num);
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
            }
        }

          if(tmp){//print  
            pthread_barrier_wait(&barrier);
            if(idx == 0){

            pthread_cond_signal(&main_cond);
            }
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
    mycond = (pthread_cond_t *)malloc(sizeof(pthread_cond_t)*num_threads);

    // barrier
    pthread_barrier_init(&barrier, NULL, num_threads);
    pthread_rwlock_init(&finish_lock, NULL);
    pthread_attr_t attr; 
    if( pthread_attr_init(&attr) != 0 ) 
    return 1; 
    if( pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED) != 0 ) 
    return -1; 

    for (int i = 0; i < num_threads; i++){
        int *idx =  (int*)malloc(sizeof(int)*4);
        *idx = i;
            if ( pthread_create(&tid[i], NULL, workerThread, (void*)(idx)) < 0){
                perror("thread Create error : ");
                exit(0);
            }
    }
    if( pthread_attr_destroy(&attr) != 0 ) return -1;
    // 초기화 끝날때까지 기다리기
    pthread_mutex_lock(&main_mutex);
    while(!ready)
        pthread_cond_wait(&main_cond, &main_mutex);
    stopping = false;
    ready = false;
    pthread_mutex_unlock(&main_mutex);
	// load input file
    FILE* fin = fopen(fn, "r");
    char action;
    long num;
    int batch_size = 5;
    int cur = 0, thr = 0;
    queue <pair<char, int> > q;
    while (fscanf(fin, "%c %ld\n", &action, &num) > 0) {
        if (action == 'p') {     // wait
            pthread_mutex_lock(&thread_mutex);
            if(!q.empty()){
                task_queue.push(q);
                cur = 0;
                while(!q.empty())
                    q.pop();
            }
            stopping = true;
            pthread_cond_broadcast(&thread_cond);
            pthread_mutex_unlock(&thread_mutex);
            
            pthread_mutex_lock(&main_mutex);
            pthread_cond_wait(&main_cond, &main_mutex);
            stopping = false;
            wake = false;
	        cout << list.printList() << endl;
            pthread_mutex_unlock(&main_mutex);
        } 
        else if ( action == 'i' || action == 'q' || action == 'w'){
            // task queue에 넣고, idle 워커를 깨워 할당하기
            if(cur >= batch_size){
                // 백그라운드 스레드 하나 깨우기
                pthread_mutex_lock(&thread_mutex);
                q.push(make_pair(action, num));
                cur = 0;
                task_queue.push(q);
                while(!q.empty())
                    q.pop();
                pthread_cond_signal(&thread_cond);
                pthread_mutex_unlock(&thread_mutex);
            }
            else{
                q.push(make_pair(action, num));
                cur++;
            }
        }
        else {
            printf("main thread ERROR: Unrecognized action: '%c'\n", action);
            exit(EXIT_FAILURE);
        }
	    count++;
    }
    if(!q.empty()){
        pthread_mutex_lock(&thread_mutex);
        task_queue.push(q);
        pthread_cond_signal(&thread_cond);
        pthread_mutex_unlock(&thread_mutex);
    }
    fprintf(stderr, "?");
    while(!task_queue.empty()){
    }
    fprintf(stderr, "?");
    finished = true;
    pthread_cond_broadcast(&thread_cond);
    for(int i=0;i<num_threads;i++)
        pthread_join(tid[i], NULL);
    fclose(fin);
    clock_gettime(CLOCK_REALTIME, &stop);
    double elapsed_time = (stop.tv_sec - start.tv_sec) + ((double) (stop.tv_nsec - start.tv_nsec))/BILLION ;

    cout << "Elapsed time: " << elapsed_time << " sec" << endl;
    cout << "Throughput: " << (double) count / elapsed_time << " ops (operations/sec)" << endl;
    return (EXIT_SUCCESS);
}

