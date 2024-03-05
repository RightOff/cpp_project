// @Author Lin Ya
// @Email xxbbb@vip.qq.com
#include "EventLoopThreadPool.h"

EventLoopThreadPool::EventLoopThreadPool(EventLoop *baseLoop, int numThreads)
    : baseLoop_(baseLoop), started_(false), numThreads_(numThreads), next_(0) {
  if (numThreads_ <= 0) {
    LOG << "numThreads_ <= 0";
    abort();
  }
}

void EventLoopThreadPool::start() {
  baseLoop_->assertInLoopThread();  //仅供调试时使用
  started_ = true;  //设置线程池为开启状态
  //将指定数量的线程逐个开启
  for (int i = 0; i < numThreads_; ++i) {
    std::shared_ptr<EventLoopThread> t(new EventLoopThread());  //创建一个EventLoopThread
    threads_.push_back(t);  //添加进EventLoopThread线程池
    loops_.push_back(t->startLoop()); //开启Loop，并返回loop，将添loop加进EventLoop池
  }
}

EventLoop *EventLoopThreadPool::getNextLoop() {
  baseLoop_->assertInLoopThread();
  assert(started_);
  EventLoop *loop = baseLoop_;  //loop暂时指向MainReactor的loop
  if (!loops_.empty()) {
    loop = loops_[next_]; //从EventLoop池中按顺序循环选择一个EventLoop分配
    next_ = (next_ + 1) % numThreads_;  //计算下一个可分配的EventLoop
  }
  return loop;
}