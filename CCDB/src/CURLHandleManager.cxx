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
    // should take in handle as argument

    // if (handle != NULL)
    // {
    //   curlHandle = handle;
    //   curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1); // CURL signals are not thread safe
    // }

    // Testing on null handle
    std::cout << "\nOpening handle\n";
    extendValidity(secondsOfBuffer);
    deleterThread = new std::thread(&CURLHandleManager::sleepAndDelete, this);
  }

  CURLHandleManager::~CURLHandleManager()
  {
    deleterThread->join();
    delete deleterThread;
    // close CURLHandle if not closed
  }

  CURL* CURLHandleManager::getHandle()
  {
    if (curlHandle != NULL)
    {
      extendValidity(secondsOfBuffer);
    }
    return curlHandle;
  }

  void CURLHandleManager::sleepAndDelete()
  {
    double secondsDiff = secondsUntilClosingHandle();
    do {
      sleep(secondsDiff);
      secondsDiff = secondsUntilClosingHandle();
    } while (secondsDiff > 0);

    std::cout << "\nClosing handle!\n";
    // close handle
  }

  double CURLHandleManager::secondsUntilClosingHandle()
  {
    auto chronoDiff = expectedEndTime - std::chrono::steady_clock::now();
    return std::chrono::duration<double>(chronoDiff).count();
  }

  void CURLHandleManager::extendValidity(size_t seconds)
  {
    expectedEndTime = std::chrono::steady_clock::now() + std::chrono::seconds(seconds);
  }

} // namespace ccdb
} // namespace o2
