#include "ThreadPool.h"
#include <thread>
#include <iostream>

ThreadPool::ThreadPool(int threadNum):
    m_threadNum(threadNum),
    m_removeThread(false){
    init();
}

ThreadPool::~ThreadPool(){
    m_mutex_tasks.lock();
    while(!m_tasks.empty()){
        m_condition.notify_all();
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    m_mutex_tasks.unlock();
    removeThreads(m_threadNum);
}

void ThreadPool::addTask(Task* task){
    if(m_threadNum <= 0){
            std::cout << "线程数为0，无法添加任务\n";
            return;
        }
    if(task != nullptr)
    {
        m_mutex_remove.lock();
        while(m_removeThread == true){
            m_mutex_remove.unlock();
            std::this_thread::sleep_for(std::chrono::seconds(1));
            m_mutex_remove.lock();
        }
        m_mutex_remove.unlock();
        m_mutex_tasks.lock();
        m_tasks.push(task);
        m_mutex_tasks.unlock();
        m_condition.notify_one();
    }
}

void ThreadPool::addThreads(int num){
    for(int i = 0; i < num; ++i){
        std::thread* t = new std::thread(&ThreadPool::worker, this);
        t->detach();
    }
    m_threadNum += num;
}

void ThreadPool::removeThreads(int threadNum){
    if(threadNum > m_threadNum){
        threadNum = m_threadNum;
    }
    for(int i = 0; i < threadNum; ++i){
        m_mutex_remove.lock();
        m_removeThread = true;
        m_mutex_remove.unlock();
        m_condition.notify_one();
    }
    m_mutex_remove.lock();
    m_removeThread = false;
    m_mutex_remove.unlock();
    m_threadNum -= threadNum;
}

void ThreadPool::worker(){
    while(1){
        std::unique_lock<std::mutex> ulock(m_mutex_condition);
        m_condition.wait(ulock);
        ulock.unlock();//如果不解锁，下一个线程无法被唤醒
        m_mutex_remove.lock();
        if(m_removeThread == true){
            m_mutex_remove.unlock();
            break;
        }
        m_mutex_remove.unlock();
        m_mutex_tasks.lock();
        Task* task = m_tasks.front();
        m_tasks.pop();
        m_mutex_tasks.unlock();
        task->m_run(task->m_args);
        delete task;
    }
    std::cout << "删除线程\n";
}

void ThreadPool::init(){
    for(int i = 0; i < m_threadNum; ++i){
        std::thread* t = new std::thread(&ThreadPool::worker, this);
        t->detach();
    }
    std::cout << "线程池创建成功，线程数：" << m_threadNum << std::endl;
}
