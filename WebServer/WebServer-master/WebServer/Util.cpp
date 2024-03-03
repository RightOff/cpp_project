// @Author Lin Ya
// @Email xxbbb@vip.qq.com
#include "Util.h"

#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <signal.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>


const int MAX_BUFF = 4096;
//读缓冲区
ssize_t readn(int fd, void *buff, size_t n) {
  size_t nleft = n;
  ssize_t nread = 0;
  ssize_t readSum = 0;
  char *ptr = (char *)buff;
  while (nleft > 0) {
    if ((nread = read(fd, ptr, nleft)) < 0) {
      if (errno == EINTR)
        nread = 0;
      else if (errno == EAGAIN) {
        return readSum;
      } else {
        return -1;
      }
    } else if (nread == 0)
      break;
    readSum += nread;
    nleft -= nread;
    ptr += nread;
  }
  return readSum;
}

////封装read
ssize_t readn(int fd, std::string &inBuffer, bool &zero) {
  ssize_t nread = 0;
  ssize_t readSum = 0;
  while (true) {
    char buff[MAX_BUFF];
    //socket设置了非阻塞，因此是非阻塞读取
    if ((nread = read(fd, buff, MAX_BUFF)) < 0) {
      if (errno == EINTR) //系统中断，重新处理
        continue;
      else if (errno == EAGAIN) { //缓冲区已经读完
        return readSum;
      } else {
        perror("read error");
        return -1;
      }
    } else if (nread == 0) {  //最可能是客户端连接关闭，可能有其他原因
      // printf("redsum = %d\n", readSum);
      zero = true;
      break;
    }
    // printf("before inBuffer.size() = %d\n", inBuffer.size());
    // printf("nread = %d\n", nread);
    readSum += nread;
    // buff += nread;

    //将字符数组buff转换成string
    inBuffer += std::string(buff, buff + nread);
    // printf("after inBuffer.size() = %d\n", inBuffer.size());
  }
  return readSum;
}


ssize_t readn(int fd, std::string &inBuffer) {
  ssize_t nread = 0;
  ssize_t readSum = 0;
  while (true) {
    char buff[MAX_BUFF];
    //socket设置了非阻塞，因此是非阻塞读取
    if ((nread = read(fd, buff, MAX_BUFF)) < 0) {
      if (errno == EINTR) //系统中断，重新处理
        continue;
      else if (errno == EAGAIN) { //缓冲区已经读完
        return readSum;
      } else {
        perror("read error");
        return -1;
      }
    } else if (nread == 0) {  //客户端连接关闭
      // printf("redsum = %d\n", readSum);
      break;
    }
    // printf("before inBuffer.size() = %d\n", inBuffer.size());
    // printf("nread = %d\n", nread);
    readSum += nread;
    // buff += nread;
    
    //将字符数组buff转换成string
    inBuffer += std::string(buff, buff + nread);  
    // printf("after inBuffer.size() = %d\n", inBuffer.size());
  }
  return readSum;
}

ssize_t writen(int fd, void *buff, size_t n) {
  size_t nleft = n;
  ssize_t nwritten = 0;
  ssize_t writeSum = 0;
  char *ptr = (char *)buff;
  while (nleft > 0) {
    if ((nwritten = write(fd, ptr, nleft)) <= 0) {
      if (nwritten < 0) {
        if (errno == EINTR) {
          nwritten = 0;
          continue;
        } else if (errno == EAGAIN) {
          return writeSum;
        } else
          return -1;
      }
    }
    writeSum += nwritten;
    nleft -= nwritten;
    ptr += nwritten;
  }
  return writeSum;
}

ssize_t writen(int fd, std::string &sbuff) {
  size_t nleft = sbuff.size();
  ssize_t nwritten = 0;
  ssize_t writeSum = 0;
  const char *ptr = sbuff.c_str();
  while (nleft > 0) {
    if ((nwritten = write(fd, ptr, nleft)) <= 0) {
      if (nwritten < 0) {
        if (errno == EINTR) {
          nwritten = 0;
          continue;
        } else if (errno == EAGAIN)
          break;
        else
          return -1;
      }
    }
    writeSum += nwritten;
    nleft -= nwritten;
    ptr += nwritten;
  }
  if (writeSum == static_cast<int>(sbuff.size()))
    sbuff.clear();
  else
    sbuff = sbuff.substr(writeSum);
  return writeSum;
}

//设置SIGPIPE的信号处理函数为SIG_IGN，即忽略对SIGPIPE信号的处理
void handle_for_sigpipe() {
  struct sigaction sa;  //查询或设置信号处理方式
  memset(&sa, '\0', sizeof(sa));
  sa.sa_handler = SIG_IGN;  //指定信号处理方式为SIG_IGN，即忽略信号
  sa.sa_flags = 0;
  if (sigaction(SIGPIPE, &sa, NULL)) return;  //设置SIGPIPE的信号处理函数为SIG_IGN
}

//将Socket设置为非阻塞
int setSocketNonBlocking(int fd) {
  int flag = fcntl(fd, F_GETFL, 0);
  if (flag == -1) return -1;

  flag |= O_NONBLOCK;
  if (fcntl(fd, F_SETFL, flag) == -1) return -1;
  return 0;
}

//禁用Nagle算法
void setSocketNodelay(int fd) {
  int enable = 1;
  setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, (void *)&enable, sizeof(enable));
}

void setSocketNoLinger(int fd) {
  struct linger linger_;
  linger_.l_onoff = 1;
  linger_.l_linger = 30;
  setsockopt(fd, SOL_SOCKET, SO_LINGER, (const char *)&linger_,
             sizeof(linger_));
}

void shutDownWR(int fd) {
  shutdown(fd, SHUT_WR);
  // printf("shutdown\n");
}
//服务器段创建套接字并监听
int socket_bind_listen(int port) {
  // 检查port值，取正确区间范围
  if (port < 0 || port > 65535) return -1;

  // 创建socket(IPv4 + TCP)，返回监听描述符
  int listen_fd = 0;
  if ((listen_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1) return -1;

  // 消除bind时"Address already in use"错误
  int optval = 1;
  //设置SO_REUSEADDR，并将其值改为(true)实现端口复用
  if (setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &optval,
                 sizeof(optval)) == -1) {
    close(listen_fd);
    return -1;
  }

  // 设置服务器IP和Port，和监听描述副绑定，返回套接字文件描述符
  struct sockaddr_in server_addr;
  bzero((char *)&server_addr, sizeof(server_addr));
  server_addr.sin_family = AF_INET;
  server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
  server_addr.sin_port = htons((unsigned short)port);
  if (bind(listen_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) ==
      -1) {
    close(listen_fd);
    return -1;
  }

  // 开始监听，最大等待队列长为2048
  if (listen(listen_fd, 2048) == -1) {
    close(listen_fd);
    return -1;
  }

  // 无效监听描述符
  if (listen_fd == -1) {
    close(listen_fd);
    return -1;
  }
  return listen_fd;
}