#include <CCDB/CURLHandleManager.h>

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

} // namespace ccdb
} // namespace o2
