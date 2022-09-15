
#define BOOST_TEST_MODULE CCDB
#define BOOST_TEST_MAIN
#define BOOST_TEST_DYN_LINK

#include <CCDB/CCDBDownloader.h>
#include <curl/curl.h>
#include "testCcdbDownloaderResources.h"
#include <chrono>   // time measurement
#include <iostream>

#include <CCDB/CcdbApi.h>

#include <boost/test/unit_test.hpp>

#include <boost/property_tree/json_parser.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/foreach.hpp>
#include <boost/optional/optional.hpp>


using namespace std;

namespace o2{
namespace ccdb{

bool aliceServer = false;
CcdbApi *api;

void setHandleOptionsForValidity(CURL* handle, std::string* dst, std::string* url, std::string* etag, CCDBDownloader* AD)
{
  curl_easy_setopt(handle, CURLOPT_WRITEFUNCTION, writeToString);
  curl_easy_setopt(handle, CURLOPT_WRITEDATA, dst);
  curl_easy_setopt(handle, CURLOPT_URL, url->c_str());

  if (aliceServer)
    api->curlSetSSLOptions(handle);

  std::string etagHeader = "If-None-Match: \"" + *etag + "\"";
  struct curl_slist *curlHeaders = nullptr;
  curlHeaders = curl_slist_append(curlHeaders, etagHeader.c_str());
  curl_easy_setopt(handle, CURLOPT_HTTPHEADER, curlHeaders);
}

std::vector<std::string> createPathsFromCS()
{
  std::vector<std::string> vec;

  std::string* pathsCS;
  if (aliceServer) {
    pathsCS = &alicePathsCS;
  } else {
    pathsCS = &ccdbPathsCS;
  }

  std::string tmp;
  for(int i = 0; i < pathsCS->size(); i++) {
    if ((*pathsCS)[i] == ',') {
      vec.push_back(tmp);
      tmp = "";
    } else {
      tmp += (*pathsCS)[i];
    }
  }
  vec.push_back(tmp);
  return vec;
}

std::vector<std::string> createEtagsFromCS()
{
  std::vector<std::string> vec;

  std::string* etagsCS;
  if (aliceServer) {
    etagsCS = &aliceEtagsCS;
  } else {
    etagsCS = &ccdbEtagsCS;
  }

  std::string tmp;
  for(int i = 0; i < etagsCS->size(); i++) {
    if ((*etagsCS)[i] == ',') {
      vec.push_back(tmp);
      tmp = "";
    } else {
      tmp += (*etagsCS)[i];
    }
  }
  vec.push_back(tmp);
  return vec;
}


int64_t blockingBatchTestSockets(int pathLimit = 0, bool printResult = false)
{
  // Preparing for downloading
  auto paths = createPathsFromCS();
  std::vector<std::string*> results;
  std::vector<std::string*> headers;
  std::unordered_map<std::string, std::string> urlETagMap;
  
  CCDBDownloader AD;

  std::vector<CURL*> handles;
  for (int i = 0; i < (pathLimit == 0 ? paths.size() : pathLimit); i++) {
    handles.push_back(curl_easy_init());
    results.push_back(new std::string());
    headers.push_back(new std::string());
    setHandleOptions(handles.back(), results.back(), headers.back(), &paths[i], &AD);
  }

  // Downloading objects
  std::cout << "Starting first batch\n";
  AD.batchBlockingPerform(handles);
  cleanAllHandles(handles);
  std::cout << "First batch completed\n";


  // Waiting for sockets to close
  sleep(5);

  std::vector<CURL*> handles2;
  for (int i = 0; i < (pathLimit == 0 ? paths.size() : pathLimit); i++) {
    handles2.push_back(curl_easy_init());
    results.push_back(new std::string());
    headers.push_back(new std::string());
    setHandleOptions(handles2.back(), results.back(), headers.back(), &paths[i], &AD);
  }

  // Downloading again
  std::cout << "Starting second batch\n";
  AD.batchBlockingPerform(handles2);
  cleanAllHandles(handles2);
  std::cout << "Second batch completed\n";

  return 0;
}

int64_t blockingBatchTest(int pathLimit = 0, bool printResult = false)
{
  // Preparing for downloading
  auto paths = createPathsFromCS();
  std::vector<std::string*> results;
  std::vector<std::string*> headers;
  std::unordered_map<std::string, std::string> urlETagMap;
  
  CCDBDownloader AD;

  auto start = std::chrono::system_clock::now();
  std::vector<CURL*> handles;
  for (int i = 0; i < (pathLimit == 0 ? paths.size() : pathLimit); i++) {
    handles.push_back(curl_easy_init());
    results.push_back(new std::string());
    headers.push_back(new std::string());
    setHandleOptions(handles.back(), results.back(), headers.back(), &paths[i], &AD);
  }

  // Downloading objects
  AD.batchBlockingPerform(handles);

  cleanAllHandles(handles);
  auto end = std::chrono::system_clock::now();
  auto difference = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
  if (printResult)
    std::cout << "BLOCKING BATCH TEST:  Download - " <<  difference << "ms.\n";

  // Extracting etags
  for (int i = 0; i < (pathLimit == 0 ? paths.size() : pathLimit); i++) {
    urlETagMap[paths[i]] = extractETAG(*headers[i]);
  }
  return difference;
}

int64_t blockingBatchTestValidity(int pathLimit = 0, bool printResult = false)
{
  // Preparing for checking objects validity
  auto paths = createPathsFromCS();
  auto etags = createEtagsFromCS();
  auto start = std::chrono::system_clock::now();

  CCDBDownloader AD;

  std::vector<std::string*> results;
  std::vector<CURL*> handles;
  for (int i = 0; i < (pathLimit == 0 ? paths.size() : pathLimit); i++) {
    auto handle = curl_easy_init();
    results.push_back(new std::string());
    handles.push_back(handle);
    setHandleOptionsForValidity(handle, results.back(), &paths[i], &etags[i], &AD);
  }

  // Checking objects validity
  AD.batchBlockingPerform(handles);

  for (int i = 0; i < (pathLimit == 0 ? paths.size() : pathLimit); i++) {
    long code;
    curl_easy_getinfo(handles[i], CURLINFO_RESPONSE_CODE, &code);
    if (code != 304) {
      char* url;
      curl_easy_getinfo(handles[i], CURLINFO_EFFECTIVE_URL, &url);
      std::cout << "INVALID CODE: " << code << ", URL: " << url << "\n";
    }
  }

  // Clean handles and print time
  cleanAllHandles(handles);
  auto end = std::chrono::system_clock::now();
  auto difference = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
  if (printResult)
    std::cout << "BLOCKING BATCH TEST:  Check validity - " <<  difference << "ms.\n";
  return difference;
}

int64_t asynchBatchTest(int pathLimit = 0, bool printResult = false)
{
  // Preparing urls and objects to write into
  auto paths = createPathsFromCS();
  std::vector<std::string*> results;
  std::vector<std::string*> headers;
  std::unordered_map<std::string, std::string> urlETagMap;
  
  // Preparing downloader
  CCDBDownloader AD;

  auto start = std::chrono::system_clock::now();

  // Preparing handles
  std::vector<CURL*> handles;
  for (int i = 0; i < (pathLimit == 0 ? paths.size() : pathLimit); i++) {
    handles.push_back(curl_easy_init());
    results.push_back(new std::string());
    headers.push_back(new std::string());
    setHandleOptions(handles.back(), results.back(), headers.back(), &paths[i], &AD);
  }

  // Performing requests
  bool requestFinished = false;
  AD.batchAsynchPerform(handles, &requestFinished);
  while (!requestFinished) sleep(0.05);

  // Cleanup and print time
  cleanAllHandles(handles);
  auto end = std::chrono::system_clock::now();
  auto difference = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
  if (printResult)
    std::cout << "ASYNC BATCH TEST:     download - " << difference << "ms.\n";

  // Extracting etags
  for (int i = 0; i < (pathLimit == 0 ? paths.size() : pathLimit); i++) {
    urlETagMap[paths[i]] = extractETAG(*headers[i]);
  }
  return difference;
}

int64_t asynchBatchTestValidity(int pathLimit = 0, bool printResult = false)
{
  // Preparing for checking objects validity
  auto paths = createPathsFromCS();
  auto etags = createEtagsFromCS();
  auto start = std::chrono::system_clock::now();
  CCDBDownloader AD;

  std::vector<std::string*> results;
  std::vector<CURL*> handles;
  for (int i = 0; i < (pathLimit == 0 ? paths.size() : pathLimit); i++) {
    auto handle = curl_easy_init();
    results.push_back(new std::string());
    handles.push_back(handle);
    setHandleOptionsForValidity(handle, results.back(), &paths[i], &etags[i], &AD);
  }

  // Checking objects validity
  bool requestFinished = false;
  AD.batchAsynchPerform(handles, &requestFinished);
  while (!requestFinished) sleep(0.001);
  cleanAllHandles(handles);

  // Checking if objects did not change
  for (int i = 0; i < (pathLimit == 0 ? paths.size() : pathLimit); i++) {
    long code;
    curl_easy_getinfo(handles[i], CURLINFO_RESPONSE_CODE, &code);
    if (code != 304) {
      std::cout << "INVALID CODE: " << code << "\n";
    }
  }

  auto end = std::chrono::system_clock::now();
  auto difference = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
  if (printResult)
    std::cout << "ASYNC BATCH TEST:     Check validity - " <<  difference << "ms.\n";
  return difference;
}

int64_t linearTest(int pathLimit = 0, bool printResult = false)
{
  auto paths = createPathsFromCS();
  std::vector<std::string*> results;
  std::vector<std::string*> headers;
  std::unordered_map<std::string, std::string> urlETagMap;

  auto start = std::chrono::system_clock::now();
  CURL *handle = curl_easy_init();
  curl_easy_setopt(handle, CURLOPT_WRITEFUNCTION, writeToString);

  for (int i = 0; i < (pathLimit == 0 ? paths.size() : pathLimit); i++) {
    results.push_back(new std::string());
    headers.push_back(new std::string());
    setHandleOptions(handle, results.back(), headers.back(), &paths[i], nullptr);
    curl_easy_perform(handle);

    long code;
    curl_easy_getinfo(handle, CURLINFO_HTTP_CODE, &code);
    if (code != 303) 
      std::cout << "Result different that 303. Received code: " << code << "\n";
  }

  auto end = std::chrono::system_clock::now();
  auto difference = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

  curl_easy_cleanup(handle);
  
  // Extracting etags
  for (int i = 0; i < (pathLimit == 0 ? paths.size() : pathLimit); i++) {
    urlETagMap[paths[i]] = extractETAG(*headers[i]);
  }
  if (printResult)
    std::cout << "LINEAR TEST:          download - " << difference << "ms.\n";
  return difference;
}

int64_t linearTestValidity(int pathLimit = 0, bool printResult = false)
{
  // Preparing for checking objects validity
  auto paths = createPathsFromCS();
  auto etags = createEtagsFromCS();
  auto start = std::chrono::system_clock::now();

  std::vector<std::string*> results;
  CURL* handle = curl_easy_init();
  for (int i = 0; i < (pathLimit == 0 ? paths.size() : pathLimit); i++) {
    results.push_back(new std::string());    
    setHandleOptionsForValidity(handle, results.back(), &paths[i], &etags[i], nullptr);
    
    curl_easy_perform(handle);
    long code;
    curl_easy_getinfo(handle, CURLINFO_RESPONSE_CODE, &code);
    if (code != 304) {
      std::cout << "INVALID CODE: " << code << "\n";
    }
  }

  curl_easy_cleanup(handle);
  auto end = std::chrono::system_clock::now();
  auto difference = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
  if (printResult)
    std::cout << "LINEAR TEST:          Check validity - " <<  difference << "ms.\n";
  return difference;
}

int64_t linearTestNoReuse(int pathLimit = 0, bool printResult = false)
{
  auto paths = createPathsFromCS();
  std::vector<std::string*> results;
  std::vector<std::string*> headers;
  std::unordered_map<std::string, std::string> urlETagMap;

  auto start = std::chrono::system_clock::now();

  for (int i = 0; i < (pathLimit == 0 ? paths.size() : pathLimit); i++) {
    CURL* handle = curl_easy_init();
    results.push_back(new std::string());
    headers.push_back(new std::string());
    setHandleOptions(handle, results.back(), headers.back(), &paths[i], nullptr);
    curl_easy_perform(handle);

    long code;
    curl_easy_getinfo(handle, CURLINFO_HTTP_CODE, &code);
    if (code != 303) 
      std::cout << "Result different that 303. Received code: " << code << "\n";

    curl_easy_cleanup(handle);
  }
  auto end = std::chrono::system_clock::now();
  auto difference = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
  if (printResult)
    std::cout << "LINEAR TEST no reuse:      download - " <<  difference << "ms.\n";

  // Extracting etags
  for (int i = 0; i < (pathLimit == 0 ? paths.size() : pathLimit); i++) {
    urlETagMap[paths[i]] = extractETAG(*headers[i]);
  }
  return difference;
}

int64_t linearTestNoReuseValidity(int pathLimit = 0, bool printResult = false)
{
  // Preparing for checking objects validity
  auto paths = createPathsFromCS();
  auto etags = createEtagsFromCS();
  auto start = std::chrono::system_clock::now();

  std::vector<std::string*> results;
  for (int i = 0; i < (pathLimit == 0 ? paths.size() : pathLimit); i++) {
    CURL* handle = curl_easy_init();
    results.push_back(new std::string());
    setHandleOptionsForValidity(handle, results.back(), &paths[i], &etags[i], nullptr);
    
    curl_easy_perform(handle);
    long code;
    curl_easy_getinfo(handle, CURLINFO_RESPONSE_CODE, &code);
    if (code != 304) {
      std::cout << "INVALID CODE: " << code << "\n";
    }
    curl_easy_cleanup(handle);
  }

  auto end = std::chrono::system_clock::now();
  auto difference = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
  if (printResult)
    std::cout << "LINEAR NO REUSE TEST: Check validity - " <<  difference << "ms.\n";
  return difference;
}

int64_t countAverageTime(int64_t (*function)(int, bool), int arg, int repeats)
{
  int64_t sum = 0;
  for(int i = 0; i < repeats; i++) {
    sum += function(arg, false);
  }
  return sum / repeats;
}

void smallTest()
{
  std::cout << "I work!\n";
}

//Michal test
BOOST_AUTO_TEST_CASE(small_test)
{
  smallTest();
  CCDBDownloader CCDBD;
  CCDBD.hello();
  // CCDBD.smallTest();
  // std::cout << "I worked\n";
  BOOST_CHECK(1 == 2);
}
//Michal test stop



BOOST_AUTO_TEST_CASE(download_benchmark)
{
  std::cout << "Test will be conducted on ";
  if (aliceServer) {
    std::cout << "https://alice-ccdb.cern.ch\n";
    api = new CcdbApi();
    api->init("https://alice-ccdb.cern.ch");
  } else {
    std::cout << "http://ccdb-test.cern.ch:8080\n";
  }

  if (curl_global_init(CURL_GLOBAL_ALL))
  {
    fprintf(stderr, "Could not init curl\n");
    return;
  }

  int testSize = 100; // max 185

  if (testSize != 0)
    std::cout << "-------------- Testing for " << testSize << " objects with " << CCDBDownloader::maxHandlesInUse << " parallel connections. -----------\n";
  else
    std::cout << "-------------- Testing for all objects with " << CCDBDownloader::maxHandlesInUse << " parallel connections. -----------\n";

  int repeats = 1;

  // Just checking for 303
  std::cout << "Benchmarking redirect time\n";
  std::cout << "Blocking perform: " << countAverageTime(blockingBatchTest, testSize, repeats) << "ms.\n";
  std::cout << "Async    perform: " << countAverageTime(asynchBatchTest, testSize, repeats) << "ms.\n";
  std::cout << "Single   handle : " << countAverageTime(linearTest, testSize, repeats) << "ms.\n";
  std::cout << "Single no reuse : " << countAverageTime(linearTestNoReuse, testSize, repeats) << "ms.\n";


  // std::cout << "--------------------------------------------------------------------------------------------\n";

  // std::cout << "Benchmarking test validity times\n";
  std::cout << "Blocking perform validity: " << countAverageTime(blockingBatchTestValidity, testSize, repeats) << "ms.\n";
  std::cout << "Async    perform validity: " << countAverageTime(asynchBatchTestValidity, testSize, repeats) << "ms.\n";
  std::cout << "Single   handle  validity: " << countAverageTime(linearTestValidity, testSize, repeats) << "ms.\n";
  std::cout << "Single no reuse  validity: " << countAverageTime(linearTestNoReuseValidity, testSize, repeats) << "ms.\n";

  // int64_t r = blockingBatchTestSockets(testSize, false);


  if(aliceServer) {
    delete api;
  }
  curl_global_cleanup();
}

}
}