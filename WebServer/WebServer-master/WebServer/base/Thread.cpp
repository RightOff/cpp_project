// @Author Lin Ya
// @Email xxbbb@vip.qq.com
#include "Thread.h"
#include <assert.h>
#include <errno.h>
#include <linux/unistd.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/prctl.h>
#include <sys/types.h>
#include <unistd.h>
#include <memory>
#include "CurrentThread.h"


#include <iostream>
using namespace std;

namespace CurrentThread {
__thread int t_cachedTid = 0;
__thread char t_tidString[32];
__thread int t_tidStringLength = 6;
__thread const char* t_threadName = "default";
}

pid_t gettid() { return static_cast<pid_t>(::syscall(SYS_gettid)); }

void CurrentThread::cacheTid() {
  if (t_cachedTid == 0) {
    t_cachedTid = gettid();
    t_tidStringLength =
        snprintf(t_tidString, sizeof t_tidString, "%5d ", t_cachedTid);
  }
}

// ThreadData结构，为了在线程中保留name,tid这些数据
struct ThreadData {
  typedef Thread::ThreadFunc ThreadFunc;
  ThreadFunc func_;
  string name_;
  pid_t* tid_;  
  CountDownLatch* latch_;

  ThreadData(const ThreadFunc& func, const string& name, pid_t* tid,
             CountDownLatch* latch)
      : func_(func), name_(name), tid_(tid), latch_(latch) {}

  void runInThread() {
    *tid_ = CurrentThread::tid(); //设置存储当前线程信息CurrentThread中的线程id
    tid_ = NULL;  //为什么要传tid_进来，并没有实际用到，有何意义？
    latch_->countDown();  //latch_减1
    latch_ = NULL;

    CurrentThread::t_threadName = name_.empty() ? "Thread" : name_.c_str(); //将name_转换成C类型字符数组
    //用prctl给线程命名，系统调用，只能设置、获取当前线程的名字
    prctl(PR_SET_NAME, CurrentThread::t_threadName);

    func_();
    CurrentThread::t_threadName = "finished";
  }
};

//线程执行的函数
void* startThread(void* obj) {
  //static_cast强制类型转换，但最好用reinterpret_cast转换void*
  ThreadData* data = static_cast<ThreadData*>(obj); 
  data->runInThread();
  delete data;
  return NULL;
}

//构造函数
Thread::Thread(const ThreadFunc& func, const string& n)
    : started_(false),
      joined_(false),
      pthreadId_(0),
      tid_(0),
      func_(func),
      name_(n),
      latch_(1) {
  setDefaultName();
}

Thread::~Thread() {
  //判断线程正在运行，且还没有结束，然后会将该线程分离，当该线程终止时，立即自动回收其占用的资源，不保留终止状态。
  //不能对一个已经处于detach状态的线程调用pthread_join，这样的调用将返回EINVAL错误
  if (started_ && !joined_) pthread_detach(pthreadId_); 
}

void Thread::setDefaultName() {
  if (name_.empty()) {
    char buf[32];
    snprintf(buf, sizeof buf, "Thread");
    name_ = buf;
  }
}

void Thread::start() {
  assert(!started_);
  started_ = true;  //设置本线程为开启状态
  //存储线程信息
  ThreadData* data = new ThreadData(func_, name_, &tid_, &latch_);
  
  //创建子线程，成功返回0，data为要传入线程的参数
  if (pthread_create(&pthreadId_, NULL, &startThread, data)) {
    started_ = false;
    delete data;
  } else {
    latch_.wait();  //阻塞，确保该线程已经启动并运行传入的函数
    assert(tid_ > 0);
  }
}

int Thread::join() {
  assert(started_);
  assert(!joined_);
  joined_ = true;
  //等到该线程执行结束，释放线程资源，获取结束时的返回信息（NULL表示不进行存储）
  return pthread_join(pthreadId_, NULL);  
}