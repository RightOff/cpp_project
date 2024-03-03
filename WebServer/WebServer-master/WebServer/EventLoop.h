// @Author Lin Ya
// @Email xxbbb@vip.qq.com
#pragma once
#include <functional>
#include <memory>
#include <vector>
#include "Channel.h"
#include "Epoll.h"
#include "Util.h"
#include "base/CurrentThread.h"
#include "base/Logging.h"
#include "base/Thread.h"


#include <iostream>
using namespace std;
//EventLoop是一个事件循环
class EventLoop {
 public:
  typedef std::function<void()> Functor;
  EventLoop();
  ~EventLoop();
  void loop();
  void quit();
  void runInLoop(Functor&& cb);
  void queueInLoop(Functor&& cb);

  bool isInLoopThread() const { return threadId_ == CurrentThread::tid(); }
  void assertInLoopThread() { assert(isInLoopThread()); }
  void shutdown(shared_ptr<Channel> channel) { shutDownWR(channel->getFd()); }
  
  //以下三个对epoll操作的函数只会在EventLoop本线程内调用
  //从epoll中删除epoll_event
  void removeFromPoller(shared_ptr<Channel> channel) {
    // shutDownWR(channel->getFd());
    poller_->epoll_del(channel);
  }
  //从epoll中更新epoll_event
  void updatePoller(shared_ptr<Channel> channel, int timeout = 0) {
    poller_->epoll_mod(channel, timeout);
  }
  //从epoll中添加epoll_event
  void addToPoller(shared_ptr<Channel> channel, int timeout = 0) {
    poller_->epoll_add(channel, timeout);
  }

 private:
  // 声明顺序 wakeupFd_ > pwakeupChannel_
  bool looping_;  //原子操作，通过CAS实现，标志正在执行事件循环（该loop是否运行起来）
  shared_ptr<Epoll> poller_;  //epoll结构，事件循环的核心部分。其生命周期由EventLoop控制
  int wakeupFd_;  //mainLoop向subLoop::wakeupFd写数据唤醒。唤醒通道，用于线程间的通信，主要是通知唤醒一个阻塞于epoll_wait中的EventLoop
  
  bool quit_; //标志退出状态
  bool eventHandling_;  //标志是否处于回调各个有事件产生的通道的回调函数过程中  
  mutable MutexLock mutex_; //锁，跨线程注册Channel时加锁。被mutable修饰的变量，将永远处于可变的状态，即使在一个const函数中
  
  std::vector<Functor> pendingFunctors_;
  bool callingPendingFunctors_;
  const pid_t threadId_;
  shared_ptr<Channel> pwakeupChannel_;  //通知通道，用于MainReactor唤醒SubReactor

  //只能被本类内部或友元访问
  void wakeup();
  void handleRead();
  void doPendingFunctors();
  void handleConn();
};
