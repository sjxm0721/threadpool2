#include<iostream>
#include<pthread.h>
#include<unistd.h>
#include<stdlib.h>
#include"threadpool.h"

using namespace std;

void taskFunc(void* arg)
{
    int  num=*(int*)arg;
    cout<<"thread "<<pthread_self()<<" is working,number is "<<num<<endl;
    sleep(1);
}

int main()
{
    ThreadPool* pool= threadPoolCreate(4,10,100);
    for(int i=0;i<100;i++)
    {
        int* num= new int(0);
        *num=i+200;
        threadPoolAdd(pool,taskFunc,num);
    }

    sleep(30);
    threadPoolDestroy(pool);
    return 0;
}