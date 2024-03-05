// @Author Lin Ya
// @Email xxbbb@vip.qq.com
#include "Server.h"
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <functional>
#include "Util.h"
#include "base/Logging.h"

Server::Server(EventLoop *loop, int threadNum, int port)
    : loop_(loop),
      threadNum_(threadNum),
      eventLoopThreadPool_(new EventLoopThreadPool(loop_, threadNum)),  //创建EventLoop线程池
      started_(false),
      acceptChannel_(new Channel(loop_)),
      port_(port),
      listenFd_(socket_bind_listen(port_)) {
  acceptChannel_->setFd(listenFd_); //将套接字描述符放入acceptChannel_
  handle_for_sigpipe();
  //将socket设置为非阻塞模式，此时如果内核缓冲区满，继续调用send/recv函数，会返回错误代码，而非阻塞在那。
  if (setSocketNonBlocking(listenFd_) < 0) {
    perror("set socket non block failed");
    abort();
  }
}

void Server::start() {
  /*创建EventLoopThreadPool，在其中创建EventLoopThread，
    并让EventLoop运行起来（loop.loop()），但没有设置监听的事件，
    除了默认监听的用于线程通信的eventfd */
  eventLoopThreadPool_->start();
  // acceptChannel_->setEvents(EPOLLIN | EPOLLET | EPOLLONESHOT);
  acceptChannel_->setEvents(EPOLLIN | EPOLLET);
  acceptChannel_->setReadHandler(bind(&Server::handNewConn, this)); //将本对象绑定到处理函数中
  acceptChannel_->setConnHandler(bind(&Server::handThisConn, this));//将本对象绑定到loop的updatePoller处理函数中
  //将acceptChannel_添加到epoll中监听socket上的事件
  loop_->addToPoller(acceptChannel_, 0);  //timeout为0
  started_ = true; 
}

void Server::handNewConn() {
  struct sockaddr_in client_addr;
  memset(&client_addr, 0, sizeof(struct sockaddr_in));
  socklen_t client_addr_len = sizeof(client_addr);
  int accept_fd = 0;
  while ((accept_fd = accept(listenFd_, (struct sockaddr *)&client_addr,
                             &client_addr_len)) > 0) {
    //从EventLoopThreadPool中分配一个EventLoop，这个EventLoop已经在loop了
    EventLoop *loop = eventLoopThreadPool_->getNextLoop();  
    LOG << "New connection from " << inet_ntoa(client_addr.sin_addr) << ":"
        << ntohs(client_addr.sin_port);
    // cout << "new connection" << endl;
    // cout << inet_ntoa(client_addr.sin_addr) << endl;
    // cout << ntohs(client_addr.sin_port) << endl;
    /*
    // TCP的保活机制默认是关闭的
    int optval = 0;
    socklen_t len_optval = 4;
    getsockopt(accept_fd, SOL_SOCKET,  SO_KEEPALIVE, &optval, &len_optval);
    cout << "optval ==" << optval << endl;
    */
    // 限制服务器的最大并发连接数
    if (accept_fd >= MAXFDS) {
      close(accept_fd);
      continue;
    }
    // 设为非阻塞模式
    if (setSocketNonBlocking(accept_fd) < 0) {
      LOG << "Set non block failed!";
      // perror("Set non block failed!");
      return;
    }

    setSocketNodelay(accept_fd);
    // setSocketNoLinger(accept_fd);

    //创建subReator
    shared_ptr<HttpData> req_info(new HttpData(loop, accept_fd));
    req_info->getChannel()->setHolder(req_info);

    //让subReator配置其默认关注的事件
    loop->queueInLoop(std::bind(&HttpData::newEvent, req_info));
  }
  acceptChannel_->setEvents(EPOLLIN | EPOLLET);
}