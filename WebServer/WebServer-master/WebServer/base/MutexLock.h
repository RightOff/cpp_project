// @Author Lin Ya
// @Email xxbbb@vip.qq.com
#pragma once
#include <pthread.h>
#include <cstdio>
#include "noncopyable.h"

//封装mutex线程锁，实现多线程同步
class MutexLock : noncopyable {
 public:
  //初始化线程锁
  MutexLock() { pthread_mutex_init(&mutex, NULL); }
  ~MutexLock() {
    pthread_mutex_lock(&mutex); //等待持有该锁的线程全部释放再析构
    pthread_mutex_destroy(&mutex);
  }
  //获取线程锁
  void lock() { pthread_mutex_lock(&mutex); }
  //释放线程锁
  void unlock() { pthread_mutex_unlock(&mutex); }
  pthread_mutex_t *get() { return &mutex; } //返回指向该锁的指针

 private:
  pthread_mutex_t mutex;

  // 友元类不受访问权限影响
 private:
  friend class Condition;
};

//封装管理线程锁
class MutexLockGuard : noncopyable {
 public:
  explicit MutexLockGuard(MutexLock &_mutex) : mutex(_mutex) { mutex.lock(); }
  ~MutexLockGuard() { mutex.unlock(); }

 private:
  MutexLock &mutex;
};