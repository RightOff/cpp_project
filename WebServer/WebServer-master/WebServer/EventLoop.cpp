// @Author Lin Ya
// @Email xxbbb@vip.qq.com
#include "EventLoop.h"
#include <sys/epoll.h>
#include <sys/eventfd.h>
#include <iostream>
#include "Util.h"
#include "base/Logging.h"
#include <thread>

using namespace std;

__thread EventLoop* t_loopInThisThread = 0;

//创建eventfd，实现多进程或多线程之间的事件通知
int createEventfd() {
  //EFD_NONBLOCK非阻塞，EFD_CLOEXEC调用exec后自动关闭文件描述符
  int evtfd = eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
  if (evtfd < 0) {
    LOG << "Failed in eventfd";
    abort();  //不进行任何清理工作，直接终止程序
  }
  return evtfd;
}

//构造函数
EventLoop::EventLoop()
    : looping_(false),  //默认没有执行事件循环
      poller_(new Epoll()), //创建一个Epoll对象
      wakeupFd_(createEventfd()), //创建eventfd
      quit_(false), //退出状态为false
      eventHandling_(false),  //未处于回调函数中
      callingPendingFunctors_(false),
      threadId_(CurrentThread::tid()),
      //将本事件和eventfd文件描述符封装为Channel
      pwakeupChannel_(new Channel(this, wakeupFd_)) { 
  if (t_loopInThisThread) {
    // LOG << "Another EventLoop " << t_loopInThisThread << " exists in this
    // thread " << threadId_;
  } else {
    t_loopInThisThread = this;
  }
  // pwakeupChannel_->setEvents(EPOLLIN | EPOLLET | EPOLLONESHOT);
  pwakeupChannel_->setEvents(EPOLLIN | EPOLLET);  //可读、边缘触发
  pwakeupChannel_->setReadHandler(bind(&EventLoop::handleRead, this));  //向Channel传入可读事件的回调函数
  pwakeupChannel_->setConnHandler(bind(&EventLoop::handleConn, this));  //向Channel传入handleConn回调函数？
  poller_->epoll_add(pwakeupChannel_, 0); //连接Epoll和Channel 
}
//回调函数
void EventLoop::handleConn() {
  // poller_->epoll_mod(wakeupFd_, pwakeupChannel_, (EPOLLIN | EPOLLET |
  // EPOLLONESHOT), 0);
  updatePoller(pwakeupChannel_, 0);
}
//EventLoop析构
EventLoop::~EventLoop() {
  // wakeupChannel_->disableAll();
  // wakeupChannel_->remove();
  close(wakeupFd_);
  t_loopInThisThread = NULL;
}
//其他线程通过本函数唤醒本线程（从epoll_wait阻塞中唤醒）
void EventLoop::wakeup() {
  uint64_t one = 1;
  ssize_t n = writen(wakeupFd_, (char*)(&one), sizeof one); //传输一个字节数据，唤醒其他线程
  if (n != sizeof one) {
    LOG << "EventLoop::wakeup() writes " << n << " bytes instead of 8";
  }
}
//可读事件的回调函数，在这里用于线程间通信，唤醒本子线程好像？
void EventLoop::handleRead() {
  uint64_t one = 1;
  ssize_t n = readn(wakeupFd_, &one, sizeof one); //只读8bit
  if (n != sizeof one) {
    LOG << "EventLoop::handleRead() reads " << n << " bytes instead of 8";
  }
  // pwakeupChannel_->setEvents(EPOLLIN | EPOLLET | EPOLLONESHOT);
  //重新设置监听事件是因为每次监听到事件后，在传递的过程中会将关注事件清空
  pwakeupChannel_->setEvents(EPOLLIN | EPOLLET);  
}

/*如果在当前线程中调用本线程Eventloop对象的runInLoop,那么直接执行回调函数.
如果该EventLoop不在当前线程中，那么将调用queueInloop把该回调函数放到待执行
回调函数队列的末尾，并通过写eventfd唤醒该EventLoop线程。
*/
void EventLoop::runInLoop(Functor&& cb) {
  if (isInLoopThread())
    cb();
  else
    queueInLoop(std::move(cb));
}

/*本线程也可以调用queueInloop将cb放到待执行函数队列中，如果该EventLoop
正处于loop函数中的执行callingPendingFunctors阶段，就需要写eventfd,否则
如果下一次一直没有事件产生，可能会影响下一次doPendingFunctors()的执行时间，
导致新添加的回调函数的执行为无限期拖后*/
void EventLoop::queueInLoop(Functor&& cb) {
  {
    MutexLockGuard lock(mutex_);  //加锁，本函数执行结束后，会析构，析构时解锁。互斥的访问待处理事件队列
    pendingFunctors_.emplace_back(std::move(cb));
  }

  // cout << std::this_thread::get_id() << std::endl;

/*   当前线程不是事件循环所在线程，在这种情况下需要唤醒事件循环线程及时处理函数队列中的函数 || 
  callingPendingFunctors_为true说明正在处理函数队列，通过wakeup让其在执行完后，下一轮循环时不会阻塞，继续拿取函数队列中的函数处理 */
  
  if (!isInLoopThread() || callingPendingFunctors_) wakeup();
}

void EventLoop::loop() {
  assert(!looping_);
  assert(isInLoopThread());
  looping_ = true;
  quit_ = false;
  // LOG_TRACE << "EventLoop " << this << " start looping";
  std::vector<SP_Channel> ret;
  while (!quit_) {
    // cout << "doing" << endl;
    ret.clear();
    ret = poller_->poll();  //获取活跃事件
    
    eventHandling_ = true;
    for (auto& it : ret) it->handleEvents();  //处理活跃的事件
    eventHandling_ = false;
    
    doPendingFunctors();  
    poller_->handleExpired(); //调用定时器处理超时连接  
  }
  looping_ = false; //标志循环关闭
}
//执行其他线程调用queueinLoop添加进去的函数队列
void EventLoop::doPendingFunctors() {
  std::vector<Functor> functors;
  callingPendingFunctors_ = true;

  //人为制造作用域
  {
    MutexLockGuard lock(mutex_);
    functors.swap(pendingFunctors_);
  }
  //以上作用域结束

  for (size_t i = 0; i < functors.size(); ++i) functors[i](); //执行pendingFunctors_中的函数
  callingPendingFunctors_ = false;
}

//主动关闭事件循环
void EventLoop::quit() {
  quit_ = true; //表示该EventLoop将要销毁
  //判断是否是当前线程调用，如果是，本EventLoop已经处理完所有事件，可以直接结束。如果不是，则唤醒EventLoop去处理剩余事件
  if (!isInLoopThread()) {
    //唤醒之后会继续处理一轮事件，然后再进入判断语句，然后因为 quit_ = true 而关闭loop,退出循环处理。
    wakeup();
  }
}