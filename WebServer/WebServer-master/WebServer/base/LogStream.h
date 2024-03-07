// @Author Lin Ya
// @Email xxbbb@vip.qq.com
#pragma once
#include <assert.h>
#include <string.h>
#include <string>
#include "noncopyable.h"

class AsyncLogging;
const int kSmallBuffer = 4000;  //小缓冲区，4KB
const int kLargeBuffer = 4000 * 1000; //大缓冲区，4MB

//管理一个固定大小的缓冲区
template <int SIZE>
class FixedBuffer : noncopyable {
 public:
  FixedBuffer() : cur_(data_) {}

  ~FixedBuffer() {}

  //向缓冲区添加数据
  void append(const char* buf, size_t len) {
    if (avail() > static_cast<int>(len)) {
      memcpy(cur_, buf, len); //内存层面的拷贝
      cur_ += len;
    }
  }

  //返回缓冲区存储信息的字符串数组
  const char* data() const { return data_; }
  //获取当前字符串存储信息的长度
  int length() const { return static_cast<int>(cur_ - data_); }
  
  //返回当前可写位置的指针
  char* current() { return cur_; }
  //判断缓冲区是否可以继续写，返回两个指针之间的距离
  int avail() const { return static_cast<int>(end() - cur_); }
  //可写位置指针后移len位
  void add(size_t len) { cur_ += len; }

  //重置缓冲区可写位置指针
  void reset() { cur_ = data_; }
  //重置缓冲区内存
  void bzero() { memset(data_, 0, sizeof data_); }

 private:
  //缓冲区字符数组末尾指针
  const char* end() const { return data_ + sizeof data_; }

  char data_[SIZE]; //缓冲区数组
  char* cur_; //指向数组中下一个可写位置的指针
};

//日志流，用于前端消息存储到前端缓冲区，
class LogStream : noncopyable {
 public:
  typedef FixedBuffer<kSmallBuffer> Buffer; //一次信息大小是有限的，因此选用小的缓冲区

  
  LogStream& operator<<(bool v) {
    buffer_.append(v ? "1" : "0", 1);
    return *this;
  }

  // 重载一系列operator<<操作符
  LogStream& operator<<(short);
  LogStream& operator<<(unsigned short);
  LogStream& operator<<(int);
  LogStream& operator<<(unsigned int);
  LogStream& operator<<(long);
  LogStream& operator<<(unsigned long);
  LogStream& operator<<(long long);
  LogStream& operator<<(unsigned long long);

  LogStream& operator<<(const void*);

  LogStream& operator<<(float v) {
    *this << static_cast<double>(v);
    return *this;
  }
  LogStream& operator<<(double);
  LogStream& operator<<(long double);

  LogStream& operator<<(char v) {
    buffer_.append(&v, 1);
    return *this;
  }

  LogStream& operator<<(const char* str) {
    if (str)
      buffer_.append(str, strlen(str));
    else
      buffer_.append("(null)", 6);
    return *this;
  }

  LogStream& operator<<(const unsigned char* str) {
    //reinterpret_cast类型转换运算符，导致实现依赖，低级别的、不安全的类型转换。（有四种强制转换?)
    return operator<<(reinterpret_cast<const char*>(str));
  }

  LogStream& operator<<(const std::string& v) {
    buffer_.append(v.c_str(), v.size());
    return *this;
  }

  //向缓冲区buffer_添加数据
  void append(const char* data, int len) { buffer_.append(data, len); }

  //返回buffer的const引用
  const Buffer& buffer() const { return buffer_; }
  //重置缓冲区
  void resetBuffer() { buffer_.reset(); }

 private:
  void staticCheck(); //未定义

  template <typename T>
  void formatInteger(T);

  Buffer buffer_; //Buffer缓冲区

  static const int kMaxNumericSize = 32;  //确保写入任何数值类型时，都不会发生缓冲区溢出
};