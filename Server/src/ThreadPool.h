#pragma once
#include <functional>
#include <queue>
#include <condition_variable>
#include <mutex>

struct Task{
    std::function<void(void *)> m_run;
    void* m_args;
};

class ThreadPool{
public:
    ThreadPool(int threadNum);
    ~ThreadPool();
    void addTask(Task* task);
    void addThreads(int num);
    void removeThreads(int threadNum);//移除指定数量线程
private:
    void worker();
    void init();//创建线程
private:
    std::queue<Task*> m_tasks;
    std::condition_variable m_condition;
    std::mutex m_mutex_condition;
    std::mutex m_mutex_tasks;
    int m_threadNum;
    bool m_removeThread;
    std::mutex m_mutex_remove;
};