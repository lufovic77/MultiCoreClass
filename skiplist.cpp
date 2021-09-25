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
pthread_mutex_t working_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t thread_cond = PTHREAD_COND_INITIALIZER;

pthread_barrier_t barrier;

pthread_cond_t main_cond = PTHREAD_COND_INITIALIZER;
pthread_mutex_t main_mutex = PTHREAD_MUTEX_INITIALIZER;

pthread_cond_t skip_cond = PTHREAD_COND_INITIALIZER;
pthread_mutex_t skip_lock= PTHREAD_MUTEX_INITIALIZER;
// task queue (queue in queue)
queue <queue<pair<char, int> >> task_queue;
// skiplist
unsigned int working = 0;
skiplist<int, int> list(0,1000000);
bool finished = false;
void* workerThread(void* index){
    pair<char, int> task;
    int idx = (int)(*(int*)index); 

    while(1){

        pthread_mutex_lock(&queue_lock);
        int size = task_queue.size();
        working | (1 << idx) ? 1 : 0;
        if (size >0){
            queue <pair<char, int> > tmp_q;
            tmp_q = task_queue.front();
            task_queue.pop();
            pthread_mutex_unlock(&queue_lock);
            while(!tmp_q.empty()){
                task = tmp_q.front();
                tmp_q.pop();
                char action = task.first;
                int num = task.second;
          //  fprintf(stderr, "%d, %c, %d\n", idx, action, num);
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
        }
        else if(size<=0){ // if task queue is empty
            pthread_mutex_unlock(&queue_lock);
            working | ~(1 << idx) ? 1 : 0;
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
    int batch_size = 3;
    queue <pair<char, int> > tmp_q;
    while (fscanf(fin, "%c %ld\n", &action, &num) > 0) {
        if (action == 'p') {     // wait
            if(!tmp_q.empty()){
                task_queue.push(tmp_q);
                count = 0;
                while(!tmp_q.empty())
                    tmp_q.pop();
            }
            pthread_cond_broadcast(&thread_cond);
            pthread_mutex_lock(&main_mutex);
            pthread_cond_wait(&main_cond, &main_mutex);
            pthread_mutex_unlock(&main_mutex);
	        cout << list.printList() << endl;
        } 
        else if ( action == 'i' || action == 'q' || action == 'w'){
            // task queue에 넣고, idle 워커를 깨워 할당하기
            if(count == batch_size){
                tmp_q.push(make_pair(action, num));
                count = 0;
                pthread_mutex_lock(&queue_lock);
                task_queue.push(tmp_q);
                pthread_mutex_unlock(&queue_lock);
                while(!tmp_q.empty())
                tmp_q.pop();
                // 백그라운드 스레드 하나 깨우기
                pthread_cond_signal(&thread_cond);
            }
            else{
                tmp_q.push(make_pair(action, num));
            }
        }
        else {
            printf("main thread ERROR: Unrecognized action: '%c'\n", action);
            exit(EXIT_FAILURE);
        }
	    count++;
    }

    if(!tmp_q.empty()){
        pthread_mutex_lock(&queue_lock);
            task_queue.push(tmp_q);
        pthread_mutex_unlock(&queue_lock);
    }
    fclose(fin);
    // clean up
    while(1){
        fprintf(stderr, "%d", working);
        if(working==0)
        break;
    }
    finished = true;
    pthread_cond_broadcast(&thread_cond);
    for(int i=0;i<num_threads;i++)
        pthread_join(tid[i], NULL);
    clock_gettime(CLOCK_REALTIME, &stop);
    double elapsed_time = (stop.tv_sec - start.tv_sec) + ((double) (stop.tv_nsec - start.tv_nsec))/BILLION ;

    cout << "Elapsed time: " << elapsed_time << " sec" << endl;
    cout << "Throughput: " << (double) count / elapsed_time << " ops (operations/sec)" << endl;
    return (EXIT_SUCCESS);
}

