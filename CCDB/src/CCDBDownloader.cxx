
#include <CCDB/CCDBDownloader.h>
#include <iostream>
#include <string>
#include <curl/curl.h>

namespace o2
{
namespace ccdb
{

// CCDBDownloader::CCDBDownloader()
// {
//   std::cout << "Hemlo!\n";
// }

void CCDBDownloader::hello()
{
  std::cout << "Hello!\n";
}

size_t writeToString(void *contents, size_t size, size_t nmemb, std::string *dst)
{
  char *conts = (char *)contents;
  for (int i = 0; i < nmemb; i++)
  {
    (*dst) += *(conts++);
  }
  return size * nmemb;
}


void CCDBDownloader::testFunction()
{
  curl_global_init(CURL_GLOBAL_ALL);
  CURL* handle = curl_easy_init();

  std::string dst;
  curl_easy_setopt(handle, CURLOPT_URL, "http://alice-ccdb.cern.ch/TRD/Calib/DCSDPsU/1659949587465");
  curl_easy_setopt(handle, CURLOPT_WRITEFUNCTION, writeToString);
  curl_easy_setopt(handle, CURLOPT_WRITEDATA, &dst);

  CURLcode result = curl_easy_perform(handle);
  std::cout << "Result: " << result << "\n";
  curl_global_cleanup();

}

} // namespace ccdb
} // namespace o2