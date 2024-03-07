// @Author Lin Ya
// @Email xxbbb@vip.qq.com
#include "Logging.h"
#include "CurrentThread.h"
#include "Thread.h"
#include "AsyncLogging.h"
#include <assert.h>
#include <iostream>
#include <time.h>  
#include <sys/time.h> 


static pthread_once_t once_control_ = PTHREAD_ONCE_INIT;
static AsyncLogging *AsyncLogger_;

std::string Logger::logFileName_ = "./WebServer.log";

//初始化後端管理日誌的對象AsyncLogger_，初始化只執行一次
void once_init()
{
    AsyncLogger_ = new AsyncLogging(Logger::getLogFileName());
    AsyncLogger_->start(); 
}
//輸出後端
void output(const char* msg, int len)
{
    pthread_once(&once_control_, once_init);
    AsyncLogger_->append(msg, len);
}

Logger::Impl::Impl(const char *fileName, int line)
  : stream_(),
    line_(line),
    basename_(fileName)
{
    formatTime();   
}

//向日誌流對象中加入時間
void Logger::Impl::formatTime()
{
    struct timeval tv;  //存储时间的结构体
    time_t time;
    char str_t[26] = {0};
    gettimeofday (&tv, NULL);   //获取当前时间
    time = tv.tv_sec;   //取出当前时间，刻度为秒
    struct tm* p_time = localtime(&time);   //将UTC时间转换为本地时间
    //将本地时间转换为年月日、小时分钟秒的形式，存储到str_t字符数组中，最多可放26个字节
    strftime(str_t, 26, "%Y-%m-%d %H:%M:%S\n", p_time); 
    stream_ << str_t;   //放入LogStream中
}

Logger::Logger(const char *fileName, int line)
  : impl_(fileName, line)
{ }

Logger::~Logger()
{
    //日誌信息產生的文件、行號輸入到LogStream
    impl_.stream_ << " -- " << impl_.basename_ << ':' << impl_.line_ << '\n';   
    const LogStream::Buffer& buf(stream().buffer());    //獲取日誌流中存儲日誌信息的數組引用
    output(buf.data(), buf.length());
}