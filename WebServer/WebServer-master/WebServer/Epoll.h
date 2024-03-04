// @Author Lin Ya
// @Email xxbbb@vip.qq.com
#pragma once
#include <sys/epoll.h>
#include <memory>
#include <unordered_map>
#include <vector>
#include "Channel.h"
#include "HttpData.h"
#include "Timer.h"

//Epoll负责I/O复用的抽象，内部调用epoll_wait获取活跃的Channel
//Epoll类是事件循环EentLoop的核心结构，每一个EventLoop都持有一个Epoll分发器
class Epoll {
 public:
  Epoll();
  ~Epoll();
  
  void epoll_add(SP_Channel request, int timeout);  //添加监听事件
  void epoll_mod(SP_Channel request, int timeout);  //修改监听事件
  void epoll_del(SP_Channel request); //删除监听事件

  //poll返回活跃事件vector
  std::vector<std::shared_ptr<Channel>> poll();
  //getEventsRequest设置相应Channel中关注的事件
  std::vector<std::shared_ptr<Channel>> getEventsRequest(int events_num);
  
  //定时器相关以及超时处理
  void add_timer(std::shared_ptr<Channel> request_data, int timeout);
  void handleExpired();

  int getEpollFd() { return epollFd_; }

 private:
  static const int MAXFDS = 100000; //最大文件描述符数量
  int epollFd_;
  std::vector<epoll_event> events_; //要回传的事件列表，epoll_event中保存发生事件的文件描述符集合
  std::shared_ptr<Channel> fd2chan_[MAXFDS];  //fd到Channel的映射数组
  std::shared_ptr<HttpData> fd2http_[MAXFDS]; //fd到HttpData对象的映射数组
  TimerManager timerManager_; //定时器管理器
};