// @Author Lin Ya
// @Email xxbbb@vip.qq.com
#pragma once
#include <functional>
#include <string>
#include <vector>
#include "CountDownLatch.h"
#include "LogStream.h"
#include "MutexLock.h"
#include "Thread.h"
#include "noncopyable.h"


class AsyncLogging : noncopyable {
 public:
  AsyncLogging(const std::string basename, int flushInterval = 2);
  ~AsyncLogging() {
    if (running_) stop();
  }
  void append(const char* logline, int len);

  //开启日志同步
  void start() {
    running_ = true;
    thread_.start();
    latch_.wait();
  }

  void stop() {
    running_ = false;
    cond_.notify();
    thread_.join();
  }

 private:
  void threadFunc();
  typedef FixedBuffer<kLargeBuffer> Buffer; //后端选择使用大缓冲区
  typedef std::vector<std::shared_ptr<Buffer>> BufferVector;
  typedef std::shared_ptr<Buffer> BufferPtr;

  const int flushInterval_; //前端缓冲区定期向后端写入的时间（冲刷间隔）
  bool running_;  //标识线程函数是否正在运行
  std::string basename_;  //日志文件名字
  Thread thread_; //封装的线程对象
  MutexLock mutex_;
  Condition cond_;  //条件变量，主要用于前端环缓冲区队列中没有数据时的休眠和唤醒？
  BufferPtr currentBuffer_; //当前使用的缓冲区，用于存储前端日志信息，如果不够会使用预备缓冲区
  BufferPtr nextBuffer_;  //备用缓冲区

  //缓冲区数组，过一段时间或者当前缓冲区已满，就将buffer加入到该容器中，后端线程负责将容器中的内容写入磁盘
  BufferVector buffers_;  
  CountDownLatch latch_;  //条件变量中观察的条件，其中的count_表示线程是否启动（启动为0）
};