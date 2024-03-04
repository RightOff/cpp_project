// @Author Lin Ya
// @Email xxbbb@vip.qq.com
#pragma once
#include <unistd.h>
#include <deque>
#include <memory>
#include <queue>
#include "HttpData.h"
#include "base/MutexLock.h"
#include "base/noncopyable.h"


class HttpData;

class TimerNode {
 public:
  TimerNode(std::shared_ptr<HttpData> requestData, int timeout);
  ~TimerNode();
  TimerNode(TimerNode &tn);
  void update(int timeout);
  bool isValid();
  void clearReq();
  ////设置deleted_为true
  void setDeleted() { deleted_ = true; }
  //返回deleted_值
  bool isDeleted() const { return deleted_; }
  //获得到期事件expiredTime_
  size_t getExpTime() const { return expiredTime_; }

 private:
  bool deleted_;  //表示该计时器是否需要删除
  size_t expiredTime_;
  std::shared_ptr<HttpData> SPHttpData; //记录这个计时器的HttpData持有者信息
};

//TimerNode自定义排序函数（依据到期时间从小到大排序）
struct TimerCmp {
  bool operator()(std::shared_ptr<TimerNode> &a,
                  std::shared_ptr<TimerNode> &b) const {
    return a->getExpTime() > b->getExpTime();
  }
};

class TimerManager {
 public:
  TimerManager();
  ~TimerManager();
  void addTimer(std::shared_ptr<HttpData> SPHttpData, int timeout);
  void handleExpiredEvent();

 private:
  typedef std::shared_ptr<TimerNode> SPTimerNode;
  //优先队列，采用deque作容器，每个元素存储指向TimerNode的指针
  std::priority_queue<SPTimerNode, std::deque<SPTimerNode>, TimerCmp>
      timerNodeQueue;
  // MutexLock lock;
};