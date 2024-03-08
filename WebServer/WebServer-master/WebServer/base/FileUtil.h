// @Author Lin Ya
// @Email xxbbb@vip.qq.com
#pragma once
#include <string>
#include "noncopyable.h"

//管理对文件的操作
class AppendFile : noncopyable {
 public:
  explicit AppendFile(std::string filename);
  ~AppendFile();
  // append 日志信息字符串写入文件
  void append(const char *logline, const size_t len);
  //从用户创建的文件缓冲区写入到文件中
  void flush();

 private:
  size_t write(const char *logline, size_t len);  //操作系统层面的文件写入

  FILE *fp_;  //文件描述符
  
  char buffer_[64 * 1024];  //文件写入缓冲区
};