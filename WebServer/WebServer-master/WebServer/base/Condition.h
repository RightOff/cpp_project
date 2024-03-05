// @Author Lin Ya
// @Email xxbbb@vip.qq.com
#pragma once
#include <errno.h>
#include <pthread.h>
#include <pthread.h>
#include <time.h>
#include <cstdint>
#include "MutexLock.h"
#include "noncopyable.h"


class Condition : noncopyable {
 public:
  explicit Condition(MutexLock &_mutex) : mutex(_mutex) {
    pthread_cond_init(&cond, NULL); //初始化pthread_cond_t
  }
  ~Condition() { pthread_cond_destroy(&cond); }
  //陷入阻塞等待
  void wait() { pthread_cond_wait(&cond, mutex.get()); }
  //唤醒一个等待该条件的线程
  void notify() { pthread_cond_signal(&cond); }
  //唤醒等待该条件的所有线程
  void notifyAll() { pthread_cond_broadcast(&cond); }

  //陷入阻塞等待，并设置超时时间
  bool waitForSeconds(int seconds) {
    struct timespec abstime;  //当前时间结构体，精度为纳秒。struct timeval精度为微秒
    clock_gettime(CLOCK_REALTIME, &abstime);  //获取当前时间
    abstime.tv_sec += static_cast<time_t>(seconds); //将传入的seconds转换为time_t类型，其是秒的类型
    return ETIMEDOUT == pthread_cond_timedwait(&cond, mutex.get(), &abstime);
  }

 private:
  MutexLock &mutex;
  pthread_cond_t cond;
};