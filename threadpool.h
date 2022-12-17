//
// Created by l on 12/16/22.
//

#ifndef THREADPOOL2_THREADPOOL_H
#define THREADPOOL2_THREADPOOL_H

#include<pthread.h>
#include<iostream>
#include<cstring>
#include<stdlib.h>
#include<unistd.h>

using namespace std;

typedef struct task
{
    void(*function)(void*arg);//存函数地址（函数的返回值为空 函数传入参数类型为void×）
    void*arg;
}Task;

struct ThreadPool
{
    Task* taskQ;
    int queueCapacity;
    int queueSize;
    int queueFront;
    int queueRear;

    pthread_t managerID;
    pthread_t * threadIDs;
    int minNum;
    int maxNum;
    int busyNum;
    int liveNum;
    int exitNum;
    pthread_mutex_t mutexPool;
    pthread_mutex_t mutexBusy;
    pthread_cond_t  notFull;
    pthread_cond_t  notEmpty;

    int shutdown;
};

ThreadPool* threadPoolCreate(int min,int max,int queueSize);

void* worker(void* arg);

void* manager(void* arg);

void threadExit(ThreadPool* pool);

void threadPoolAdd(ThreadPool* pool,void(*func)(void*),void* arg);

int threadPoolBusyNum(ThreadPool* pool);

int threadPoolAliveNum(ThreadPool* pool);

int threadPoolDestroy(ThreadPool*pool);


#endif //THREADPOOL2_THREADPOOL_H
