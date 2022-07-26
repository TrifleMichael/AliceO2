#include <CCDB/CURLHandleManager.h>
#include <iostream>

namespace o2
{
namespace ccdb
{

  CURLHandleManager::CURLHandleManager()
  {

  }

  CURL* CURLHandleManager::getHandle()
  {
    return curlHandle;
  }


  void CURLHandleManager::sleepAndDelete()
  {
    
  }

  void CURLHandleManager::testMultithread()
  {
    deleterThread = new std::thread(&CURLHandleManager::testFunction, this, "Banana");
    std::cout << "\nOrange\n";
    deleterThread->join();
  }

  void CURLHandleManager::testFunction(std::string word)
  {
    std::cout << "\n" << word << "\n";
  }

} // namespace ccdb
} // namespace o2
