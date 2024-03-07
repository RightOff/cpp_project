// @Author Lin Ya
// @Email xxbbb@vip.qq.com
#pragma once
#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <string>
#include "LogStream.h"


class AsyncLogging;

class Logger {
 public:
  //构造函数调用实现类实现
  Logger(const char *fileName, int line);
  ~Logger();
  LogStream &stream() { return impl_.stream_; }

  static void setLogFileName(std::string fileName) { logFileName_ = fileName; }
  static std::string getLogFileName() { return logFileName_; }

 private:
  class Impl {
   public:
    Impl(const char *fileName, int line);
    void formatTime();

    LogStream stream_;  //日志信息
    int line_;  //记录行号
    std::string basename_;  //记录文件名
  };

  //成员变量
  Impl impl_; //实现类，实现具体功能
  static std::string logFileName_;  //类间共同使用日志文件路径logFileName_
};

// 宏定义，Logger(__FILE__, __LINE__)表示实例化一个类，
// __FILE__这个宏会被替换为当前源文件的文件名（包括路径），__LINE__这个宏会被替换为当前源代码行号
#define LOG Logger(__FILE__, __LINE__).stream() 