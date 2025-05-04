#ifndef GOOGLETEST_SAMPLES_asyncLogging_H_
#define GOOGLETEST_SAMPLES_asyncLogging_H_
#include <functional>
#include <gtest/gtest.h>
#include "/home/zhoufeiwei/Desktop/ZFWTinyMuduo/base/AsyncLogging.h"
#include "/home/zhoufeiwei/Desktop/ZFWTinyMuduo/net/Buffer.h"
#include <string>
#include <thread> // std::this_thread::sleep_for
#include <fstream>
#include <iostream>
#include <iterator>
#include <memory>
#include <vector>

/**
 * AsyncLogging
 *
 * std::this_thread::sleep_for 用于让当前线程暂停执行指定的时间
 * std::string::npos 是一个特殊的常量，表示“无效位置”或“未找到”
 */

class AsyncLoggerTest : public ::testing::Test
{
protected:
  void SetUp() override
  {
    logFilePath = "ascLog.txt";
    rollSize = 100 * 1024 * 1024;
    flush = 3;
    logger = std::make_unique<zfwmuduo::AsyncLogging>(logFilePath, rollSize, flush);
    logger->start();
  }

  std::unique_ptr<zfwmuduo::AsyncLogging> logger;
  std::string logFilePath;
  off_t rollSize;
  int flush;
};

//===================== 测试开始 =====================
// 单线程写入日志, 并关闭
TEST_F(AsyncLoggerTest, SingleThreadLog)
{
  const char *logMessage = "this is a test log message for testing AsyncLogging in single thread.";
  logger->append(logMessage, strlen(logMessage));
  std::this_thread::sleep_for(std::chrono::milliseconds(100)); // 等待日志写入完成

  std::ifstream logFile(logFilePath); // 打开日志文件
  // 读取日志文件内容
  std::string content((std::istream_iterator<char>(logFile)), std::istream_iterator<char>());
  logFile.close();

  std::cout << "Log file content:\n"
            << content << std::endl;

  EXPECT_NE(std::string::npos, content.find(logMessage));

  logger->stop();
}

// // 多线程写入日志, 并关闭
// TEST_F(AsyncLoggerTest, MultiThreadLog)
// {
//   const int numThreads = 10;
//   const int numMessages = 100;
//   std::vector<std::thread> threads;

//   for (int i = 0; i < numThreads; ++i)
//   {
//     threads.emplace_back(
//         [this, i, numMessages]
//         {
//           for (int j = 0; j < numMessages; ++j)
//           {
//             char logMessage[100];
//             snprintf(logMessage, sizeof(logMessage), "this is %d test log message for testing AsyncLogging in single thread %d.", i, j);
//             logger->append(logMessage, strlen(logMessage));
//           }
//         });
//   }

//   for (auto &thread : threads)
//     thread.join();

//   std::this_thread::sleep_for(std::chrono::milliseconds(200)); // 等待日志写入完成

//   std::ifstream logFile(logFilePath); // 打开日志文件
//   // 读取日志文件内容
//   std::string content((std::istream_iterator<char>(logFile)), std::istream_iterator<char>());

//   for (int i = 0; i < numThreads; ++i)
//   {
//     for (int j = 0; j < numMessages; ++j)
//     {
//       std::string exceptMessage = "this is " + std::to_string(i) + " test log message for testing AsyncLogging in single thread " + std::to_string(j) + ".";
//       EXPECT_NE(std::string::npos, content.find(exceptMessage));
//     }
//   }

//   logger->stop();
// }

int main(int argc, char **argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}

#endif // GOOGLETEST_SAMPLES_asyncLogging_H_