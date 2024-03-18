// @Author Lin Ya
// @Email xxbbb@vip.qq.com
#include "Timer.h"
#include <sys/time.h>
#include <unistd.h>
#include <queue>

//有参构造
TimerNode::TimerNode(std::shared_ptr<HttpData> requestData, int timeout)
    : deleted_(false), SPHttpData(requestData) {
  struct timeval now;
  gettimeofday(&now, NULL);
  // 以毫秒计
  expiredTime_ =
      (((now.tv_sec % 10000) * 1000) + (now.tv_usec / 1000)) + timeout; //万一now.tv_sec % 10000=9999，那么接下来的expiredTime_不就越来越小了么？
}

//析构会处理SPHttpData,一般为到期了，从优先队列中弹出，然后析构时移除相应的Channel
TimerNode::~TimerNode() {
  if (SPHttpData) SPHttpData->handleClose();
}

//拷贝构造
TimerNode::TimerNode(TimerNode &tn)
    : SPHttpData(tn.SPHttpData), expiredTime_(0) {}

//更新定时器到期时间expiredTime_
void TimerNode::update(int timeout) {
  struct timeval now;
  gettimeofday(&now, NULL);
  expiredTime_ =
      (((now.tv_sec % 10000) * 1000) + (now.tv_usec / 1000)) + timeout;
}

//验证定时器是否还有效
bool TimerNode::isValid() {
  struct timeval now;
  gettimeofday(&now, NULL); //获取当前时间
  size_t temp = (((now.tv_sec % 10000) * 1000) + (now.tv_usec / 1000));
  if (temp < expiredTime_)  //预期过期时间大于当前时间，计时器合法
    return true;
  else {
    this->setDeleted(); 
    return false;
  }
}

void TimerNode::clearReq() {
  SPHttpData.reset(); //，释放指针指向的对象，释放指向该计时器的HttpData持有者信息的指针
  this->setDeleted(); //设置该计时器删除标记为TRUE
}

TimerManager::TimerManager() {}

TimerManager::~TimerManager() {}


//添加定时器到定时器管理优先队列、SPHttpData中
void TimerManager::addTimer(std::shared_ptr<HttpData> SPHttpData, int timeout) {
  SPTimerNode new_node(new TimerNode(SPHttpData, timeout));
  timerNodeQueue.push(new_node);  //已经有过的SPHttpData，timeout可能不同，也会加进去么？
  SPHttpData->linkTimer(new_node);  //设置HttpData持有的计时器为新的计时器
}

//处理逻辑是没明白？
/* 处理逻辑是这样的~
因为(1) 优先队列不支持随机访问
(2) 即使支持，随机删除某节点后破坏了堆的结构，需要重新更新堆结构。
所以对于被置为deleted的时间节点，会延迟到它(1)超时 或
(2)它前面的节点都被删除时，它才会被删除。
一个点被置为deleted,它最迟会在TIMER_TIME_OUT时间后被删除。
这样做有两个好处：
(1) 第一个好处是不需要遍历优先队列，省时。
(2)
第二个好处是给超时时间一个容忍的时间，就是设定的超时时间是删除的下限(并不是一到超时时间就立即删除)，如果监听的请求在超时后的下一次请求中又一次出现了，
就不用再重新申请RequestData节点了，这样可以继续重复利用前面的RequestData，减少了一次delete和一次new的时间。
*/


//处理超时事件，把计时器移除,在TimerNode析构时会关闭连接
void TimerManager::handleExpiredEvent() {
  // MutexLockGuard locker(lock);
  while (!timerNodeQueue.empty()) {
    SPTimerNode ptimer_now = timerNodeQueue.top();
    if (ptimer_now->isDeleted())  //看看删除标记是否为true,一般是删除计时器或重置计时器时会设置为true
      timerNodeQueue.pop();
    else if (ptimer_now->isValid() == false)  //如果超时无效了，删除
      timerNodeQueue.pop();
    else
      break;
  }
}