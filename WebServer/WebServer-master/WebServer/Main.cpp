// @Author Lin Ya
// @Email xxbbb@vip.qq.com
#include <getopt.h>
#include <string>
#include "EventLoop.h"
#include "Server.h"
#include "base/Logging.h"


int main(int argc, char *argv[]) {
  int threadNum = 4;  //设置线程数
  int port = 80;  //设置服务器端口号
  std::string logPath = "./WebServer.log";  //日志保存路径

  // parse args
  int opt;
  const char *str = "t:l:p:";
  while ((opt = getopt(argc, argv, str)) != -1) { 
    //getopt()分析命令行参数。第三个为可选参数，包含正确的参数选项字符串，一个冒号表示选项后必须有参数，有两个冒号参数必须紧跟在选项后不能以空格隔开。

    switch (opt) {
      case 't': {
        threadNum = atoi(optarg); //转换为int，其中optarg为指向参数的指针
        break;
      }
      case 'l': {
        logPath = optarg;
        if (logPath.size() < 2 || optarg[0] != '/') {
          printf("logPath should start with \"/\"\n");
          abort();  //立即终止当前进程，产生异常程序终止，进程终止时不会损毁任何对象
        }
        break;
      }
      case 'p': {
        port = atoi(optarg);
        break;
      }
      default:
        break;
    }
  }
  Logger::setLogFileName(logPath);  //Logger类存储静态日志文件保存路径
// STL库在多线程上应用
#ifndef _PTHREADS
  LOG << "_PTHREADS is not defined !";
#endif
  EventLoop mainLoop; 
  Server myHTTPServer(&mainLoop, threadNum, port);
  myHTTPServer.start();
  mainLoop.loop();
  return 0;
}
