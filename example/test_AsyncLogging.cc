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
    logFilePath = "ascLog";
    rollSize = 100 * 1024 * 1024;
    flush = 3;
    logger = std::make_unique<zfwmuduo::AsyncLogging>(logFilePath, rollSize, flush);
    logger->start();
  }

  void TearDown() override
  {
    logger->stop();
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
  const char *logMessage = "this is a test log message for testing AsyncLogging in single thread.\n";
  logger->append(logMessage, strlen(logMessage));
  std::this_thread::sleep_for(std::chrono::milliseconds(1000)); // 等待日志写入完成
}

// 多线程写入日志, 并关闭
TEST_F(AsyncLoggerTest, MultiThreadLog)
{
  const int numThreads = 5;
  const int numMessages = 2;
  std::vector<std::thread> threads;

  for (int i = 0; i < numThreads; ++i)
  {
    threads.emplace_back(
        [this, i, numMessages]
        {
          for (int j = 0; j < numMessages; ++j)
          {
            char logMessage[100];
            snprintf(logMessage, sizeof(logMessage), "this is %d test log message for testing AsyncLogging in single thread %d.\n", i, j);
            logger->append(logMessage, strlen(logMessage));
          }
        });
  }

  for (auto &thread : threads)
    thread.join();

  std::this_thread::sleep_for(std::chrono::milliseconds(1000));
}

bool isFileEmpty(const std::string &filePath)
{
  std::ifstream file(filePath);
  if (!file)
  {
    // 文件无法打开，返回false
    return false;
  }
  file.seekg(0, std::ios::end); // 移动到文件末尾
  return file.tellg() == 0;     // 如果文件大小为0，返回true
}

void printFileContent(const std::string &filePath)
{
  std::ifstream file(filePath);
  if (!file)
  {
    std::cerr << "Error: Unable to open file " << filePath << std::endl;
    return;
  }

  std::string line;
  while (std::getline(file, line))
  {
    std::cout << line << std::endl;
  }
}

TEST_F(AsyncLoggerTest, IsFileEmpty)
{
  EXPECT_FALSE(isFileEmpty(logFilePath + ".log")) << "The file should not be empty.";
  printFileContent(logFilePath + ".log");
}