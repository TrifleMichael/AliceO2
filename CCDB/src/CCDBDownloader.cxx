// Copyright 2019-2023 CERN and copyright holders of ALICE O2.
// See https://alice-o2.web.cern.ch/copyright for details of the copyright holders.
// All rights not expressly granted are reserved.
//
// This software is distributed under the terms of the GNU General Public
// License v3 (GPL Version 3), copied verbatim in the file "COPYING".
//
// In applying this license CERN does not waive the privileges and immunities
// granted to it by virtue of its status as an Intergovernmental Organization
// or submit itself to any jurisdiction.

#include <CCDB/CCDBDownloader.h>
#include "CommonUtils/StringUtils.h"
#include "CommonUtils/FileSystemUtils.h"
#include "CCDB/CCDBTimeStampUtils.h"

#include <filesystem>
#include <curl/curl.h>
#include <unordered_map>
#include <cstdio>
#include <cstdlib>
#include <uv.h>
#include <string>
#include <thread>
#include <vector>
#include <condition_variable>
#include <mutex>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <fairlogger/Logger.h>
#include <boost/asio/ip/host_name.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/interprocess/sync/named_semaphore.hpp>

// TODO check what const and what not

namespace o2::ccdb
{
std::mutex gIOMutex2; // TODO get rid of 2

void uvErrorCheck(int code)
{
  if (code != 0) {
    char buf[1000];
    uv_strerror_r(code, buf, 1000);
    LOG(error) << "CCDBDownloader: UV error - " << buf;
  }
}

void curlEasyErrorCheck(CURLcode code)
{
  if (code != CURLE_OK) {
    LOG(error) << "CCDBDownloader: CURL error - " << curl_easy_strerror(code);
  }
}

void curlMultiErrorCheck(CURLMcode code)
{
  if (code != CURLM_OK) {
    LOG(error) << "CCDBDownloader: CURL error - " << curl_multi_strerror(code);
  }
}

namespace
{
std::string uniqueAgentID()
{
  std::string host = boost::asio::ip::host_name();
  char const* jobID = getenv("ALIEN_PROC_ID");
  if (jobID) {
    return fmt::format("{}-{}-{}-{}", host, getCurrentTimestamp() / 1000, o2::utils::Str::getRandomString(6), jobID);
  } else {
    return fmt::format("{}-{}-{}", host, getCurrentTimestamp() / 1000, o2::utils::Str::getRandomString(6));
  }
}
} // namespace

CCDBDownloader::CCDBDownloader(uv_loop_t* uv_loop)
  : mUserAgentId(uniqueAgentID())
{
  std::string snapshotReport{};
  const char* cachedir = getenv("ALICEO2_CCDB_LOCALCACHE");
  if (cachedir) {
    if (cachedir[0] == 0) {
      mSnapshotCachePath = ".";
    } else {
      mSnapshotCachePath = cachedir;
    }
    snapshotReport = fmt::format("(cache snapshots to dir={}", mSnapshotCachePath);
  }
  if (getenv("IGNORE_VALIDITYCHECK_OF_CCDB_LOCALCACHE")) {
    mPreferSnapshotCache = true;
    if (mSnapshotCachePath.empty()) {
      LOGP(fatal, "IGNORE_VALIDITYCHECK_OF_CCDB_LOCALCACHE is defined but the ALICEO2_CCDB_LOCALCACHE is not");
    }
    snapshotReport += ", prefer if available";
  }
  if (!snapshotReport.empty()) {
    snapshotReport += ')';
  }



  // Old constructor starts

  if (uv_loop) {
    mExternalLoop = true;
    mUVLoop = uv_loop;
  } else {
    mExternalLoop = false;
    setupInternalUVLoop();
  }

  // Preparing timer to be used by curl
  mTimeoutTimer = new uv_timer_t();
  mTimeoutTimer->data = this;
  uvErrorCheck(uv_timer_init(mUVLoop, mTimeoutTimer));
  mHandleMap[(uv_handle_t*)mTimeoutTimer] = true;

  initializeMultiHandle();
}

void CCDBDownloader::setupInternalUVLoop()
{
  mUVLoop = new uv_loop_t();
  uvErrorCheck(uv_loop_init(mUVLoop));
}

void CCDBDownloader::initializeMultiHandle()
{
  mCurlMultiHandle = curl_multi_init();
  curlMultiErrorCheck(curl_multi_setopt(mCurlMultiHandle, CURLMOPT_SOCKETFUNCTION, handleSocket));
  auto socketData = &mSocketData;
  socketData->curlm = mCurlMultiHandle;
  socketData->CD = this;
  curlMultiErrorCheck(curl_multi_setopt(mCurlMultiHandle, CURLMOPT_SOCKETDATA, socketData));
  curlMultiErrorCheck(curl_multi_setopt(mCurlMultiHandle, CURLMOPT_TIMERFUNCTION, startTimeout));
  curlMultiErrorCheck(curl_multi_setopt(mCurlMultiHandle, CURLMOPT_TIMERDATA, mTimeoutTimer));
  curlMultiErrorCheck(curl_multi_setopt(mCurlMultiHandle, CURLMOPT_MAX_TOTAL_CONNECTIONS, mMaxHandlesInUse));
}

CCDBDownloader::~CCDBDownloader()
{
  // Loop has been ordered to stop via signalToClose()
  curlMultiErrorCheck(curl_multi_cleanup(mCurlMultiHandle));

  if (!mExternalLoop) {
    // Schedule all handles to close. Execute loop to allow them to execute their destructors.
    while (uv_loop_alive(mUVLoop) && uv_loop_close(mUVLoop) == UV_EBUSY) {
      uv_walk(mUVLoop, closeHandles, this);
      uv_run(mUVLoop, UV_RUN_ONCE);
    }
    delete mUVLoop;
  }
}

void closeHandles(uv_handle_t* handle, void* arg)
{
  auto CD = (CCDBDownloader*)arg;
  // Close only handles belonging to the Downloader
  if (CD->mHandleMap.find(handle) != CD->mHandleMap.end()) {
    if (!uv_is_closing(handle)) {
      uv_close(handle, onUVClose);
    }
  }
}

void onUVClose(uv_handle_t* handle)
{
  if (handle != nullptr) {
    delete handle;
  }
}

void CCDBDownloader::closesocketCallback(void* clientp, curl_socket_t item)
{
  auto CD = (CCDBDownloader*)clientp;
  if (CD->mExternalLoop) {
    // If external uv loop is used then the keepalive mechanism is active.
    if (CD->mSocketTimerMap.find(item) != CD->mSocketTimerMap.end()) {
      auto timer = CD->mSocketTimerMap[item];
      uvErrorCheck(uv_timer_stop(timer));
      // we are getting rid of the uv_timer_t pointer ... so we need
      // to free possibly attached user data pointers as well. Counteracts action of opensocketCallback
      if (timer->data) {
        delete (DataForClosingSocket*)timer->data;
      }
      CD->mSocketTimerMap.erase(item);
      if (close(item) == -1) {
        LOG(error) << "CCDBDownloader: Socket failed to close";
      }
    }
  } else {
    if (close(item) == -1) {
      LOG(error) << "CCDBDownloader: Socket failed to close";
    }
  }
}

curl_socket_t opensocketCallback(void* clientp, curlsocktype purpose, struct curl_sockaddr* address)
{
  auto CD = (CCDBDownloader*)clientp;
  auto sock = socket(address->family, address->socktype, address->protocol);
  if (sock == -1) {
    LOG(error) << "CCDBDownloader: Socket failed to open";
  }

  if (CD->mExternalLoop) {
    CD->mSocketTimerMap[sock] = new uv_timer_t();
    uvErrorCheck(uv_timer_init(CD->mUVLoop, CD->mSocketTimerMap[sock]));
    CD->mHandleMap[(uv_handle_t*)CD->mSocketTimerMap[sock]] = true;

    auto data = new DataForClosingSocket();
    data->CD = CD;
    data->socket = sock;
    CD->mSocketTimerMap[sock]->data = data;
  }

  return sock;
}

void CCDBDownloader::closeSocketByTimer(uv_timer_t* handle)
{
  auto data = (DataForClosingSocket*)handle->data;
  auto CD = data->CD;
  auto sock = data->socket;

  if (CD->mSocketTimerMap.find(sock) != CD->mSocketTimerMap.end()) {
    uvErrorCheck(uv_timer_stop(CD->mSocketTimerMap[sock]));
    CD->mSocketTimerMap.erase(sock);
    if (close(sock) == -1) {
      LOG(error) << "CCDBDownloader: Socket failed to close";
    }
    delete data;
  }
}

void CCDBDownloader::curlTimeout(uv_timer_t* handle)
{
  auto CD = (CCDBDownloader*)handle->data;
  int running_handles;
  curl_multi_socket_action(CD->mCurlMultiHandle, CURL_SOCKET_TIMEOUT, 0, &running_handles);
  CD->checkMultiInfo();
}

void CCDBDownloader::curlPerform(uv_poll_t* handle, int status, int events)
{
  uvErrorCheck(status);
  int running_handles;
  int flags = 0;
  if (events & UV_READABLE) {
    flags |= CURL_CSELECT_IN;
  }
  if (events & UV_WRITABLE) {
    flags |= CURL_CSELECT_OUT;
  }

  auto context = (CCDBDownloader::curl_context_t*)handle->data;

  curlMultiErrorCheck(curl_multi_socket_action(context->CD->mCurlMultiHandle, context->sockfd, flags, &running_handles));
  context->CD->checkMultiInfo();
}

int CCDBDownloader::handleSocket(CURL* easy, curl_socket_t s, int action, void* userp, void* socketp)
{
  auto socketData = (CCDBDownloader::DataForSocket*)userp;
  auto CD = (CCDBDownloader*)socketData->CD;
  CCDBDownloader::curl_context_t* curl_context;
  int events = 0;

  switch (action) {
    case CURL_POLL_IN:
    case CURL_POLL_OUT:
    case CURL_POLL_INOUT:

      curl_context = socketp ? (CCDBDownloader::curl_context_t*)socketp : CD->createCurlContext(s);
      curlMultiErrorCheck(curl_multi_assign(socketData->curlm, s, (void*)curl_context));

      if (action != CURL_POLL_IN) {
        events |= UV_WRITABLE;
      }
      if (action != CURL_POLL_OUT) {
        events |= UV_READABLE;
      }

      if (CD->mExternalLoop && CD->mSocketTimerMap.find(s) != CD->mSocketTimerMap.end()) {
        uvErrorCheck(uv_timer_stop(CD->mSocketTimerMap[s]));
      }

      uvErrorCheck(uv_poll_start(curl_context->poll_handle, events, curlPerform));
      break;
    case CURL_POLL_REMOVE:
      if (socketp) {
        if (CD->mExternalLoop) {
          // If external loop is used then start the keepalive timeout.
          if (CD->mSocketTimerMap.find(s) != CD->mSocketTimerMap.end()) {
            uvErrorCheck(uv_timer_start(CD->mSocketTimerMap[s], closeSocketByTimer, CD->mKeepaliveTimeoutMS, 0));
          }
        }
        uvErrorCheck(uv_poll_stop(((CCDBDownloader::curl_context_t*)socketp)->poll_handle));
        CD->destroyCurlContext((CCDBDownloader::curl_context_t*)socketp);
        curlMultiErrorCheck(curl_multi_assign(socketData->curlm, s, nullptr));
      }
      break;
    default:
      abort();
  }

  return 0;
}

void CCDBDownloader::setMaxParallelConnections(int limit)
{
  mMaxHandlesInUse = limit;
}

void CCDBDownloader::setKeepaliveTimeoutTime(int timeoutMS)
{
  mKeepaliveTimeoutMS = timeoutMS;
}

void CCDBDownloader::setConnectionTimeoutTime(int timeoutMS)
{
  mConnectionTimeoutMS = timeoutMS;
}

void CCDBDownloader::setRequestTimeoutTime(int timeoutMS)
{
  mRequestTimeoutMS = timeoutMS;
}

void CCDBDownloader::setHappyEyeballsHeadstartTime(int headstartMS)
{
  mHappyEyeballsHeadstartMS = headstartMS;
}

void CCDBDownloader::setOfflineTimeoutSettings()
{
  setConnectionTimeoutTime(60000);
  setRequestTimeoutTime(300000);
  setHappyEyeballsHeadstartTime(500);
}

void CCDBDownloader::setOnlineTimeoutSettings()
{
  setConnectionTimeoutTime(5000);
  setRequestTimeoutTime(30000);
  setHappyEyeballsHeadstartTime(500);
}

CCDBDownloader::curl_context_t* CCDBDownloader::createCurlContext(curl_socket_t sockfd)
{
  curl_context_t* context;

  context = (curl_context_t*)malloc(sizeof(*context));
  context->CD = this;
  context->sockfd = sockfd;
  context->poll_handle = new uv_poll_t();

  uvErrorCheck(uv_poll_init_socket(mUVLoop, context->poll_handle, sockfd));
  mHandleMap[(uv_handle_t*)(context->poll_handle)] = true;
  context->poll_handle->data = context;

  return context;
}

void CCDBDownloader::curlCloseCB(uv_handle_t* handle)
{
  auto* context = (curl_context_t*)handle->data;
  delete context->poll_handle;
  free(context);
}

void CCDBDownloader::destroyCurlContext(curl_context_t* context)
{
  uv_close((uv_handle_t*)context->poll_handle, curlCloseCB);
}

// TODO change name maybe?
void CCDBDownloader::deletePerformData(CCDBDownloader::PerformData* data, CURLcode code)
{
  *data->codeDestination = code;
  *data->transferFinished = true;
  --(*data->requestsLeft);
  delete data;
}

void CCDBDownloader::transferFinished(CURL* easy_handle, CURLcode curlCode)
{
  mHandlesInUse--;
  PerformData* data;
  curlEasyErrorCheck(curl_easy_getinfo(easy_handle, CURLINFO_PRIVATE, &data));

  curlMultiErrorCheck(curl_multi_remove_handle(mCurlMultiHandle, easy_handle));
  
  char* effectiveUrl;
  curl_easy_getinfo(easy_handle, CURLINFO_EFFECTIVE_URL, &effectiveUrl);
  long httpCode;
  curl_easy_getinfo(easy_handle, CURLINFO_HTTP_CODE, &httpCode);

  std::cout << "Transfer finished for " << effectiveUrl << " with http code " << httpCode <<"\n";

  if (data->type != REQUEST) {
    deletePerformData(data, curlCode);
  } else {
    if (300 <= httpCode && httpCode < 400) {
      // Follow redirect
      std::vector<std::string>* locationVector = (*data->locationsMap)[easy_handle];

      bool nextDownloadScheduled = false;
      bool resultRetrieved = false;
      while (++data->currentLocationIndex < locationVector->size() && !resultRetrieved) {
        std::string nextLocation = (*locationVector)[data->currentLocationIndex];

        if (nextLocation.find("file:/", 0) != std::string::npos) {
          std::cout << "Found local file " << nextLocation << "\n";
          // TODO react to local redirect
        } else if (nextLocation.find("alien:/", 0) != std::string::npos) {
          std::cout << "Pointed to alien "  << nextLocation << "\n";
          loadFileToMemory(*data->dst, nextLocation, nullptr); // TODO Double check whether no local headers ok
          if (data->dst->size() != 0) {
            resultRetrieved = true;
          } else {
            std::cout << "PossibleError: loadFileToMemory could not retrieve object from " << nextLocation << "\n";
          }
          // *data->objectPtr = item; // TODO What about object pointer?
        } else {
          std::cout << "Setting up redirect to " << (data->host + nextLocation) << "\n";
          curl_easy_setopt(easy_handle, CURLOPT_URL, (data->host + nextLocation).c_str());
          mHandlesToBeAdded.push_back(easy_handle);
          nextDownloadScheduled = true;
        }
      }

      if (!nextDownloadScheduled) {
        // No more redirects to follow
        deletePerformData(data, curlCode);
      }
    } else if (data->dst->size() == 0) { // TODO recheck the condition
      if (++data->hostInd < hostsPool.size()) {
        string fullUrl = getFullUrlForRetrieval(easy_handle, data->path, *data->metadata, data->timestamp, data->hostInd);
        std::cout << "Pointing to next host " << fullUrl << "\n";
        curl_easy_setopt(easy_handle, CURLOPT_URL, fullUrl.c_str());
        mHandlesToBeAdded.push_back(easy_handle);
      }
    } else {
      deletePerformData(data, curlCode);
    }
  }

  checkHandleQueue();

  // Calling timeout starts a new download if a new easy_handle was added.
  int running_handles;
  curlMultiErrorCheck(curl_multi_socket_action(mCurlMultiHandle, CURL_SOCKET_TIMEOUT, 0, &running_handles));
  checkMultiInfo();
}

void CCDBDownloader::checkMultiInfo()
{
  CURLMsg* message;
  int pending;

  while ((message = curl_multi_info_read(mCurlMultiHandle, &pending))) {
    switch (message->msg) {
      case CURLMSG_DONE: {
        CURLcode code = message->data.result;
        transferFinished(message->easy_handle, code);
      } break;

      default:
        fprintf(stderr, "CURLMSG default\n");
        break;
    }
  }
}

int CCDBDownloader::startTimeout(CURLM* multi, long timeout_ms, void* userp)
{
  auto timeout = (uv_timer_t*)userp;

  if (timeout_ms < 0) {
    uvErrorCheck(uv_timer_stop(timeout));
  } else {
    if (timeout_ms == 0) {
      timeout_ms = 1; // Calling curlTimeout when timeout = 0 could create an infinite loop
    }
    uvErrorCheck(uv_timer_start(timeout, curlTimeout, timeout_ms, 0));
  }
  return 0;
}

void CCDBDownloader::setHandleOptions(CURL* handle, PerformData* data)
{
  curlEasyErrorCheck(curl_easy_setopt(handle, CURLOPT_PRIVATE, data));
  curlEasyErrorCheck(curl_easy_setopt(handle, CURLOPT_CLOSESOCKETFUNCTION, closesocketCallback));
  curlEasyErrorCheck(curl_easy_setopt(handle, CURLOPT_CLOSESOCKETDATA, this));
  curlEasyErrorCheck(curl_easy_setopt(handle, CURLOPT_OPENSOCKETFUNCTION, opensocketCallback));
  curlEasyErrorCheck(curl_easy_setopt(handle, CURLOPT_OPENSOCKETDATA, this));

  curlEasyErrorCheck(curl_easy_setopt(handle, CURLOPT_TIMEOUT_MS, mRequestTimeoutMS));
  curlEasyErrorCheck(curl_easy_setopt(handle, CURLOPT_CONNECTTIMEOUT_MS, mConnectionTimeoutMS));
  curlEasyErrorCheck(curl_easy_setopt(handle, CURLOPT_HAPPY_EYEBALLS_TIMEOUT_MS, mHappyEyeballsHeadstartMS));
  curlEasyErrorCheck(curl_easy_setopt(handle, CURLOPT_USERAGENT, mUserAgentId.c_str()));
}

void CCDBDownloader::checkHandleQueue()
{
  if (mHandlesToBeAdded.size() > 0) {
    // Add handles without going over the limit
    while (mHandlesToBeAdded.size() > 0 && mHandlesInUse < mMaxHandlesInUse) {
      curlMultiErrorCheck(curl_multi_add_handle(mCurlMultiHandle, mHandlesToBeAdded.front()));
      mHandlesInUse++;
      mHandlesToBeAdded.erase(mHandlesToBeAdded.begin());
    }
  }
}

void CCDBDownloader::runLoop(bool noWait)
{
  uv_run(mUVLoop, noWait ? UV_RUN_NOWAIT : UV_RUN_ONCE);
}

size_t WriteHeaderCallback(void* contents, size_t size, size_t nmemb, void* userdata) { // TODO Remove or change
  size_t totalSize = size * nmemb;
  std::string header(reinterpret_cast<char*>(contents), totalSize);
  
  auto headers = (std::vector<std::string>*)userdata;
  if (header.find("Content-Location") != std::string::npos) {
    std::string redirect = header.substr(18, header.size()-20);
    headers->push_back(redirect);
  }
    
  return totalSize;
}

CURLcode CCDBDownloader::perform(CURL* handle)
{
  std::vector<CURL*> handleVector;
  handleVector.push_back(handle);

  auto code = batchBlockingPerform(handleVector).back();

  return code;
}

CCDBDownloader::TransferResults* CCDBDownloader::prepareResultsStruct(size_t numberOfHandles)
{
  auto results = new CCDBDownloader::TransferResults();
  results->requestsLeft = numberOfHandles;
  // We will save pointers to positions in the vectors `curlCodes` and `transferFinishedFlags` at a later point.
  // To make sure the vectors aren't relocated after saving pointers (and positions invalidated) we set the final size here.
  // The size must not be changed later.
  results->curlCodes.resize(numberOfHandles);
  results->transferFinishedFlags.resize(numberOfHandles);

  // Fill with default values
  for (int i = 0; i < numberOfHandles; i++) {
    results->transferFinishedFlags[i] = false;
    results->curlCodes[i] = CURLE_FAILED_INIT;
  }
  return results;
}

CCDBDownloader::PerformData* CCDBDownloader::createPerformData(uint handleIndex, TransferResults* results, RequestType requestType)
{
  auto* data = new CCDBDownloader::PerformData();
  data->type = requestType;
  data->codeDestination = &results->curlCodes[handleIndex];
  // vector<bool> positions must be passed via iterator. It is because of a quirk that only vector<bool> has.
  data->transferFinished = results->transferFinishedFlags.begin() + handleIndex;
  data->requestsLeft = &results->requestsLeft;
  return data;
}

std::vector<CURLcode> CCDBDownloader::batchBlockingPerform(std::vector<CURL*> const& handleVector)
{
  auto results = prepareResultsStruct(handleVector.size());

  for (int handleIndex = 0; handleIndex < handleVector.size(); handleIndex++) {
    auto* data = createPerformData(handleIndex, results, BLOCKING);
    setHandleOptions(handleVector[handleIndex], data);
    mHandlesToBeAdded.push_back(handleVector[handleIndex]);
  }
  checkHandleQueue();
  while (results->requestsLeft > 0) {
    uv_run(mUVLoop, UV_RUN_ONCE);
  }
  vector<CURLcode> curlCodes = results->curlCodes;
  delete results;
  return curlCodes;
}

CCDBDownloader::TransferResults* CCDBDownloader::batchAsynchPerform(std::vector<CURL*> const& handleVector)
{
  auto results = prepareResultsStruct(handleVector.size());

  for (int handleIndex = 0; handleIndex < handleVector.size(); handleIndex++) {
    auto* data = createPerformData(handleIndex, results, ASYNCHRONOUS);
    setHandleOptions(handleVector[handleIndex], data);
    mHandlesToBeAdded.push_back(handleVector[handleIndex]);
  }
  checkHandleQueue();
  return results;
}

CCDBDownloader::TransferResults* CCDBDownloader::batchRequestPerform(CURL* const& handle, std::string path, const map<string, string>& metadata, long timestamp, o2::pmr::vector<char>& dst)
{
  auto results = prepareResultsStruct(1); // TODO remove this vector?
  std::map<CURL*, std::vector<std::string>*> locationsMap;

  auto* data = createPerformData(0, results, REQUEST);
  data->hostInd = 0;
  data->path = path;
  data->metadata = &metadata;
  data->timestamp = timestamp;
  data->objectPtr = &results->objectPtr;
  data->dst = &dst;

  curl_easy_setopt(handle, CURLOPT_HEADERFUNCTION, WriteHeaderCallback);
  locationsMap[handle] = new std::vector<std::string>();
  curl_easy_setopt(handle, CURLOPT_HEADERDATA, locationsMap[handle]);
  data->locationsMap = &locationsMap;

  setHandleOptions(handle, data);
  mHandlesToBeAdded.push_back(handle);

  checkHandleQueue();
  while (results->requestsLeft > 0) {
    uv_run(mUVLoop, UV_RUN_ONCE);
  }
  // TODO Free map
  return results;
}

std::vector<CURLcode>::iterator CCDBDownloader::getAll(TransferResults* results)
{
  while (results->requestsLeft > 0) {
    runLoop(false);
  }
  return results->curlCodes.begin();
}

void CCDBDownloader::init(std::vector<std::string> hosts) {
  hostsPool = hosts;
}

CCDBDownloader::TransferResults* CCDBDownloader::scheduleFromRequest2(CURL* handle, uint hostInd, std::string path, const map<string, string>& metadata, long timestamp, o2::pmr::vector<char>& dst, size_t writeCallBack(void* contents, size_t size, size_t nmemb, void* chunkptr))
{
  HeaderObjectPair_t hoPair{{}, &dst, 0};
  curl_easy_setopt(handle, CURLOPT_WRITEFUNCTION, writeCallBack);
  curl_easy_setopt(handle, CURLOPT_WRITEDATA, (void*)&hoPair);
  string fullUrl = getFullUrlForRetrieval(handle, path, metadata, timestamp, hostInd);
  curl_easy_setopt(handle, CURLOPT_URL, fullUrl.c_str());
  auto userAgent = uniqueAgentID();
  curl_easy_setopt(handle, CURLOPT_USERAGENT, userAgent.c_str());

  std::cout << "Starting transfer for " << fullUrl << "\n";
  TransferResults* results = batchRequestPerform(handle, path, metadata, timestamp, dst);

  std::cout << "Curl code " << results->curlCodes[0] << "\n";
  long httpCode;
  curl_easy_getinfo(handle, CURLINFO_HTTP_CODE, &httpCode);
  std::cout << "Http code " << httpCode << "\n";

  std::cout << "Vector size " << dst.size() << "\n";

  return results;
}

void* CCDBDownloader::downloadAlienContent(std::string const& url, std::type_info const& tinfo) const
{
  if (!initTGrid()) {
    return nullptr;
  }
  std::lock_guard<std::mutex> guard(gIOMutex2); // TODO double check
  auto memfile = TMemFile::Open(url.c_str(), "OPEN");
  if (memfile) {
    auto cl = tinfo2TClass(tinfo);
    auto content = extractFromTFile(*memfile, cl, "ccdb_object"); // TODO REMOVE "ccdb_object"
    delete memfile;
    return content;
  }
  return nullptr;
}

TClass* CCDBDownloader::tinfo2TClass(std::type_info const& tinfo)
{
  TClass* cl = TClass::GetClass(tinfo);
  if (!cl) {
    throw std::runtime_error(fmt::format("Could not retrieve ROOT dictionary for type {}, aborting", tinfo.name()));
    return nullptr;
  }
  return cl;
}

void* CCDBDownloader::extractFromTFile(TFile& file, TClass const* cl, const char* what)
{
  if (!cl) {
    return nullptr;
  }
  auto object = file.GetObjectChecked(what, cl);
  if (!object) {
    // it could be that object was stored with previous convention
    // where the classname was taken as key
    std::string objectName(cl->GetName());
    o2::utils::Str::trim(objectName);
    object = file.GetObjectChecked(objectName.c_str(), cl);
    LOG(warn) << "Did not find object under expected name " << what;
    if (!object) {
      return nullptr;
    }
    LOG(warn) << "Found object under deprecated name " << cl->GetName();
  }
  auto result = object;
  // We need to handle some specific cases as ROOT ties them deeply
  // to the file they are contained in
  if (cl->InheritsFrom("TObject")) {
    // make a clone
    // detach from the file
    auto tree = dynamic_cast<TTree*>((TObject*)object);
    if (tree) {
      tree->LoadBaskets(0x1L << 32); // make tree memory based
      tree->SetDirectory(nullptr);
      result = tree;
    } else {
      auto h = dynamic_cast<TH1*>((TObject*)object);
      if (h) {
        h->SetDirectory(nullptr);
        result = h;
      }
    }
  }
  return result;
}

bool CCDBDownloader::initTGrid() const
{
  if (mNeedAlienToken && !mAlienInstance) {
    static bool allowNoToken = getenv("ALICEO2_CCDB_NOTOKENCHECK") && atoi(getenv("ALICEO2_CCDB_NOTOKENCHECK"));
    if (!allowNoToken && !checkAlienToken()) {
      LOG(fatal) << "Alien Token Check failed - Please get an alien token before running with https CCDB endpoint, or alice-ccdb.cern.ch!";
    }
    mAlienInstance = TGrid::Connect("alien");
    static bool errorShown = false;
    if (!mAlienInstance && errorShown == false) {
      if (allowNoToken) {
        LOG(error) << "TGrid::Connect returned nullptr. May be due to missing alien token";
      } else {
        LOG(fatal) << "TGrid::Connect returned nullptr. May be due to missing alien token";
      }
      errorShown = true;
    }
  }
  return mAlienInstance != nullptr;
}

bool CCDBDownloader::checkAlienToken()
{
#ifdef __APPLE__
  LOG(debug) << "On macOS we simply rely on TGrid::Connect(\"alien\").";
  return true;
#endif
  if (getenv("ALICEO2_CCDB_NOTOKENCHECK") && atoi(getenv("ALICEO2_CCDB_NOTOKENCHECK"))) {
    return true;
  }
  if (getenv("JALIEN_TOKEN_CERT")) {
    return true;
  }
  auto returncode = system("LD_PRELOAD= alien-token-info &> /dev/null");
  if (returncode == -1) {
    LOG(error) << "...";
  }
  return returncode == 0;
}

// TODO convert mInSnapshotMode and mPreferSnapshotCache to member fields
void CCDBDownloader::loadFileToMemory(o2::pmr::vector<char>& dest, const std::string& path, std::map<std::string, std::string>* localHeaders)
{
  // Read file to memory as vector. For special case of the locally cached file retriev metadata stored directly in the file
  constexpr size_t MaxCopySize = 0x1L << 25;
  auto signalError = [&dest, localHeaders]() {
    dest.clear();
    dest.reserve(1);
    if (localHeaders) { // indicate that an error occurred ---> used by caching layers (such as CCDBManager)
      (*localHeaders)["Error"] = "An error occurred during retrieval";
    }
  };
  if (path.find("alien:/") == 0 && !initTGrid()) {
    signalError();
    return;
  }
  std::string fname(path);
  if (fname.find("?filetype=raw") == std::string::npos) {
    fname += "?filetype=raw";
  }
  std::unique_ptr<TFile> sfile{TFile::Open(fname.c_str())};
  if (!sfile || sfile->IsZombie()) {
    LOG(error) << "Failed to open file " << fname;
    signalError();
    return;
  }
  size_t totalread = 0, fsize = sfile->GetSize(), b00 = sfile->GetBytesRead();
  dest.resize(fsize);
  char* dptr = dest.data();
  sfile->Seek(0);
  long nread = 0;
  do {
    size_t b0 = sfile->GetBytesRead(), b1 = b0 - b00;
    size_t readsize = fsize - b1 > MaxCopySize ? MaxCopySize : fsize - b1;
    if (readsize == 0) {
      break;
    }
    sfile->Seek(totalread, TFile::kBeg);
    bool failed = sfile->ReadBuffer(dptr, (Int_t)readsize);
    nread = sfile->GetBytesRead() - b0;
    if (failed || nread < 0) {
      LOG(error) << "failed to copy file " << fname << " to memory buffer";
      signalError();
      return;
    }
    dptr += nread;
    totalread += nread;
  } while (nread == (long)MaxCopySize);

  if (localHeaders) {
    TMemFile memFile("name", const_cast<char*>(dest.data()), dest.size(), "READ");
    auto storedmeta = (std::map<std::string, std::string>*)extractFromTFile(memFile, TClass::GetClass("std::map<std::string, std::string>"), CCDBMETA_ENTRY);
    if (storedmeta) {
      *localHeaders = *storedmeta; // do a simple deep copy
      delete storedmeta;
    }
    if ((mInSnapshotMode || mPreferSnapshotCache) && localHeaders->find("ETag") == localHeaders->end()) { // generate dummy ETag to profit from the caching
      (*localHeaders)["ETag"] = path;
    }
  }
  return;
}

namespace
{
template <typename MapType = std::map<std::string, std::string>>
size_t header_map_callback(char* buffer, size_t size, size_t nitems, void* userdata)
{
  auto* headers = static_cast<MapType*>(userdata);
  auto header = std::string(buffer, size * nitems);
  std::string::size_type index = header.find(':', 0);
  if (index != std::string::npos) {
    const auto key = boost::algorithm::trim_copy(header.substr(0, index));
    const auto value = boost::algorithm::trim_copy(header.substr(index + 1));
    headers->insert(std::make_pair(key, value));
  }
  return size * nitems;
}
} // namespace
void CCDBDownloader::initCurlHTTPHeaderOptionsForRetrieve(CURL* curlHandle, curl_slist*& option_list, long timestamp, std::map<std::string, std::string>* headers, std::string const& etag,
                                                   const std::string& createdNotAfter, const std::string& createdNotBefore) const
{
  // struct curl_slist* list = nullptr;
  if (!etag.empty()) {
    option_list = curl_slist_append(option_list, ("If-None-Match: " + etag).c_str());
  }

  if (!createdNotAfter.empty()) {
    option_list = curl_slist_append(option_list, ("If-Not-After: " + createdNotAfter).c_str());
  }

  if (!createdNotBefore.empty()) {
    option_list = curl_slist_append(option_list, ("If-Not-Before: " + createdNotBefore).c_str());
  }

  if (headers != nullptr) {
    option_list = curl_slist_append(option_list, ("If-None-Match: " + to_string(timestamp)).c_str());
    curl_easy_setopt(curlHandle, CURLOPT_HEADERFUNCTION, header_map_callback<>);
    curl_easy_setopt(curlHandle, CURLOPT_HEADERDATA, headers);
  }

  if (option_list) {
    curl_easy_setopt(curlHandle, CURLOPT_HTTPHEADER, option_list);
  }

  curl_easy_setopt(curlHandle, CURLOPT_USERAGENT, mUserAgentId.c_str()); // TODO mUserAgentId from something else is it ok
}

// A helper function used in a few places. Updates a ROOT file with meta/header information.
void CCDBDownloader::updateMetaInformationInLocalFile(std::string const& filename, std::map<std::string, std::string> const* headers, CCDBQuery const* querysummary)
{
  int gErrorIgnoreLevel = 6001; // TODO temporary

  std::lock_guard<std::mutex> guard(gIOMutex2);
  auto oldlevel = gErrorIgnoreLevel;
  gErrorIgnoreLevel = 6001; // ignoring error messages here (since we catch with IsZombie)
  TFile snapshotfile(filename.c_str(), "UPDATE");
  // The assumption is that the blob is a ROOT file
  if (!snapshotfile.IsZombie()) {
    if (querysummary && !snapshotfile.Get(CCDBQUERY_ENTRY)) {
      snapshotfile.WriteObjectAny(querysummary, TClass::GetClass(typeid(*querysummary)), CCDBQUERY_ENTRY);
    }
    if (headers && !snapshotfile.Get(CCDBMETA_ENTRY)) {
      snapshotfile.WriteObjectAny(headers, TClass::GetClass(typeid(*headers)), CCDBMETA_ENTRY);
    }
    snapshotfile.Write();
    snapshotfile.Close();
  }
  gErrorIgnoreLevel = oldlevel;
}

void CCDBDownloader::logReading(const std::string& path, long ts, const std::map<std::string, std::string>* headers, const std::string& comment) const
{
  std::string upath{path};
  if (headers) {
    auto ent = headers->find("Valid-From");
    if (ent != headers->end()) {
      upath += "/" + ent->second;
    }
    ent = headers->find("ETag");
    if (ent != headers->end()) {
      upath += "/" + ent->second;
    }
  }
  upath.erase(remove(upath.begin(), upath.end(), '\"'), upath.end());
  LOGP(info, "ccdb reads {}{}{} for {} ({}, agent_id: {}), ", mUrl, mUrl.back() == '/' ? "" : "/", upath, ts < 0 ? getCurrentTimestamp() : ts, comment, mUserAgentId);
}

std::string CCDBDownloader::getTimestampString(long timestamp) const
{
  stringstream ss;
  ss << timestamp;
  return ss.str();
}

std::string CCDBDownloader::getHostUrl(int hostIndex) const
{
  return hostsPool.at(hostIndex);
}

// todo make a single method of the one above and below
string CCDBDownloader::getFullUrlForRetrieval(CURL* curl, const string& path, const map<string, string>& metadata, long timestamp, int hostIndex) const
{
  if (mInSnapshotMode) {
    return getSnapshotFile(mSnapshotTopPath, path);
  }

  // Prepare timestamps
  string validityString = getTimestampString(timestamp < 0 ? getCurrentTimestamp() : timestamp);
  // Get host url
  string hostUrl = getHostUrl(hostIndex);
  // Build URL
  string fullUrl = hostUrl + "/" + path + "/" + validityString + "/";
  // Add metadata
  for (auto& kv : metadata) {
    string mfirst = kv.first;
    string msecond = kv.second;
    // trick for the metadata in case it contains special characters
    char* mfirstEncoded = curl_easy_escape(curl, mfirst.c_str(), mfirst.size());
    char* msecondEncoded = curl_easy_escape(curl, msecond.c_str(), msecond.size());
    fullUrl += string(mfirstEncoded) + "=" + string(msecondEncoded) + "/";
    curl_free(mfirstEncoded);
    curl_free(msecondEncoded);
  }
  return fullUrl;
}

void CCDBDownloader::initInSnapshotMode(std::string const& snapshotpath)
{
  mSnapshotTopPath = snapshotpath.empty() ? "." : snapshotpath;
  mInSnapshotMode = true;
}

void CCDBDownloader::loadFileToMemory(o2::pmr::vector<char>& dest, std::string const& path,
                               std::map<std::string, std::string> const& metadata, long timestamp,
                               std::map<std::string, std::string>* headers, std::string const& etag,
                               const std::string& createdNotAfter, const std::string& createdNotBefore, bool considerSnapshot,
                               size_t writeCallBack(void* contents, size_t size, size_t nmemb, void* chunkptr))
{
  LOGP(debug, "loadFileToMemory {} ETag=[{}]", path, etag);

  // if we are in snapshot mode we can simply open the file, unless the etag is non-empty:
  // this would mean that the object was is already fetched and in this mode we don't to validity checks!
  bool createSnapshot = considerSnapshot && !mSnapshotCachePath.empty(); // create snaphot if absent
  int fromSnapshot = 0;
  boost::interprocess::named_semaphore* sem = nullptr;
  std::string semhashedstring{}, snapshotpath{}, logfile{};
  std::unique_ptr<std::fstream> logStream;
  auto sem_release = [&sem, &semhashedstring, path, this]() {
    if (sem) {
      sem->post();
      if (sem->try_wait()) { // if nobody else is waiting remove the semaphore resource
        sem->post();
        boost::interprocess::named_semaphore::remove(semhashedstring.c_str());
      }
    }
  };

  if (createSnapshot) { // create named semaphore
    std::hash<std::string> hasher;
    semhashedstring = "aliceccdb" + std::to_string(hasher(mSnapshotCachePath + path)).substr(0, 16);
    try {
      sem = new boost::interprocess::named_semaphore(boost::interprocess::open_or_create_t{}, semhashedstring.c_str(), 1);
    } catch (std::exception e) {
      LOG(warn) << "Exception occurred during CCDB (cache) semaphore setup; Continuing without";
      sem = nullptr;
    }
    if (sem) {
      sem->wait(); // wait until we can enter (no one else there)
    }
    logfile = mSnapshotCachePath + "/log";
    logStream = std::make_unique<std::fstream>(logfile, ios_base::out | ios_base::app);
    if (logStream->is_open()) {
      *logStream.get() << "CCDB-access[" << getpid() << "] of " << mUserAgentId << " to " << path << " timestamp " << timestamp << " for load to memory\n"; // TODO check if the following is ok changed mUniqueAgent to mUserAgent
    }
  }

  if (mInSnapshotMode) { // file must be there, otherwise a fatal will be produced
    loadFileToMemory(dest, getSnapshotFile(mSnapshotTopPath, path), headers);
    fromSnapshot = 1;
  } else if (mPreferSnapshotCache && std::filesystem::exists(snapshotpath = getSnapshotFile(mSnapshotCachePath, path))) {
    // if file is available, use it, otherwise cache it below from the server. Do this only when etag is empty since otherwise the object was already fetched and cached
    if (etag.empty()) {
      loadFileToMemory(dest, snapshotpath, headers);
    }
    fromSnapshot = 2;
  } else {
    if (path.find("alien:/", 0) != std::string::npos) {
      std::cout << "Retrieving from alien\n";
      loadFileToMemory(dest, path, headers);
    } else if (path.find("file:/", 0) != std::string::npos) {
      std::cout << "Retrieving from local file\n";
      loadFileToMemory(dest, path, headers);
    }
    CURL* curl_handle = curl_easy_init();
    curl_easy_setopt(curl_handle, CURLOPT_USERAGENT, mUserAgentId.c_str());
    curl_slist* options_list = nullptr;
    initCurlHTTPHeaderOptionsForRetrieve(curl_handle, options_list, timestamp, headers, etag, createdNotAfter, createdNotBefore);

    scheduleFromRequest2(curl_handle, 0, path, metadata, timestamp, dest, writeCallBack);

    curl_slist_free_all(options_list);
    curl_easy_cleanup(curl_handle);
  }

  if (dest.empty()) {
    sem_release();
    return; // nothing was fetched: either cached value is good or error was produced
  }
  // !considerSnapshot means that the call was made by retrieve for snapshoting reasons
  logReading(path, timestamp, headers, fmt::format("{}{}", considerSnapshot ? "load to memory" : "retrieve", fromSnapshot ? " from snapshot" : ""));

  // are we asked to create a snapshot ?
  if (createSnapshot && fromSnapshot != 2 && !(mInSnapshotMode && mSnapshotTopPath == mSnapshotCachePath)) { // store in the snapshot only if the object was not read from the snapshot
    auto snapshotdir = getSnapshotDir(mSnapshotCachePath, path);
    snapshotpath = getSnapshotFile(mSnapshotCachePath, path);
    o2::utils::createDirectoriesIfAbsent(snapshotdir);
    if (logStream->is_open()) {
      *logStream.get() << "CCDB-access[" << getpid() << "] ... " << mUserAgentId << " downloading to snapshot " << snapshotpath << " from memory\n"; // TODO check if changing mUserAgentId to mUniqueAgentID is ok
    }
    { // dump image to a file
      LOGP(debug, "creating snapshot {} -> {}", path, snapshotpath);
      CCDBQuery querysummary(path, metadata, timestamp);
      {
        std::ofstream objFile(snapshotpath, std::ios::out | std::ofstream::binary);
        std::copy(dest.begin(), dest.end(), std::ostreambuf_iterator<char>(objFile));
      }
      // now open the same file as root file and store metadata
      updateMetaInformationInLocalFile(snapshotpath, headers, &querysummary);
    }
  }
  sem_release();
}

} // namespace o2
