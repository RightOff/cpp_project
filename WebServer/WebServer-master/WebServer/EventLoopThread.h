// @Author Lin Ya
// @Email xxbbb@vip.qq.com
#pragma once
#include "EventLoop.h"
#include "base/Condition.h"
#include "base/MutexLock.h"
#include "base/Thread.h"
#include "base/noncopyable.h"


class EventLoopThread : noncopyable {//noncopyable 是一个常见的工具类，用于防止对象被复制（通过删除复制构造函数和赋值运算符）。
 public:
  EventLoopThread();
  ~EventLoopThread();
  EventLoop* startLoop();

 private:
  EventLoop* loop_; //一个指向 EventLoop 对象的指针，用于在当前线程中执行事件循环。
  bool exiting_;  //用于指示线程是否应该退出其事件循环。
  Thread thread_; //一个 Thread 对象，代表执行事件循环的线程。
  MutexLock mutex_; //用于保护共享资源（如 exiting_ 标志）免受多个线程的同时访问。
  Condition cond_;  //条件变量，与mutex_关联实现线程同步
  
  void threadFunc();

};