#include <CCDB/CURLHandleManager.h>
#include <iostream>
#include <chrono>
#include <unistd.h>     //for using the function sleep

namespace o2
{
namespace ccdb
{

  CURLHandleManager::CURLHandleManager(CURL* handle)
  {
    std::cout << "\nCreating manager\n";
    if (handle != NULL)
    {
      curlHandle = handle;
      curl_easy_setopt(curlHandle, CURLOPT_NOSIGNAL, 1); // CURL signals are not thread safe
      extendValidity(secondsOfBuffer);
      deleterThread = new std::thread(&CURLHandleManager::sleepAndDelete, this);
    }
  }

  CURLHandleManager::~CURLHandleManager()
  {
    deleterThread->join();
    delete deleterThread;
    curl_easy_cleanup(curlHandle); // cleanup is null safe

    std::cout << "\n Destroying manager\n";
  }

  void CURLHandleManager::sleepAndDelete()
  {
    double secondsDiff = secondsUntilClosingHandle();
    do {
      sleep(secondsDiff);
      secondsDiff = secondsUntilClosingHandle();
    } while (secondsDiff > 0);

    curl_easy_cleanup(curlHandle);
    std::cout << "\nClosing handle!\n";
  }

  double CURLHandleManager::secondsUntilClosingHandle()
  {
    auto chronoDiff = expectedEndTime - std::chrono::steady_clock::now();
    return std::chrono::duration<double>(chronoDiff).count();
  }

  void CURLHandleManager::extendValidityDefault()
  {
    extendValidity(secondsOfBuffer);
  }

  void CURLHandleManager::extendValidity(size_t seconds)
  {
    std::cout << "\n Extending validity\n";
    expectedEndTime = std::chrono::steady_clock::now() + std::chrono::seconds(seconds);
  }

} // namespace ccdb
} // namespace o2
