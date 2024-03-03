// @Author Lin Ya
// @Email xxbbb@vip.qq.com
#pragma once
#include <memory>
#include <vector>
#include "EventLoopThread.h"
#include "base/Logging.h"
#include "base/noncopyable.h"


class EventLoopThreadPool : noncopyable {
 public:
  EventLoopThreadPool(EventLoop* baseLoop, int numThreads);

  ~EventLoopThreadPool() { LOG << "~EventLoopThreadPool()"; }
  void start();

  EventLoop* getNextLoop();

 private:
  EventLoop* baseLoop_; //MainReactor中的loop
  bool started_;  //开始状态
  int numThreads_;  //线程数量
  int next_;  //线程池标号
  std::vector<std::shared_ptr<EventLoopThread>> threads_; //EventLoopThread线程池
  std::vector<EventLoop*> loops_; //EventLoop池
};