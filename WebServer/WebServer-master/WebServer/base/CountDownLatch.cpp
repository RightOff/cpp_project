// @Author Lin Ya
// @Email xxbbb@vip.qq.com
#include "CountDownLatch.h"

CountDownLatch::CountDownLatch(int count)
    : mutex_(), condition_(mutex_), count_(count) {}

//事件等待
void CountDownLatch::wait() {
  MutexLockGuard lock(mutex_);  //该函数调用结束后自动析构，析构时会释放线程锁
  //当count_大于0，说明线程还未启动，进入事件等待，如果小于0，说明线程已启动。该函数处理结束
  while (count_ > 0) condition_.wait(); 
}

//事件激活，表明该线程正常运行
void CountDownLatch::countDown() {
  MutexLockGuard lock(mutex_);
  --count_;
  if (count_ == 0) condition_.notifyAll();  //唤醒所有等待count_条件的线程
}