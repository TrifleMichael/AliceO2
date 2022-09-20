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

// Called by CURL in order to open a new socket. Newly opened sockets are assigned a timeout timer and added to socketTimerMap.
curl_socket_t opensocket_callback(void* clientp, curlsocktype purpose, struct curl_sockaddr* address);


// Called by CURL in order to close a socket. It will be called by CURL even if a timout timer closed the socket beforehand.
void closesocket_callback(void* clientp, curl_socket_t item);

// TODO: Remove or move to tests
void cleanAllHandles(std::vector<CURL*> handles);


// Is used to react to polling file descriptors in poll_handle
// Calls handle_socket indirectly for further reading*
// If call is finished closes handle indirectly by check multi info
void curl_perform(uv_poll_t *req, int status, int events);

// TODO: Remove or move to tests
size_t writeToString(void *contents, size_t size, size_t nmemb, std::string *dst);

// TODO: Remove or move to tests
std::string extractETAG(std::string headers);

// TODO: Rename
void checkGlobals(uv_timer_t *handle);

// Used by CURL to react to action happening on a socket.
int handleSocket(CURL *easy, curl_socket_t s, int action, void *userp, void *socketp);

class CCDBDownloader
{
public:

  int socketTimoutMS = 5000; // Time for which sockets will stay open after last download finishes

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

  CCDBDownloader();
  ~CCDBDownloader();

  // Creates a new multi-handle
  void initializeMultiHandle();

  // Removes easy_handle from multi_handle, makes callbacks, releases locks for blocking dowloands etc.
  void finalizeDownload(CURL* handle);

  // Creates structure holding information about a socket including a poll handle assigned to it
  curl_context_t *createCurlContext(curl_socket_t sockfd, CCDBDownloader *objPtr);

  // Is called when handle is closed. Frees data stored within it.
  static void curlCloseCB(uv_handle_t *handle);

  // Destroyes data about a socket
  static void destroyCurlContext(curl_context_t *context);

  // Checks messages inside curl multi handle
  void checkMultiInfo(void);

  // Connects curl timer with uv timer
  static int startTimeout(CURLM *multi, long timeout_ms, void *userp);

  // Asynchroniously signalls the event loop to check for new easy_handles to add to multi_handle
  void makeLoopCheckQueueAsync();

  // If multi_handles uses less then maximum number of handles then add handles from the queue.
  void checkHandleQueue();

  // Start the event loop.
  void asynchLoop();

  // Perform on a single handle in a blocking manner.
  CURLcode perform(CURL* handle);

  // Perform on a batch of handles. Callback will be exectuted in it's own thread after all handles finish their transfers.
  CURLcode *asynchPerformWithCallback(CURL* handle, bool *completionFlag, void (*cbFun)(void*), void* cbData);

  // Perform on a batch of handles in a blocking manner.
  std::vector<CURLcode> batchBlockingPerform(std::vector<CURL*> handleVector);

  // Perform on a batch of handles. Completion flag will be set to true when all handles finish their transfers.
  std::vector<CURLcode>* batchAsynchPerform(std::vector<CURL*> handleVector, bool *completionFlag);

};

void setHandleOptions(CURL* handle, std::string* dst, std::string* headers, std::string* path);

} // namespace ccdb
} // namespace o2

#endif
