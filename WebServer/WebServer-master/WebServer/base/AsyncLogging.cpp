// @Author Lin Ya
// @Email xxbbb@vip.qq.com
#include "AsyncLogging.h"
#include <assert.h>
#include <stdio.h>
#include <unistd.h>
#include <functional>
#include "LogFile.h"

AsyncLogging::AsyncLogging(std::string logFileName_, int flushInterval)
    : flushInterval_(flushInterval),
      running_(false),
      basename_(logFileName_),
      thread_(std::bind(&AsyncLogging::threadFunc, this), "Logging"),
      mutex_(),
      cond_(mutex_),
      currentBuffer_(new Buffer),
      nextBuffer_(new Buffer),
      buffers_(),
      latch_(1) {
  assert(logFileName_.size() > 1);
  currentBuffer_->bzero();
  nextBuffer_->bzero();
  buffers_.reserve(16); //设置缓冲区容器的大小为16
}

void AsyncLogging::append(const char* logline, int len) {
  MutexLockGuard lock(mutex_);
  if (currentBuffer_->avail() > len)
    currentBuffer_->append(logline, len);
  else {
    buffers_.push_back(currentBuffer_);
    currentBuffer_.reset();
    if (nextBuffer_)
      currentBuffer_ = std::move(nextBuffer_);
    else
      currentBuffer_.reset(new Buffer);
    currentBuffer_->append(logline, len);
    cond_.notify();
  }
}

//后端线程中要执行的函数
void AsyncLogging::threadFunc() {
  assert(running_ == true);
  latch_.countDown(); //等待条件满足，通知线程已启动
  LogFile output(basename_);  //日志文件对象
  BufferPtr newBuffer1(new Buffer); 
  BufferPtr newBuffer2(new Buffer); 
  newBuffer1->bzero();
  newBuffer2->bzero();
  BufferVector buffersToWrite;  //待写入文件的缓冲区数组
  buffersToWrite.reserve(16);   //指定存放缓冲区的大小为16

  //向文件中写日志
  while (running_) {
    assert(newBuffer1 && newBuffer1->length() == 0);
    assert(newBuffer2 && newBuffer2->length() == 0);
    assert(buffersToWrite.empty());

    //前端向后端的buffersToWrite传递日志容器
    {
      MutexLockGuard lock(mutex_);
      //如果buffers_容器为空，等待flushInterval_时间，前端向currentBuffer_放入信息后再继续
      if (buffers_.empty())  // unusual usage!
      {
        cond_.waitForSeconds(flushInterval_);
      }
      buffers_.push_back(currentBuffer_); //将前端存储的日志信息buffer放入vector

      //对指针的reset，指向缓冲区对象的引用计数减1，如果该计数变为0，则删除该对象，释放其内存将currentBuffer重置为空指针
      currentBuffer_.reset(); 

      currentBuffer_ = std::move(newBuffer1); //移动赋值，currentBuffer_接管newBuffer1
      buffersToWrite.swap(buffers_);  //将待写入日志信息的容器内容交给buffersToWrite写入文件中
      //如果备用缓冲区不存在，将newBuffer2给它
      if (!nextBuffer_) {
        nextBuffer_ = std::move(newBuffer2);
      }
    }

    assert(!buffersToWrite.empty());

    //后端控制buffersToWrite将日志信息转交给LogFile对象，让其负责写入磁盘

    //为什么是25？删掉之后的元素，岂不是有的日志信息不再写入文件了？
    if (buffersToWrite.size() > 25) {
      // char buf[256];
      // snprintf(buf, sizeof buf, "Dropped log messages at %s, %zd larger
      // buffers\n",
      //          Timestamp::now().toFormattedString().c_str(),
      //          buffersToWrite.size()-2);
      // fputs(buf, stderr);
      // output.append(buf, static_cast<int>(strlen(buf)));
      buffersToWrite.erase(buffersToWrite.begin() + 2, buffersToWrite.end());
    }

    //将缓冲区内容交由Logile写入文件中
    for (size_t i = 0; i < buffersToWrite.size(); ++i) {
      // FIXME: use unbuffered stdio FILE ? or use ::writev ?
      output.append(buffersToWrite[i]->data(), buffersToWrite[i]->length());
    }

    if (buffersToWrite.size() > 2) {
      // drop non-bzero-ed buffers, avoid trashing
      buffersToWrite.resize(2);
    }

    if (!newBuffer1) {
      assert(!buffersToWrite.empty());
      //将buffersToWrite中的最后一个指向buffer的指针赋给newBuffer1
      newBuffer1 = buffersToWrite.back();
      buffersToWrite.pop_back();  //移除最后一个元素
      //对对象的reset，重置缓冲区可写位置为数组头部
      newBuffer1->reset();
    }

    if (!newBuffer2) {
      assert(!buffersToWrite.empty());
      newBuffer2 = buffersToWrite.back(); 
      buffersToWrite.pop_back();  
      newBuffer2->reset();  //
    }

    buffersToWrite.clear(); //清除后端缓冲区容器内容
    output.flush(); //立即写入磁盘
  }
  output.flush(); //以防有剩余日志为写入磁盘
}
