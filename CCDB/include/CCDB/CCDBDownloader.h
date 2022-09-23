#include <stdio.h>
#include <stdlib.h>
#include <uv.h>
#include <curl/curl.h>
#include <string>
#include <vector>
#include <iostream>
#include <thread> // get_id
#include <mutex>
#include <condition_variable>
#include <unordered_map>

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
  int socketTimoutMS = 4000; // Time for which sockets will stay open after last download finishes
  int maxHandlesInUse = 3;   // Max number of handles that can be used at the same time

  CCDBDownloader();
  ~CCDBDownloader();

  /**
   * Perform on a single handle in a blocking manner. Has the same effect as curl_easy_perform().
   *
   * @param handle Handle to be performed on. It can be reused or cleaned after perform finishes.
   */
  CURLcode perform(CURL* handle);

  /**
   * Perform on a batch of handles. Callback will be exectuted in it's own thread after all handles finish their transfers.
   *
   * @param handles Handles to be performed on.
   */
  std::vector<CURLcode>* asynchBatchPerformWithCallback(std::vector<CURL*> handles, bool* completionFlag, void (*cbFun)(void*), void* cbData);

  /**
   * Perform on a batch of handles in a blocking manner. Has the same effect as calling curl_easy_perform() on all handles in the vector.
   * @param handleVector Handles to be performed on.
   */
  std::vector<CURLcode> batchBlockingPerform(std::vector<CURL*> handleVector);

  /**
   * Perform on a batch of handles. Completion flag will be set to true when all handles finish their transfers.
   * @param handleVector Handles to be performed on.
   * @param completionFlag Should be set to false before passing it to this function. Will be set to true after all transfers finish.
   */
  std::vector<CURLcode>* batchAsynchPerform(std::vector<CURL*> handleVector, bool* completionFlag);

  /**
   * Limits the number of parallel connections. Should be used only if no transfers are happening.
   */
  void setMaxParallelConnections(int limit);

  /**
   * Limits the time a socket and its connection will be opened after transfer finishes.
   */
  void setSocketTimoutTime(int timoutMS);

 private:

  int handlesInUse = 0;
  std::unordered_map<curl_socket_t, uv_timer_t*> socketTimerMap;
  uv_loop_t loop;
  CURLM* curlMultiHandle = nullptr;
  uv_timer_t* timeout;
  std::vector<CURL*> handlesToBeAdded;
  std::mutex handlesQueueLock;
  std::thread* loopThread;
  std::vector<std::pair<std::thread*, bool*>> threadFlagPairVector;
  bool closeLoop = false;

  enum RequestType {
    BLOCKING,
    ASYNCHRONOUS,
    ASYNCHRONOUS_WITH_CALLBACK
  };

  typedef struct curl_context_s {
    uv_poll_t poll_handle;
    curl_socket_t sockfd = -1;
    CCDBDownloader* CD = nullptr;
  } curl_context_t;

  typedef struct DataForSocket {
    CCDBDownloader* CD;
    CURLM* curlm;
  } DataForSocket;

  typedef struct DataForClosingSocket {
    CCDBDownloader* CD;
    curl_socket_t socket;
  } DataForClosingSocket;

  typedef struct PerformData {
    bool asynchronous;
    std::condition_variable* cv;
    bool* completionFlag;
    CURLcode* codeDestination;
    void (*cbFun)(void*);
    std::thread* cbThread;
    void* cbData;
    size_t* requestsLeft;
    RequestType type;
  } PerformData;

  /**
   * Called by CURL in order to open a new socket. Newly opened sockets are assigned a timeout timer and added to socketTimerMap.
   *
   * @param clientp Pointer to the CCDBDownloader instance which controls the socket.
   * @param purpose Purpose of opened socket. This parameter is unused but required by the callback template.
   * @param address Structure containing information about family, type and protocol for the socket.
   */
  static curl_socket_t opensocketCallback(void* clientp, curlsocktype purpose, struct curl_sockaddr* address);

  /**
   * Called by CURL in order to close a socket. It will be called by CURL even if a timout timer closed the socket beforehand.
   *
   * @param clientp Pointer to the CCDBDownloader instance which controls the socket.
   * @param item File descriptor of the socket.
   */
  static void closesocketCallback(void* clientp, curl_socket_t item);

  /**
   *  Is used to react to polling file descriptors in poll_handle.
   *
   * @param handle Handle assigned to this callback.
   * @param status Used to signal errors.
   * @param events Bitmask used to describe events on the socket.
   */
  static void curlPerform(uv_poll_t* handle, int status, int events);

  /**
   * Checks if loop was signalled to close. The handle connected with this callbacks is always active as to prevent the uv_loop from stopping.
   *
   * @param handle uv_handle to which this callbacks is assigned
   */
  static void checkStopSignal(uv_timer_t* handle);

  /**
   * Used by CURL to react to action happening on a socket.
   */
  static int handleSocket(CURL* easy, curl_socket_t s, int action, void* userp, void* socketp);

  /**
   * Deletes the handle.
   *
   * @param handle Handle assigned to this callback.
   */
  static void onUVClose(uv_handle_t* handle);

  /**
   * uv_walk callback which is used to close passed handle.
   *
   * @param handle Handle to be closed.
   * @param arg Argument required by callback template. Is not used in this implementation.
   */
  static void closeHandles(uv_handle_t* handle, void* arg);

  /**
   * Asynchroniously notifies the loop to check its CURL handle queue.
   *
   * @param handle Handle which is assigned to this callback.
   */
  static void asyncUVHandleCheckQueue(uv_async_t* handle);

  /**
   * Closes socket assigned to the timer handle.
   *
   * @param handle Handle which is assigned to this callback.
   */
  static void closeSocketByTimer(uv_timer_t* handle);

  /**
   * Starts new transfers.
   *
   * @param req Handle which is assigned to this callback.
   */
  static void curlTimeout(uv_timer_t* req);

  /**
   * Is called when a poll handle conencted to as socket is closed. Frees data stored within the handle.
   *
   * @param handle Handle assigned to this callback.
   */
  static void curlCloseCB(uv_handle_t* handle);

  /**
   * Closes poll handle assigned to the socket contained in the context and frees data within the handle.
   *
   * @param context Structure containing information about socket and handle to be closed.
   */
  static void destroyCurlContext(curl_context_t* context);

  /**
   * Connects curl timer with uv timer.
   *
   * @param multi Multi handle for which the timout will be set
   * @param timeout_ms Time until timeout
   * @param userp Pointer to the uv_timer_t handle that is used for timeout.
   */
  static int startTimeout(CURLM* multi, long timeout_ms, void* userp);

  /**
   * Checks if any of the callback threads have finished running and approprietly joins them.
   */
  void checkForThreadsToJoin();

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
   * Checks message queue inside curl multi handle.
   */
  void checkMultiInfo();

  /**
   * Sets openSocketCallback and closeSocketCallback with appropriate arguments. Stores data inside the CURL handle.
   */
  void setHandleOptions(CURL* handle, PerformData* data);

  /**
   * Creates structure holding information about a socket including a poll handle assigned to it
   *
   * @param socketfd File descriptor of socket for which the structure will be created
   */
  curl_context_t* createCurlContext(curl_socket_t sockfd);

  /**
   * Asynchroniously signals the event loop to check for new easy_handles to add to multi handle.
   */
  void makeLoopCheckQueueAsync();

  /**
   * If multi_handles uses less then maximum number of handles then add handles from the queue.
   */
  void checkHandleQueue();

  /**
   * Start the event loop. This function should be ran in the `loopThread`.
   */
  void runLoop();

};

} // namespace ccdb
} // namespace o2

#endif
