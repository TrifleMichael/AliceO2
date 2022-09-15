#include <stdio.h>
#include <stdlib.h>
#include <uv.h>
#include <curl/curl.h>
#include <string>
#include <vector>
#include <iostream>
#include <thread>     // get_id
#include <mutex>
#include <condition_variable>
#include <unordered_map>

#include <chrono>   // time measurement
#include <unistd.h> // time measurement

#ifndef ALICEO2_CCDBDOWNLOADER_H
#define ALICEO2_CCDBDOWNLOADER_H

using namespace std;

namespace o2
{
namespace ccdb
{

curl_socket_t opensocket_callback(void* clientp, curlsocktype purpose, struct curl_sockaddr* address);
void closesocket_callback(void* clientp, curl_socket_t item);
void cleanAllHandles(std::vector<CURL*> handles);

void curl_perform(uv_poll_t *req, int status, int events);
size_t writeToString(void *contents, size_t size, size_t nmemb, std::string *dst);
std::string extractETAG(std::string headers);
void checkGlobals(uv_timer_t *handle);

int handleSocket(CURL *easy, curl_socket_t s, int action, void *userp, void *socketp);

class CCDBDownloader
{
public:

  int socketTimoutMS = 20000; // Time for which sockets will stay open after last download finishes

  enum RequestType
  {
    BLOCKING,
    ASYNCHRONOUS,
    ASYNCHRONOUS_WITH_CALLBACK
  };

  typedef struct curl_context_s
  {
    uv_poll_t poll_handle;
    curl_socket_t sockfd = -1;
    CCDBDownloader *objPtr = nullptr;
  } curl_context_t;

  typedef struct DataForSocket
  {
    CCDBDownloader *objPtr;
    CURLM *curlm;
  } DataForSocket;

  typedef struct DataForClosingSocket
  {
    CCDBDownloader* AD;
    curl_socket_t socket;
  } DataForClosingSocket;

  int handlesInUse = 0;
  static int const maxHandlesInUse = 5; // static and constant just for testing
  bool multiHandleActive = false;

  uv_timer_t* socketTimoutTimer;
  bool socketTimoutTimerRunning = false;
  std::unordered_map<curl_socket_t, uv_timer_t*> socketTimerMap;

  bool closeLoop = false;
  uv_loop_t loop;
  CURLM *curlMultiHandle = nullptr;
  uv_timer_t *timeout;
  std::vector<CURL*> handlesToBeAdded;
  std::mutex handlesQueueLock;
  std::thread *loopThread;
  std::vector<std::pair<std::thread*, bool*> > threadFlagPairVector;

  typedef struct PerformData
  {
    bool asynchronous;
    std::condition_variable *cv;
    bool *completionFlag;
    CURLcode *codeDestination;
    void (*cbFun)(void*);
    std::thread *cbThread;
    void *cbData;
    bool callback = false;
    bool batchRequest = false;
    size_t *requestsLeft;
    RequestType type;
  } PerformData;

  void smallTest();

  CCDBDownloader();
  ~CCDBDownloader();
  void initializeMultiHandle();
  void finalizeDownload(CURL* handle);
  curl_context_t *createCurlContext(curl_socket_t sockfd, CCDBDownloader *objPtr);
  static void curlCloseCB(uv_handle_t *handle);
  static void destroyCurlContext(curl_context_t *context);
  void checkMultiInfo(void);
  static int startTimeout(CURLM *multi, long timeout_ms, void *userp);
  void makeLoopCheckQueueAsync();
  void checkHandleQueue();
  void asynchLoop();
  bool init();
  CURLcode blockingPerform(CURL* handle);
  CURLcode *asynchPerformWithCallback(CURL* handle, bool *completionFlag, void (*cbFun)(void*), void* cbData);
  std::vector<CURLcode> batchBlockingPerform(std::vector<CURL*> handleVector);
  std::vector<CURLcode>* batchAsynchPerform(std::vector<CURL*> handleVector, bool *completionFlag);

};

void setHandleOptions(CURL* handle, std::string* dst, std::string* headers, std::string* path, CCDBDownloader* AD);

} // namespace ccdb
} // namespace o2

#endif
