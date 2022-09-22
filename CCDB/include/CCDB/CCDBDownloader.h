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
    CCDBDownloader *CD = nullptr;
  } curl_context_t;

  typedef struct DataForSocket
  {
    CCDBDownloader *CD;
    CURLM *curlm;
  } DataForSocket;

  typedef struct DataForClosingSocket
  {
    CCDBDownloader* CD;
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
    size_t *requestsLeft;
    RequestType type;
  } PerformData;

  CCDBDownloader();
  ~CCDBDownloader();


// Called by CURL in order to open a new socket. Newly opened sockets are assigned a timeout timer and added to socketTimerMap.
static curl_socket_t opensocket_callback(void* clientp, curlsocktype purpose, struct curl_sockaddr* address);


// Called by CURL in order to close a socket. It will be called by CURL even if a timout timer closed the socket beforehand.
static void closesocket_callback(void* clientp, curl_socket_t item);

// TODO: Remove or move to tests
static void cleanAllHandles(std::vector<CURL*> handles);


// Is used to react to polling file descriptors in poll_handle
// Calls handle_socket indirectly for further reading*
// If call is finished closes handle indirectly by check multi info
static void curlPerform(uv_poll_t *req, int status, int events);

// TODO: Rename
static void checkGlobals(uv_timer_t *handle);

// Used by CURL to react to action happening on a socket.
static int handleSocket(CURL *easy, curl_socket_t s, int action, void *userp, void *socketp);

static void onUVClose(uv_handle_t* handle);

static void closeHandles(uv_handle_t* handle, void* arg);

static void closePolls(uv_handle_t* handle, void* arg);

static void asyncUVHandleCallback(uv_async_t *handle);

static void closeHandleTimerCallback(uv_timer_t* handle);

static void onTimeout(uv_timer_t *req);













  /**
   * Creates a new multi_handle for the downloader
   */
  void initializeMultiHandle();

  /**
   * Releases resources reserver for the transfer, marks transfer as complete, passes the CURLcode to the destination and launches callbacks if requested
   * 
   * @param handle The easy_handle for which the transfer completed
   * @param curlCode The code produced for the handle by the transfer
   */
  void transferFinished(CURL* handle, CURLcode curlCode);

  /**
   * Creates structure holding information about a socket including a poll handle assigned to it
   * 
   * @param socketfd File descriptor of socket for which the structure will be created
   */
  curl_context_t *createCurlContext(curl_socket_t sockfd);

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
  std::vector<CURLcode> *asynchBatchPerformWithCallback(std::vector<CURL*> handle, bool *completionFlag, void (*cbFun)(void*), void* cbData);

  // Perform on a batch of handles in a blocking manner.
  std::vector<CURLcode> batchBlockingPerform(std::vector<CURL*> handleVector);

  // Perform on a batch of handles. Completion flag will be set to true when all handles finish their transfers.
  std::vector<CURLcode>* batchAsynchPerform(std::vector<CURL*> handleVector, bool *completionFlag);

};

void setHandleOptions(CURL* handle, std::string* dst, std::string* headers, std::string* path);

} // namespace ccdb
} // namespace o2

#endif
