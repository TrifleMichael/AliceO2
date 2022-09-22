#include <CCDB/CCDBDownloader.h>

#include <curl/curl.h>
#include <unordered_map>
#include <stdio.h>
#include <stdlib.h>
#include <uv.h>
#include <curl/curl.h>
#include <string>
#include <iostream>
#include <thread>     // get_id
#include <vector>
#include <condition_variable>
#include <mutex>

#include <chrono>   // time measurement
#include <unistd.h> // time measurement

#include <sys/types.h> /* See NOTES */
#include <sys/socket.h>

// THIS IS THE SSL TEST ! THIS IS THE SSL TEST ! THIS IS THE SSL TEST ! THIS IS THE SSL TEST !

/*
g++ -std=c++11 GigaTest.cpp -lpthread -lcurl -luv -o GigaTest && ./GigaTest
*/

// Ideas: call handleSocket for each socket to close them (mark with flag passed outside)

/*
TODO:

- add asynchronous closeLoop call

- check what can be free'd in destructor
- free things in finalize Download

- pooling threads only when they exist

- multiple uv loop threads

Information:

- Curl multi handle automatically reuses connections. Source: https://everything.curl.dev/libcurl/connectionreuse
- Keepalive for http is set to 118 seconds by default by CURL Source: https://stackoverflow.com/questions/60141625/libcurl-how-does-connection-keep-alive-work

*/

