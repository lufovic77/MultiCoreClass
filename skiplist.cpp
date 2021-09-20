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

//#include "skiplist2.h"
#include "skiplist.h"

using namespace std;

pthread_mutex_t thread_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t queue_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t thread_cond = PTHREAD_COND_INITIALIZER;

pthread_cond_t async_cond = PTHREAD_COND_INITIALIZER;
pthread_mutex_t mutex_lock= PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t async_mutex = PTHREAD_MUTEX_INITIALIZER;

// task queue
queue <pair<char, int> > task_queue;
// skiplist
skiplist<int, int> list(0,1000000);

class ThreadPool{
    private:
        int num_threads;
        vector <pthread_t> tid; // array of threads
    public:
        ThreadPool(int number){
            num_threads = number;
            tid.reserve (number);
        }
        void threadCreate(int index){
            if ( pthread_create(&tid[index], NULL, workerThread, NULL) < 0){
                perror("thread Create error : ");
                exit(0);
            }
        }
        static void *workerThread(void* param);
        
};

void* ThreadPool::workerThread(void* parma){
    pair<char, int > task;
    while(1){
        pthread_mutex_lock(&thread_mutex);
        pthread_cond_wait(&thread_cond, &thread_mutex);
        if(task_queue.empty()){ // if task queue is empty
            pthread_mutex_lock(&thread_mutex);
            pthread_cond_wait(&thread_cond, &thread_mutex);
        }
        else{
            pthread_mutex_lock(&queue_lock);
            task = task_queue.front();
            char action = task.first;
            int num = task.second;
            task_queue.pop();
            pthread_mutex_unlock(&queue_lock);
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
                printf("ERROR: Unrecognized action: '%c'\n", action);
                exit(EXIT_FAILURE);
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
    char *fn = argv[1];
	int num_threads= *argv[2] - '0';
	// 첫번째 인자: 입력 파일, 두번째 인자: 스레드 수

    clock_gettime( CLOCK_REALTIME, &start);
    
	// create Threads
	// 스레드풀 만들고, 완전히 초기화 될때까지 기다리기. 처음에는 스레드들이 idle하다
    ThreadPool threadPool(num_threads);
    for (int i = 0; i < num_threads; i++){
        threadPool.threadCreate(i);
    }
    cout<<"Threads Created"<<endl;


	// load input file
    FILE* fin = fopen(fn, "r");
    char action;
    long num;

    while (fscanf(fin, "%c %ld\n", &action, &num) > 0) {
        if (action == 'p') {     // wait
	        cout << list.printList() << endl;
        } 
        else if ( action == 'i' || action == 'q' || action == 'w'){
            // task queue에 넣고, idle 워커를 깨워 할당하기
            // task queue를 관리함에 있어 lock을 걸어야 할까 ..?
            pthread_mutex_lock(&queue_lock);
            task_queue.push(make_pair(action, num));
            pthread_mutex_unlock(&queue_lock);
            // 백그라운드 스레드 하나 깨우기
            pthread_cond_signal(&thread_cond);
        }
        
        else {
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

