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
#include "CCDB/CCDBTimeStampUtils.h"

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

namespace o2::ccdb
{

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
        // std::cout << "Redirect to: " << nextLocation << "\n";

        if (nextLocation.find("file:/", 0) != std::string::npos) {

        } else if (nextLocation.find("alien:/", 0) != std::string::npos) {
          std::cout << "Retrieving content from alien at " << nextLocation << "\n";
          void* item = downloadAlienContent(nextLocation, typeid(TObject));
          *data->objectPtr = item;
          if (item) {
            resultRetrieved = true;
            std::cout << "Succesfully retrieved\n";
          }
        } else {
          curl_easy_setopt(easy_handle, CURLOPT_URL, (data->hostUrl + nextLocation).c_str());
          mHandlesToBeAdded.push_back(easy_handle);
          nextDownloadScheduled = true;
        }
      }

      if (!nextDownloadScheduled) {
        deletePerformData(data, curlCode);
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

  // std::vector<std::string> locations;
  // for(int i = 0; i < headers.size(); i++) {
  //   if (headers[i].find("Content-Location") != std::string::npos) {
  //     locations.push_back(headers[i].substr(18));
  //   }
  // }

  // for(int i = 0; i < locations.size(); i++) {
  //   std::cout << "Location: " << locations[i];
  // }


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

CCDBDownloader::TransferResults* CCDBDownloader::batchRequestPerform(std::string host, std::vector<CURL*> const& handleVector)
{
  auto results = prepareResultsStruct(handleVector.size());
  std::map<CURL*, std::vector<std::string>*> locationsMap;

  for (int handleIndex = 0; handleIndex < handleVector.size(); handleIndex++) {
    auto* data = createPerformData(handleIndex, results, REQUEST);
    data->objectPtr = &results->objectPtr; // TODO Multiple objects !

    CURL* handle = handleVector[handleIndex];
    curl_easy_setopt(handle, CURLOPT_HEADERFUNCTION, WriteHeaderCallback);
    locationsMap[handle] = new std::vector<std::string>();
    curl_easy_setopt(handle, CURLOPT_HEADERDATA, locationsMap[handle]);
    data->locationsMap = &locationsMap;
    data->hostUrl = host;

    setHandleOptions(handleVector[handleIndex], data);
    mHandlesToBeAdded.push_back(handleVector[handleIndex]);
  }
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

CCDBDownloader::TransferResults* CCDBDownloader::scheduleFromRequest(std::string host, std::string url, std::string* dst, size_t (*writeCallback)(void*, size_t, size_t, std::string*))
{
  CURL* handle = curl_easy_init();
  curl_easy_setopt(handle, CURLOPT_WRITEFUNCTION, writeCallback);
  curl_easy_setopt(handle, CURLOPT_WRITEDATA, dst);
  curl_easy_setopt(handle, CURLOPT_URL, (host + url).c_str());
  auto userAgent = uniqueAgentID();
  curl_easy_setopt(handle, CURLOPT_USERAGENT, userAgent.c_str());

  std::vector<CURL*> handleVector;
  handleVector.push_back(handle);
  TransferResults* results = batchRequestPerform(host, handleVector);

  std::cout << "Curl code " << results->curlCodes[0] << "\n";
  long httpCode;
  curl_easy_getinfo(handle, CURLINFO_HTTP_CODE, &httpCode);
  std::cout << "Http code " << httpCode << "\n";

  curl_easy_cleanup(handle);
  return results;
}

void* CCDBDownloader::downloadAlienContent(std::string const& url, std::type_info const& tinfo) const
{
  if (!initTGrid()) {
    return nullptr;
  }
  // std::lock_guard<std::mutex> guard(gIOMutex);
  auto memfile = TMemFile::Open(url.c_str(), "OPEN");
  if (memfile) {
    auto cl = tinfo2TClass(tinfo);
    auto content = extractFromTFile(*memfile, cl, "o2::base::MatLayerCylSet"); // TODO REMOVE
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
    std::cout << "Connecting to grid\n";
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
    std::cout << "Certificate found\n";
    return true;
  } else {
    std::cout << "Alien token not found\n";
  }
  auto returncode = system("LD_PRELOAD= alien-token-info &> /dev/null");
  if (returncode == -1) {
    LOG(error) << "...";
  }
  return returncode == 0;
}

} // namespace o2