namespace o2 {
namespace ccdb {

CCDBDownloader::CCDBDownloader()
{
  // Preparing timer to be used by curl
  timeout = new uv_timer_t();
  timeout->data = this;
  uv_loop_init(&loop);
  uv_timer_init(&loop, timeout);

  // Preparing curl handle
  initializeMultiHandle();  

  // Global timer
  // uv_loop runs only when there are active handles, this handle guarantees the loop won't close immedietly after starting
  auto timerCheckQueueHandle = new uv_timer_t();
  timerCheckQueueHandle->data = this;
  uv_timer_init(&loop, timerCheckQueueHandle);
  uv_timer_start(timerCheckQueueHandle, checkStopSignal, 100, 100);

  loopThread = new std::thread(&CCDBDownloader::runLoop, this);
}

void CCDBDownloader::initializeMultiHandle()
{
  curlMultiHandle = curl_multi_init();
  curl_multi_setopt(curlMultiHandle, CURLMOPT_SOCKETFUNCTION, handleSocket);
  auto socketData = new DataForSocket();
  socketData->curlm = curlMultiHandle;
  socketData->CD = this;
  curl_multi_setopt(curlMultiHandle, CURLMOPT_SOCKETDATA, socketData);
  curl_multi_setopt(curlMultiHandle, CURLMOPT_TIMERFUNCTION, startTimeout);
  curl_multi_setopt(curlMultiHandle, CURLMOPT_TIMERDATA, timeout);
  curl_multi_setopt(curlMultiHandle, CURLMOPT_MAX_TOTAL_CONNECTIONS, maxHandlesInUse);

}

CCDBDownloader::~CCDBDownloader()
{
  for(auto socketTimerPair : socketTimerMap) {
    uv_timer_stop(socketTimerPair.second);
    uv_close((uv_handle_t*)socketTimerPair.second, onUVClose);
  }

  // Close async thread
  closeLoop = true;
  loopThread->join();

  delete loopThread;

  // Close and if any handles are running then signal to close, and run loop once to close them
  // This may take more then one iteration of loop - hence the "while"
  while (UV_EBUSY == uv_loop_close(&loop)) {
    closeLoop = false;
    uv_walk(&loop, closeHandles, NULL);
    uv_run(&loop, UV_RUN_ONCE);
  }
  curl_multi_cleanup(curlMultiHandle);
}

void CCDBDownloader::closeHandles(uv_handle_t* handle, void* arg)
{
  if (!uv_is_closing(handle))
    uv_close(handle, onUVClose);
}

void CCDBDownloader::onUVClose(uv_handle_t* handle)
{
  if (handle != NULL)
  {
    delete handle;
  }
}

// TODO: Rename
void CCDBDownloader::checkStopSignal(uv_timer_t *handle)
{
  // Check for closing signal
  auto CD = (CCDBDownloader*)handle->data;
  if(CD->closeLoop) {
    uv_timer_stop(handle);
    uv_stop(&CD->loop);
  }
  CD->checkForThreadsToJoin();
}

void CCDBDownloader::checkForThreadsToJoin() // TODO: MOVE DOWN
{
  // Join and erase threads that finished running callback functions
  for (int i = 0; i < threadFlagPairVector.size(); i++)
  {
    if (*(threadFlagPairVector[i].second))
    {
      threadFlagPairVector[i].first->join();
      delete (threadFlagPairVector[i].first);
      delete (threadFlagPairVector[i].second);
      threadFlagPairVector.erase(threadFlagPairVector.begin() + i);
    }
  }
}

void CCDBDownloader::closesocketCallback(void *clientp, curl_socket_t item)
{
  auto CD = (CCDBDownloader*)clientp;
  if (CD->socketTimerMap.find(item) != CD->socketTimerMap.end()) {
    uv_timer_stop(CD->socketTimerMap[item]);
    CD->socketTimerMap.erase(item);
    close(item);
  }
}

curl_socket_t CCDBDownloader::opensocketCallback(void *clientp, curlsocktype purpose, struct curl_sockaddr *address)
{
  auto CD = (CCDBDownloader*)clientp;
  auto sock = socket(address->family, address->socktype, address->protocol);

  CD->socketTimerMap[sock] = new uv_timer_t();
  uv_timer_init(&CD->loop, CD->socketTimerMap[sock]);

  auto data = new CCDBDownloader::DataForClosingSocket();
  data->CD = CD;
  data->socket = sock;
  CD->socketTimerMap[sock]->data = data;

  return sock;
}

void CCDBDownloader::asyncUVHandleCheckQueue(uv_async_t *handle)
{
  auto CD = (CCDBDownloader*)handle->data;
  uv_close((uv_handle_t*)handle, onUVClose);
  CD->checkHandleQueue();
}

// TODO: Change name
void CCDBDownloader::closeSocketByTimer(uv_timer_t* handle)
{
  auto data = (CCDBDownloader::DataForClosingSocket*)handle->data;
  auto CD = data->CD;
  auto sock = data->socket;

  if (CD->socketTimerMap.find(sock) != CD->socketTimerMap.end()) {
    uv_timer_stop(CD->socketTimerMap[sock]);
    CD->socketTimerMap.erase(sock);
    close(sock);
    return;
  }
}

// TODO: Rename
void CCDBDownloader::curlTimeout(uv_timer_t *handle)
{
  auto CD = (CCDBDownloader *)handle->data;
  int running_handles;
  curl_multi_socket_action(CD->curlMultiHandle, CURL_SOCKET_TIMEOUT, 0, &running_handles);
  CD->checkMultiInfo();
}

void CCDBDownloader::curlPerform(uv_poll_t *handle, int status, int events)
{
  int running_handles;
  int flags = 0;
  if (events & UV_READABLE)
    flags |= CURL_CSELECT_IN;
  if (events & UV_WRITABLE)
    flags |= CURL_CSELECT_OUT;

  auto context = (CCDBDownloader::curl_context_t *)handle->data;
  
  curl_multi_socket_action(context->CD->curlMultiHandle, context->sockfd, flags, &running_handles);
  context->CD->checkMultiInfo();
}

int CCDBDownloader::handleSocket(CURL *easy, curl_socket_t s, int action, void *userp, void *socketp)
{
  auto socketData = (CCDBDownloader::DataForSocket *)userp;
  auto CD = (CCDBDownloader*)socketData->CD;
  CCDBDownloader::curl_context_t *curl_context;
  int events = 0;

  switch (action)
  {
  case CURL_POLL_IN:
  case CURL_POLL_OUT:
  case CURL_POLL_INOUT:

    // Create context associated with socket and create a poll for said socket
    curl_context = socketp ? (CCDBDownloader::curl_context_t *)socketp : CD->createCurlContext(s);
    curl_multi_assign(socketData->curlm, s, (void *)curl_context);

    if (action != CURL_POLL_IN)
      events |= UV_WRITABLE;
    if (action != CURL_POLL_OUT)
      events |= UV_READABLE;

    if (CD->socketTimerMap.find(s) != CD->socketTimerMap.end()) {
      uv_timer_stop(CD->socketTimerMap[s]);
    }

    uv_poll_start(&curl_context->poll_handle, events, curlPerform);
    break;
  case CURL_POLL_REMOVE:
    if (socketp)
    {
      if (CD->socketTimerMap.find(s) != CD->socketTimerMap.end()) {
        uv_timer_start(CD->socketTimerMap[s], closeSocketByTimer, CD->socketTimoutMS, 0);
      }

      // Stop polling the socket, remove context assiciated with it. Socket will stay open until multi handle is closed.
      uv_poll_stop(&((CCDBDownloader::curl_context_t *)socketp)->poll_handle);
      CD->destroyCurlContext((CCDBDownloader::curl_context_t *)socketp);
      curl_multi_assign(socketData->curlm, s, NULL);
    }
    break;
  default:
    abort();
  }

  return 0;
}

void CCDBDownloader::setMaxParallelConnections(int limit)
{
  maxHandlesInUse = limit;
}

void CCDBDownloader::setSocketTimoutTime(int timoutMS)
{
  socketTimoutMS = timoutMS;
}

CCDBDownloader::curl_context_t *CCDBDownloader::createCurlContext(curl_socket_t sockfd)
{
  curl_context_t *context;

  context = (curl_context_t *)malloc(sizeof(*context));
  context->CD = this;
  context->sockfd = sockfd;

  uv_poll_init_socket(&loop, &context->poll_handle, sockfd);
  context->poll_handle.data = context;

  return context;
}

void CCDBDownloader::curlCloseCB(uv_handle_t *handle)
{
  curl_context_t *context = (curl_context_t *)handle->data;
  free(context);
}

void CCDBDownloader::destroyCurlContext(curl_context_t *context)
{
  uv_close((uv_handle_t *)&context->poll_handle, curlCloseCB);
}

void callbackWrappingFunction(void (*cbFun)(void*), void* data, bool* completionFlag)
{
  cbFun(data);
  *completionFlag = true;
}

void CCDBDownloader::transferFinished(CURL* easy_handle, CURLcode curlCode)
{
  handlesInUse--;
  PerformData *data;
  curl_easy_getinfo(easy_handle, CURLINFO_PRIVATE, &data);

  // Remove handle from multihandle
  curl_multi_remove_handle(curlMultiHandle, easy_handle);

  // Save curl code generated by curlPerform to destination
  *data->codeDestination = curlCode;

  // If no requests left then signal finished based on type of operation
  if (--(*data->requestsLeft) == 0)
  {
    switch (data->type)
    {
    case BLOCKING:
      data->cv->notify_all();
      break;
    case ASYNCHRONOUS:
      *data->completionFlag = true;
      break;
    case ASYNCHRONOUS_WITH_CALLBACK:
    // TODO : TEST IF CALLBACKS WORK AND ARE CLOSED CORRECTLY!
      *data->completionFlag = true;
      bool* cbFlag = (bool*)malloc(sizeof(bool));
      *cbFlag = false;
      auto cbThread = new std::thread(&callbackWrappingFunction, data->cbFun, data->cbData, cbFlag);
      threadFlagPairVector.emplace_back(cbThread, cbFlag);
      break;
    }
  }
  delete data;

  // Check if any handles are waiting in queue
  checkHandleQueue();

  // Calling timout starts a new download
  int running_handles;
  curl_multi_socket_action(curlMultiHandle, CURL_SOCKET_TIMEOUT, 0, &running_handles);
  checkMultiInfo();
}

void CCDBDownloader::checkMultiInfo()
{
  CURLMsg *message;
  int pending;

  while ((message = curl_multi_info_read(curlMultiHandle, &pending)))
  {
    switch (message->msg)
    {
    case CURLMSG_DONE:
    {
      CURLcode code = message->data.result;
      transferFinished(message->easy_handle, code);      
    }
    break;

    default:
      fprintf(stderr, "CURLMSG default\n");
      break;
    }
  }
}

int CCDBDownloader::startTimeout(CURLM *multi, long timeout_ms, void *userp)
{
  auto timeout = (uv_timer_t *)userp;

  if (timeout_ms < 0)
  {
    uv_timer_stop(timeout);
  }
  else
  {
    if (timeout_ms == 0)
      timeout_ms = 1; // Calling curlTimeout when timeout = 0 could create an infinite loop                       
    uv_timer_start(timeout, curlTimeout, timeout_ms, 0);
  }
  return 0;
}

void CCDBDownloader::checkHandleQueue()
{
  // Lock access to handle queue
  handlesQueueLock.lock();
  if (handlesToBeAdded.size() > 0)
  {
    // Add handles without going over the limit
    while(handlesToBeAdded.size() > 0 && handlesInUse < maxHandlesInUse) {
      curl_multi_add_handle(curlMultiHandle, handlesToBeAdded.front());
      handlesInUse++;
      handlesToBeAdded.erase(handlesToBeAdded.begin());
    }
  }
  handlesQueueLock.unlock();
}

void CCDBDownloader::runLoop()
{
  uv_run(&loop, UV_RUN_DEFAULT);
}

std::vector<CURLcode>* CCDBDownloader::batchAsynchPerform(std::vector<CURL*> handleVector, bool *completionFlag)
{
  auto codeVector = new std::vector<CURLcode>(handleVector.size());
  size_t *requestsLeft = new size_t();
  *requestsLeft = handleVector.size();

  handlesQueueLock.lock();
  for(int i = 0; i < handleVector.size(); i++)
  {
    auto *data = new CCDBDownloader::PerformData();

    data->codeDestination = &(*codeVector)[i];
    (*codeVector)[i] = CURLE_FAILED_INIT;
    data->requestsLeft = requestsLeft;
    data->completionFlag = completionFlag;
    data->type = ASYNCHRONOUS;

    curl_easy_setopt(handleVector[i], CURLOPT_PRIVATE, data);

    curl_easy_setopt(handleVector[i], CURLOPT_CLOSESOCKETFUNCTION, closesocketCallback);
    curl_easy_setopt(handleVector[i], CURLOPT_CLOSESOCKETDATA, this);
    curl_easy_setopt(handleVector[i], CURLOPT_OPENSOCKETFUNCTION, opensocketCallback);
    curl_easy_setopt(handleVector[i], CURLOPT_OPENSOCKETDATA, this);

    handlesToBeAdded.push_back(handleVector[i]);
  }
  handlesQueueLock.unlock();
  makeLoopCheckQueueAsync();
  return codeVector;
}

CURLcode CCDBDownloader::perform(CURL* handle)
{
  char* url;;
  curl_easy_getinfo(handle, CURLINFO_EFFECTIVE_URL, &url);

  std::vector<CURL*> handleVector;
  handleVector.push_back(handle);
  return batchBlockingPerform(handleVector).back();
}

std::vector<CURLcode> CCDBDownloader::batchBlockingPerform(std::vector<CURL*> handleVector)
{
  std::condition_variable cv;
  std::mutex cv_m;
  std::unique_lock<std::mutex> lk(cv_m);

  std::vector<CURLcode> codeVector(handleVector.size());
  size_t requestsLeft = handleVector.size();

  handlesQueueLock.lock();
  for(int i = 0; i < handleVector.size(); i++)
  {
    auto *data = new CCDBDownloader::PerformData();
    data->codeDestination = &codeVector[i];
    codeVector[i] = CURLE_FAILED_INIT;
    data->cv = &cv;
    data->type = BLOCKING;
    data->requestsLeft = &requestsLeft;

    curl_easy_setopt(handleVector[i], CURLOPT_PRIVATE, data);

    curl_easy_setopt(handleVector[i], CURLOPT_CLOSESOCKETFUNCTION, closesocketCallback);
    curl_easy_setopt(handleVector[i], CURLOPT_CLOSESOCKETDATA, this);
    curl_easy_setopt(handleVector[i], CURLOPT_OPENSOCKETFUNCTION, opensocketCallback);
    curl_easy_setopt(handleVector[i], CURLOPT_OPENSOCKETDATA, this);

    handlesToBeAdded.push_back(handleVector[i]);
  }
  handlesQueueLock.unlock();
  makeLoopCheckQueueAsync();
  cv.wait(lk);
  return codeVector;
}

std::vector<CURLcode> *CCDBDownloader::asynchBatchPerformWithCallback(std::vector<CURL*> handleVector, bool *completionFlag, void (*cbFun)(void*), void* cbData)
{
  auto codeVector = new std::vector<CURLcode>(handleVector.size());
  size_t *requestsLeft = new size_t();
  *requestsLeft = handleVector.size();

  handlesQueueLock.lock();
  for(int i = 0; i < handleVector.size(); i++)
  {
    auto *data = new CCDBDownloader::PerformData();

    data->codeDestination = &(*codeVector)[i];
    (*codeVector)[i] = CURLE_FAILED_INIT;
    data->requestsLeft = requestsLeft;
    data->completionFlag = completionFlag;
    data->type = ASYNCHRONOUS_WITH_CALLBACK;

    data->cbFun = cbFun;
    data->cbData = cbData;

    curl_easy_setopt(handleVector[i], CURLOPT_PRIVATE, data);

    curl_easy_setopt(handleVector[i], CURLOPT_CLOSESOCKETFUNCTION, closesocketCallback);
    curl_easy_setopt(handleVector[i], CURLOPT_CLOSESOCKETDATA, this);
    curl_easy_setopt(handleVector[i], CURLOPT_OPENSOCKETFUNCTION, opensocketCallback);
    curl_easy_setopt(handleVector[i], CURLOPT_OPENSOCKETDATA, this);

    handlesToBeAdded.push_back(handleVector[i]);
  }
  handlesQueueLock.unlock();
  makeLoopCheckQueueAsync();
  return codeVector;
}

void CCDBDownloader::makeLoopCheckQueueAsync()
{
  auto asyncHandle = new uv_async_t();
  asyncHandle->data = this;
  uv_async_init(&loop, asyncHandle, asyncUVHandleCheckQueue);
  uv_async_send(asyncHandle);
}

}
}