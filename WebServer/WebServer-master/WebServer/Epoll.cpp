// @Author Lin Ya
// @Email xxbbb@vip.qq.com
#include "Epoll.h"
#include <assert.h>
#include <errno.h>
#include <netinet/in.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <deque>
#include <queue>
#include "Util.h"
#include "base/Logging.h"


#include <arpa/inet.h>
#include <iostream>
using namespace std;

const int EVENTSNUM = 4096;
const int EPOLLWAIT_TIME = 10000;

typedef shared_ptr<Channel> SP_Channel;
//构造、析构
//EPOLL_CLOEXEC表示当进程被替换时会关闭打开的epoll文件描述符，events_容器用EVENTSNUM个默认值初始化
Epoll::Epoll() : epollFd_(epoll_create1(EPOLL_CLOEXEC)), events_(EVENTSNUM) {
  assert(epollFd_ > 0);
}
Epoll::~Epoll() {}

// 注册新描述符
void Epoll::epoll_add(SP_Channel request, int timeout) {
  int fd = request->getFd();
  if (timeout > 0) {
    add_timer(request, timeout);
    fd2http_[fd] = request->getHolder();
  }
  struct epoll_event event;
  event.data.fd = fd;
  event.events = request->getEvents();

  //更新LastEvents
  request->EqualAndUpdateLastEvents();

  fd2chan_[fd] = request;
  if (epoll_ctl(epollFd_, EPOLL_CTL_ADD, fd, &event) < 0) {
    perror("epoll_add error");
    fd2chan_[fd].reset(); //恢复默认值
  }
}

// 修改描述符状态，调用epoll_ctl
void Epoll::epoll_mod(SP_Channel request, int timeout) {
  if (timeout > 0) add_timer(request, timeout);
  int fd = request->getFd();
  if (!request->EqualAndUpdateLastEvents()) {
    struct epoll_event event;
    event.data.fd = fd;
    event.events = request->getEvents();
    if (epoll_ctl(epollFd_, EPOLL_CTL_MOD, fd, &event) < 0) {
      perror("epoll_mod error");
      fd2chan_[fd].reset();
    }
  }
}

// 从epoll中删除描述符
void Epoll::epoll_del(SP_Channel request) {
  int fd = request->getFd();
  struct epoll_event event;
  event.data.fd = fd;
  event.events = request->getLastEvents();
  // event.events = 0;
  // request->EqualAndUpdateLastEvents()
  
  //既然是删除，应该可以直接传NULL为什么还要传event?
  if (epoll_ctl(epollFd_, EPOLL_CTL_DEL, fd, &event) < 0) {
    perror("epoll_del error");
  }
  fd2chan_[fd].reset();
  fd2http_[fd].reset();
}

// 返回活跃事件数，调用epoll_wait
std::vector<SP_Channel> Epoll::poll() {
  while (true) {
    int event_count =
        epoll_wait(epollFd_, &*events_.begin(), events_.size(), EPOLLWAIT_TIME);  //从begin位置开始填活跃事件
    if (event_count < 0) perror("epoll wait error");
    std::vector<SP_Channel> req_data = getEventsRequest(event_count);
    if (req_data.size() > 0) return req_data;
  }
}

void Epoll::handleExpired() { timerManager_.handleExpiredEvent(); } //处理超时事件

// 分发处理函数，设置相应Channel中的Revents
std::vector<SP_Channel> Epoll::getEventsRequest(int events_num) {
  std::vector<SP_Channel> req_data;
  for (int i = 0; i < events_num; ++i) {
    // 获取有事件产生的描述符
    int fd = events_[i].data.fd;

    SP_Channel cur_req = fd2chan_[fd];  

    if (cur_req) { 
      cur_req->setRevents(events_[i].events); //将发生的事件注册到fd对应的Channel中
      cur_req->setEvents(0);  //将fd对应Channel中所关注的事件置空，想要继续监听需要重新设置
      // 加入线程池之前将Timer和request分离
      // cur_req->seperateTimer();
      req_data.push_back(cur_req);  //将cur_req（指向Channel对象的指针）加入req_data（保存指针的vector）容器中
    } else {
      LOG << "SP cur_req is invalid";
    }
  }
  return req_data;
}

void Epoll::add_timer(SP_Channel request_data, int timeout) {
  shared_ptr<HttpData> t = request_data->getHolder(); //获取Channel中的持有者holder
  if (t)
    timerManager_.addTimer(t, timeout);
  else
    LOG << "timer add fail";
}