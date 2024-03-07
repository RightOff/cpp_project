// @Author Lin Ya
// @Email xxbbb@vip.qq.com
#pragma once
#include <pthread.h>
#include <sys/syscall.h>
#include <unistd.h>
#include <functional>
#include <memory>
#include <string>
#include "CountDownLatch.h"
#include "noncopyable.h"

class Thread : noncopyable {
 public:
  typedef std::function<void()> ThreadFunc;
  //函数声明可以没有形参名
  explicit Thread(const ThreadFunc&, const std::string& name = std::string());
  ~Thread();
  void start();
  int join();
  bool started() const { return started_; }
  pid_t tid() const { return tid_; }
  const std::string& name() const { return name_; }

 private:
  void setDefaultName();  //如果名字为空，设置默认名字
  bool started_;  //线程开启状态
  bool joined_; //标志线程是否结束
  pthread_t pthreadId_; //线程id
  pid_t tid_; //进程id
  ThreadFunc func_;
  std::string name_;  //线程名
  CountDownLatch latch_;  //条件变量观察的条件
};