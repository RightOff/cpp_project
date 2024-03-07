// @Author Lin Ya
// @Email xxbbb@vip.qq.com
#include "FileUtil.h"
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <sys/stat.h>
#include <unistd.h>

using namespace std;


AppendFile::AppendFile(string filename) : fp_(fopen(filename.c_str(), "ae")) {//标准文件打开模式是没有ae的？
  // 设置文件写入缓冲区，缓冲区由用户提供，后续操作并不需要操作buffer,直接操作文件
  setbuffer(fp_, buffer_, sizeof buffer_);
}

AppendFile::~AppendFile() { fclose(fp_); }

void AppendFile::append(const char* logline, const size_t len) {
  size_t n = this->write(logline, len); //写入文件
  size_t remain = len - n;  //剩余未写入的字节数
  while (remain > 0) {
    size_t x = this->write(logline + n, remain);
    if (x == 0) {
      int err = ferror(fp_);  //返回文件的错误状态，没有错误返回0
      //如果有错误，将错误信息输出到标准错误流stderr
      if (err) fprintf(stderr, "AppendFile::append() failed !\n");
      //最好调用clearerr(fp_)，清除错误标志，否则后续的ferror调用可能会持续返回错误状态，即使是后续操作实际上并没有错误
      break;
    }
    n += x;
    remain = len - n;
  }
}

void AppendFile::flush() { fflush(fp_); }

size_t AppendFile::write(const char* logline, size_t len) {
  return fwrite_unlocked(logline, 1, len, fp_);
}