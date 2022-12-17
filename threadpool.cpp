//
// Created by l on 12/16/22.
//

#include "threadpool.h"

const int NUMBER=2;

ThreadPool* threadPoolCreate(int min,int max,int queueCapacity)
{
    ThreadPool* pool=new ThreadPool;
    do {
        pool->threadIDs=new pthread_t[max];
        if(pool->threadIDs==NULL)
        {
            cout<<"malloc threadIDs fail..."<<endl;
            break;
        }
        memset(pool->threadIDs,0,sizeof(pthread_t)*max);
        pool->minNum=min;
        pool->maxNum=max;
        pool->busyNum=0;
        pool->liveNum=min;
        pool->exitNum=0;

        if(pthread_mutex_init(&pool->mutexPool,NULL)!=0||
                pthread_mutex_init(&pool->mutexBusy,NULL)!=0||
                pthread_cond_init(&pool->notFull,NULL)!=0||
                pthread_cond_init(&pool->notEmpty,NULL)!=0)
        {
            cout<<"mutex or condition init fail..."<<endl;
            break;
        }


        pool->taskQ=new Task[queueCapacity];
        pool->queueCapacity=queueCapacity;
        pool->queueSize=0;
        pool->queueFront=0;
        pool->queueRear=0;

        pool->shutdown=0;

        pthread_create(&pool->managerID,NULL,manager,pool);
        for(int i=0;i<min;i++)
        {
            pthread_create(&pool->threadIDs[i],NULL,worker,pool);
        }
        return pool;
    }while(0);
    if(pool&&pool->threadIDs)delete pool->threadIDs;
    if(pool&&pool->taskQ)delete pool->taskQ;
    if(pool) delete pool;
    return NULL;
}



void* worker(void* arg)
{
    ThreadPool* pool=(ThreadPool*)arg;
    while(1)
    {
        pthread_mutex_lock(&pool->mutexPool);
        while(pool->queueSize==0&&!pool->shutdown)
        {
            pthread_cond_wait(&pool->notEmpty,&pool->mutexPool);

            if(pool->exitNum>0)
            {
                pool->exitNum--;
                if(pool->liveNum>pool->minNum)
                {
                    pool->liveNum--;
                    pthread_mutex_unlock(&pool->mutexPool);
                    threadExit(pool);
                }
            }
        }

        if(pool->shutdown)
        {
            pthread_mutex_unlock(&pool->mutexPool);
            threadExit(pool);
        }

        Task task;
        task.function=pool->taskQ[pool->queueFront].function;
        task.arg=pool->taskQ[pool->queueFront].arg;

        pool->queueFront=(pool->queueFront+1)%pool->queueCapacity;
        pool->queueSize--;

        pthread_cond_signal(&pool->notFull);
        pthread_mutex_unlock(&pool->mutexPool);

        cout<<"thread "<<pthread_self()<<" start working..."<<endl;
        pthread_mutex_lock(&pool->mutexBusy);
        pool->busyNum++;
        pthread_mutex_unlock(&pool->mutexBusy);
        task.function(task.arg);
        delete task.arg;
        task.arg=NULL;

        cout<<"thread "<<pthread_self()<<" end working..."<<endl;
        pthread_mutex_lock(&pool->mutexBusy);
        pool->busyNum--;
        pthread_mutex_unlock(&pool->mutexBusy);

    }
}



void * manager(void * arg)
{
    ThreadPool* pool=(ThreadPool*)arg;

    while(!pool->shutdown)
    {
        sleep(3);

        pthread_mutex_lock(&pool->mutexPool);
        int queueSize=pool->queueSize;
        int liveNum=pool->liveNum;
        int busyNum=pool->busyNum;
        pthread_mutex_unlock(&pool->mutexPool);
        if(queueSize>liveNum&&liveNum<pool->maxNum)
        {
            pthread_mutex_lock(&pool->mutexPool);
            int counter=0;
            for(int i=0;i<pool->maxNum&&counter<NUMBER&&pool->liveNum<pool->maxNum;i++)
            {
                if(pool->threadIDs[i]==0)
                {
                    pthread_create(&pool->threadIDs[i],NULL,worker,pool);
                    counter++;
                    pool->liveNum++;
                }
            }
            pthread_mutex_unlock(&pool->mutexPool);
        }


        if(busyNum*2<liveNum&&liveNum>pool->minNum)
        {
            pthread_mutex_lock(&pool->mutexPool);
            pool->exitNum=NUMBER;
            pthread_mutex_unlock(&pool->mutexPool);
            for(int i=0;i<NUMBER;i++)
            {
                pthread_cond_signal(&pool->notEmpty);
            }
        }
    }
    return NULL;
}


void threadExit(ThreadPool* pool)
{
    pthread_t tid=pthread_self();
    for(int i=0;i<pool->maxNum;i++)
    {
        if(pool->threadIDs[i]==tid)
        {
            pool->threadIDs[i]=0;
            cout<<"threadExit() called, "<<pthread_self()<<" exiting..."<<endl;
            break;
        }
    }
    pthread_exit(NULL);
}

void threadPoolAdd(ThreadPool* pool,void(*func)(void *),void* arg)
{
    pthread_mutex_lock(&pool->mutexPool);
    while(pool->queueSize==pool->queueCapacity&&!pool->shutdown)
    {
        pthread_cond_wait(&pool->notFull,&pool->mutexPool);
    }

    if(pool->shutdown)
    {
        pthread_mutex_unlock(&pool->mutexPool);
        return;
    }

    pool->taskQ[pool->queueRear].function=func;
    pool->taskQ[pool->queueRear].arg=arg;
    pool->queueRear=(pool->queueRear+1)%pool->queueCapacity;
    pool->queueSize++;

    pthread_cond_signal(&pool->notEmpty);

    pthread_mutex_unlock(&pool->mutexPool);
}

int threadPoolBusyNum(ThreadPool* pool)
{
    pthread_mutex_lock(&pool->mutexPool);
    int busyNum=pool->busyNum;
    pthread_mutex_unlock(&pool->mutexPool);
    return busyNum;
}


int threadPoolAliveNum(ThreadPool* pool)
{
    pthread_mutex_lock(&pool->mutexPool);
    int liveNum=pool->liveNum;
    pthread_mutex_unlock(&pool->mutexPool);
    return liveNum;
}

int threadPoolDestroy(ThreadPool* pool)
{
    if(pool==NULL)
    {
        return -1;
    }

    pool->shutdown=1;
    pthread_join(pool->managerID,NULL);

    for(int i=0;i<pool->liveNum;i++)
    {
        pthread_cond_signal(&pool->notEmpty);
    }

    if(pool->taskQ)
    {
        delete pool->taskQ;
    }
    if(pool->threadIDs)
    {
        delete pool->threadIDs;
    }
    pthread_mutex_destroy(&pool->mutexBusy);
    pthread_mutex_destroy(&pool->mutexPool);
    pthread_cond_destroy(&pool->notFull);
    pthread_cond_destroy(&pool->notEmpty);
    delete pool;
    pool=NULL;
    return 0;
}