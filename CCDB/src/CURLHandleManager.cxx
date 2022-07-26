#include <CCDB/CURLHandleManager.h>
#include <iostream>
#include <chrono>
#include <unistd.h>     //for using the function sleep
#include <stdlib.h>     //for using the function sleep

namespace o2
{
namespace ccdb
{

  CURLHandleManager::CURLHandleManager()
  {
    // if (handle != NULL)
    // {
    //   curlHandle = handle;
    //   curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1); // CURL signals are not thread safe
    //   startTime = std::chrono::steady_clock::now();
    //   expectedEndTime = startTime + bufferTime;
    // }

    // Testing on null handle
    expectedEndTime = std::chrono::steady_clock::now() + std::chrono::seconds(secondsOfBuffer);
    deleterThread = new std::thread(&CURLHandleManager::sleepAndDelete, this);
    deleterThread->join();    
  }

  CURL* CURLHandleManager::getHandle()
  {
    return curlHandle;
  }

  void CURLHandleManager::sleepAndDelete()
  {
    auto chronoDiff = expectedEndTime - std::chrono::steady_clock::now();
    double secondsDiff = std::chrono::duration<double>(chronoDiff).count();
    std::cout << "\nThis should be around 10: " << secondsDiff << "\n";
    //sleep(secondsDiff);
  }


} // namespace ccdb
} // namespace o2
