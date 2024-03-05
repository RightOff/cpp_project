// @Author Lin Ya
// @Email xxbbb@vip.qq.com
#include "EventLoopThread.h"
#include <functional>

//无参构造
EventLoopThread::EventLoopThread()
    : loop_(NULL),
      exiting_(false),
      //绑定当前对象指针到threadFunc，然后传递到thread
      thread_(bind(&EventLoopThread::threadFunc, this), "EventLoopThread"),
      mutex_(),
      //初始化条件变量
      cond_(mutex_) {}

EventLoopThread::~EventLoopThread() {
  exiting_ = true;
  if (loop_ != NULL) {
    loop_->quit();  //关闭EventLoop
    thread_.join(); //等到线程执行结束
  }
}

EventLoop* EventLoopThread::startLoop() {
  assert(!thread_.started());
  thread_.start();
  {
    MutexLockGuard lock(mutex_);
    // 一直等到threadFun在Thread里真正跑起来
    while (loop_ == NULL) cond_.wait();
  }
  return loop_;
}

void EventLoopThread::threadFunc() {
  EventLoop loop; //在线程中创建一个EventLoop并运行

  {
    MutexLockGuard lock(mutex_);
    loop_ = &loop;  //将本EventLoop传递给本EventLoopThread中的loop_
    cond_.notify(); //通知EventLoopThread，线程已创建完毕且已经创建EventLoop
  }

  loop.loop();  //该线程开始监听、处理客户端套接字
  // assert(exiting_);
  loop_ = NULL; //本线程创建完毕，置空准备创建下一个线程
}