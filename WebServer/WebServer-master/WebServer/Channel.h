// @Author Lin Ya
// @Email xxbbb@vip.qq.com
#pragma once
#include <sys/epoll.h>
#include <sys/epoll.h>
#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include "Timer.h"

class EventLoop;
class HttpData;

//对文件描述符和需要监听的事件进行封装，还存放着事件发生时的回调函数
class Channel {
 private:
  typedef std::function<void()> CallBack;
  EventLoop *loop_; //当前Channle属于的EentLoop
  int fd_;  //监听的文件描述符
  __uint32_t events_; //注册感兴趣的事件类型集合
  __uint32_t revents_;  //监听到的事件类型集合
  __uint32_t lastEvents_;

  // 方便找到上层持有该Channel的对象
  //上层持有者的生命周期不受Channel的控制，所以用weak_ptr并能够形成弱回调语义
  std::weak_ptr<HttpData> holder_;  

 private:
  //可能被废弃的函数
  int parse_URI();
  int parse_Headers();
  int analysisRequest();

  //事件处理函数（函数对象）
  CallBack readHandler_;   //读事件回调函数
  CallBack writeHandler_;  //写事件回调函数
  CallBack errorHandler_;  //错误发生回调函数
  CallBack connHandler_;   //回调函数

 public:
  //构造、析构函数
  Channel(EventLoop *loop);
  Channel(EventLoop *loop, int fd);
  ~Channel();

  //获取、设置文件描述符
  int getFd();
  void setFd(int fd);

  //设置、获取holder
  void setHolder(std::shared_ptr<HttpData> holder) { holder_ = holder; }
  std::shared_ptr<HttpData> getHolder() {
    //尝试获取holder,如果不存在，返回空的share_ptr
    std::shared_ptr<HttpData> ret(holder_.lock()); 
    return ret;
  }

  //设置四个回调函数对象
  void setReadHandler(CallBack &&readHandler) { readHandler_ = readHandler; }
  void setWriteHandler(CallBack &&writeHandler) {
    writeHandler_ = writeHandler;
  }
  void setErrorHandler(CallBack &&errorHandler) {
    errorHandler_ = errorHandler;
  }
  void setConnHandler(CallBack &&connHandler) { connHandler_ = connHandler; }
  
  //事件处理函数
  void handleEvents() {
    events_ = 0;
    //EPOLLHUP：表示挂断，读写都关闭；EPOLLIN：数据可读（包含普通和优先数据）
    if ((revents_ & EPOLLHUP) && !(revents_ & EPOLLIN)) {
      events_ = 0;
      return;
    }
    //套接字错误
    if (revents_ & EPOLLERR) {
      if (errorHandler_) errorHandler_();
      events_ = 0;
      return;
    }
    //EPOLLPRI：高优先级数据可读（比如TCP带外数据）
    //EPOLLRDHUP：TCP连接被对方关闭或者对方只关闭了写操作（不是所有版本都支持）
    if (revents_ & (EPOLLIN | EPOLLPRI | EPOLLRDHUP)) { 
      handleRead();
    }
    //数据可写（包含普通和优先数据）
    if (revents_ & EPOLLOUT) {  
      handleWrite();
    }
    handleConn();
  }
  void handleRead();
  void handleWrite();
  void handleError(int fd, int err_num, std::string short_msg);
  void handleConn();

  //设置监听到的事件
  void setRevents(__uint32_t ev) { revents_ = ev; }
  //设置、获取感兴趣的事件
  void setEvents(__uint32_t ev) { events_ = ev; }
  //获取监听事件集合
  __uint32_t &getEvents() { return events_; }

  //判断本次关注的事件与上一次关注的事件相比，是否相同
  bool EqualAndUpdateLastEvents() {
    bool ret = (lastEvents_ == events_);
    lastEvents_ = events_;
    return ret;
  }
  //获取lastEvents_
  __uint32_t getLastEvents() { return lastEvents_; }
};

typedef std::shared_ptr<Channel> SP_Channel;