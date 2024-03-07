// @Author Lin Ya
// @Email xxbbb@vip.qq.com
#pragma once
#include <memory>
#include <string>
#include "FileUtil.h"
#include "MutexLock.h"
#include "noncopyable.h"


// TODO 提供自动归档功能，对日志进行操作
class LogFile : noncopyable {
 public:
  // 每被append flushEveryN次，flush一下，会往文件写，只不过，文件也是带缓冲区的
  LogFile(const std::string& basename, int flushEveryN = 1024);
  ~LogFile();

  //向日志缓冲区中追加一条日志
  void append(const char* logline, int len);
  //将文件缓冲区的内容立即写入到磁盘上
  void flush();
  //本项目并为提供日志回滚操作
  bool rollFile();  

 private:
  //不带锁的append
  void append_unlocked(const char* logline, int len); 

  const std::string basename_;  //日志文件名
  const int flushEveryN_; //append多少次后执行flush操作

  int count_; //记录append次数
  std::unique_ptr<MutexLock> mutex_;  //锁
  std::unique_ptr<AppendFile> file_;  //AppendFile对象管理实际的日志文件操作
};