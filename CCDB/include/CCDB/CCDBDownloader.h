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
#ifndef O2_CCDBDOWNLOADER_H_
#define O2_CCDBDOWNLOADER_H_

#include "CCDB/CCDBQuery.h"

#include <cstdio>
#include <cstdlib>
#include <curl/curl.h>
#include <string>
#include <vector>
#include <iostream>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <unordered_map>
#include <map> // TODO maybe switch to unordered?

#include <memory>
#include <TObject.h>
#include <TFile.h>
#include <TTree.h>
#include <TH1F.h>
#include <TMessage.h>
#include <TGrid.h>
#include "CCDB/CcdbObjectInfo.h"
#include <CommonUtils/ConfigurableParam.h>
#include <type_traits>
#include <TMemFile.h>
#include "MemoryResources/MemoryResources.h" // TODO check if that has a reason to be moved



typedef struct uv_loop_s uv_loop_t;
typedef struct uv_timer_s uv_timer_t;
typedef struct uv_poll_s uv_poll_t;
typedef struct uv_signal_s uv_signal_t;
typedef struct uv_async_s uv_async_t;
typedef struct uv_handle_s uv_handle_t;

#if !defined(__CINT__) && !defined(__MAKECINT__) && !defined(__ROOTCLING__) && !defined(__CLING__)
#include <TJAlienCredentials.h>
#else
class TJAlienCredentials;
#endif

class TFile;
class TGrid;

using namespace std;

namespace o2::ccdb
{

// std::mutex gIOMutex; // to protect TMemFile IO operations // TODO here is good?

/*
 Some functions below aren't member functions of CCDBDownloader because both curl and libuv require callback functions which have to be either static or non-member.
 Because non-static functions are used in the functions below, they must be non-member.
*/

/**
 * uv_walk callback which is used to close passed handle.
 *
 * @param handle Handle to be closed.
 * @param arg Argument required by callback template. Is not used in this implementation.
 */
void closeHandles(uv_handle_t* handle, void* arg);

/**
 * Called by CURL in order to open a new socket. Newly opened sockets are assigned a timeout timer and added to socketTimerMap.
 *
 * @param clientp Pointer to the CCDBDownloader instance which controls the socket.
 * @param purpose Purpose of opened socket. This parameter is unused but required by the callback template.
 * @param address Structure containing information about family, type and protocol for the socket.
 */
curl_socket_t opensocketCallback(void* clientp, curlsocktype purpose, struct curl_sockaddr* address);

/**
 * Delete the handle.
 *
 * @param handle Handle assigned to this callback.
 */
void onUVClose(uv_handle_t* handle);

/// A class encapsulating and performing simple CURL requests in terms of a so-called CURL multi-handle.
/// A multi-handle allows to use a connection pool (connection cache) in the CURL layer even
/// with short-lived CURL easy-handles. Thereby the overhead of connection to servers can be
/// significantly reduced. For more info, see for instance https://everything.curl.dev/libcurl/connectionreuse.
///
/// Further, this class adds functionality on top
/// of simple CURL (aysync requests, timeout handling, event loop, etc).
class CCDBDownloader
{
 public:

  /**
   * Timer starts for each socket when its respective transfer finishes, and is stopped when another transfer starts for that handle.
   * When the timer runs out it closes the socket. The period for which socket stays open is defined by socketTimeoutMS.
   */
  std::unordered_map<curl_socket_t, uv_timer_t*> mSocketTimerMap;

  /**
   * The UV loop which handles transfers. Can be created internally or provided through a constructor.
   */
  uv_loop_t* mUVLoop;

  /**
   * Map used to store active uv_handles belonging to the CcdbDownloader. If internal uv_loop is used, then all uv_handles should be marked in this map.
   */
  std::unordered_map<uv_handle_t*, bool> mHandleMap;

  /**
   * Time for which sockets will stay open after last download finishes
   */
  int mKeepaliveTimeoutMS = 100;

  /**
   * Time for connection to start before it times out.
   */
  int mConnectionTimeoutMS = 60000;

  /**
   * Time for request to finish before it times out.
   */
  int mRequestTimeoutMS = 300000;

  /**
   * Head start of IPv6 in regards to IPv4.
   */
  int mHappyEyeballsHeadstartMS = 500;

  /**
   * Max number of handles that can be used at the same time
   */
  int mMaxHandlesInUse = 3;

  /**
   * Variable denoting whether an external or internal uv_loop is being used.
   */
  bool mExternalLoop;

  CCDBDownloader(uv_loop_t* uv_loop = nullptr);
  ~CCDBDownloader();

  /**
   * Perform on a single handle in a blocking manner. Has the same effect as curl_easy_perform().
   *
   * @param handle Handle to be performed on. It can be reused or cleaned after perform finishes.
   */
  CURLcode perform(CURL* handle);

  /**
   * Perform on a batch of handles in a blocking manner. Has the same effect as calling curl_easy_perform() on all handles in the vector.
   *
   * @param handleVector Handles to be performed on.
   */
  std::vector<CURLcode> batchBlockingPerform(std::vector<CURL*> const& handleVector);

  /**
   * Structure created for a batch of requests. Holds the information about current state of transfers from that batch.
   */
  typedef struct {
    std::vector<CURLcode> curlCodes;
    std::vector<bool> transferFinishedFlags;
    size_t requestsLeft;

    void* objectPtr;
  } TransferResults;

  /**
   * Schedules performing on a batch of handles. To perform run the mUVLoop or use getAll().
   *
   * @param handleVector Handles to be performed on.
   */
  TransferResults* batchAsynchPerform(std::vector<CURL*> const& handleVector);

  /**
   * Performs on a batch of handles, identified via transfer results. It's the equivalent of using future.get()
   *
   * @param transferResults Structure returned by batchAsynchPerform.
   */
  std::vector<CURLcode>::iterator getAll(TransferResults* transferResults);

  /**
   * Limits the number of parallel connections. Should be used only if no transfers are happening.
   */
  void setMaxParallelConnections(int limit);

  /**
   * Limits the time a socket and its connection will be opened after transfer finishes.
   */
  void setKeepaliveTimeoutTime(int timeoutMS);

  /**
   * Setter for the connection timeout.
   */
  void setConnectionTimeoutTime(int timeoutMS);

  /**
   * Setter for the request timeout.
   */
  void setRequestTimeoutTime(int timeoutMS);

  /**
   * Setter for the happy eyeballs headstart.
   */
  void setHappyEyeballsHeadstartTime(int headstartMS);

  /**
   * Sets the timeout values selected for the offline environment.
   */
  void setOfflineTimeoutSettings();

  /**
   * Sets the timeout values selected for the online environment.
   */
  void setOnlineTimeoutSettings();

  /**
   * Run the uvLoop once.
   *
   * @param noWait Using this flag will cause the loop to run only if sockets have pendind data.
   */
  void runLoop(bool noWait);

  TransferResults* batchRequestPerform(std::string host, std::vector<CURL*> const& handleVector); // TODO comment

  struct HeaderObjectPair_t { // TODO move
    std::map<std::string, std::string> header;
    o2::pmr::vector<char>* object = nullptr;
    int counter = 0;
  };

  TransferResults* scheduleFromRequest(std::string host, std::string url, o2::pmr::vector<char>& dst, size_t writeCallBack(void* contents, size_t size, size_t nmemb, void* chunkptr)); // TODO comment

  bool mInSnapshotMode = false;
  void loadFileToMemory(o2::pmr::vector<char>& dest, std::string const& path,
                        std::map<std::string, std::string> const& metadata, long timestamp,
                        std::map<std::string, std::string>* headers, std::string const& etag,
                        const std::string& createdNotAfter, const std::string& createdNotBefore, bool considerSnapshot,
                        size_t writeCallBack(void* contents, size_t size, size_t nmemb, void* chunkptr));

  void init(std::vector<std::string> hosts);
  void initInSnapshotMode(std::string const& snapshotpath);

 private:
  TransferResults* scheduleFromRequest2(CURL* handle, std::string url, o2::pmr::vector<char>& dst, size_t writeCallBack(void* contents, size_t size, size_t nmemb, void* chunkptr));
  std::string mUrl;
  // the failure to load the file to memory is signaled by 0 size and non-0 capacity
  static bool isMemoryFileInvalid(const o2::pmr::vector<char>& v) { return v.size() == 0 && v.capacity() > 0; }
  // internal helper function to update a CCDB file with meta information
  static void updateMetaInformationInLocalFile(std::string const& filename, std::map<std::string, std::string> const* headers, CCDBQuery const* querysummary = nullptr);
  // report what file is read and for which purpose
  void logReading(const std::string& path, long ts, const std::map<std::string, std::string>* headers, const std::string& comment) const;
  std::string getHostUrl(int hostIndex) const;
  std::vector<std::string> hostsPool{};

  /**
   * Transform and return a string representation of the given timestamp.
   *
   * @param timestamp
   * @return a string representation of the given timestamp.
   */
  std::string getTimestampString(long timestamp) const;


   /**
   * Build the full url to store an object.
   * @param path The path where the object is going to be found.
   * @param metadata Key-values representing the metadata for this object.
   * @param timestamp When the object we retrieve must be valid. If omitted or negative, the current timestamp is used.
   * @return The full url to store an object (url / startValidity / endValidity / [metadata &]* )
   */
  std::string getFullUrlForRetrieval(CURL* curl, const std::string& path, const std::map<std::string, std::string>& metadata,
                                     long timestamp = -1, int hostIndex = 0) const;


   /// initialize HTTPS header information for the CURL handle. Needs to be given an existing curl_slist* pointer to work with (may be nullptr), which needs to be free by the caller.
  void initCurlHTTPHeaderOptionsForRetrieve(CURL* curlHandle, curl_slist*& option_list, long timestamp, std::map<std::string, std::string>* headers, std::string const& etag, const std::string& createdNotAfter, const std::string& createdNotBefore) const;


  bool mPreferSnapshotCache;
  std::string snapshotReport;
  std::string mSnapshotTopPath{}; // todo
  std::string mSnapshotCachePath{}; // todo

  std::string getSnapshotDir(const std::string& topdir, const std::string& path) const { return topdir + "/" + path; } // todo
  std::string getSnapshotFile(const std::string& topdir, const std::string& path, const std::string& sfile = "snapshot.root") const // todo
  {
    return getSnapshotDir(topdir, path) + '/' + sfile;
  }

  constexpr static const char* CCDBMETA_ENTRY = "ccdb_meta"; // TODO comment
  constexpr static const char* CCDBQUERY_ENTRY = "ccdb_query";

  void loadFileToMemory(o2::pmr::vector<char>& dest, const std::string& path, std::map<std::string, std::string>* localHeaders); // TODO comment

  constexpr static const char* CCDBOBJECT_ENTRY = "ccdb_object"; // TODO comment

  bool mNeedAlienToken = true; // TODO comment

  mutable TGrid* mAlienInstance = nullptr; // TODO comment

  std::string mUserAgentId = "CCDBDownloader"; // TODO comment
  
  static bool checkAlienToken(); // TODO Comment

  void* downloadAlienContent(std::string const& url, std::type_info const& tinfo) const; // TODO comment

  static TClass* tinfo2TClass(std::type_info const& tinfo); // TODO comment

  static void* extractFromTFile(TFile& file, TClass const* cl, const char* what = CCDBOBJECT_ENTRY); // TODO comment

  std::vector<std::string> hosts; // TODO Check fix remove

  bool initTGrid() const; // TODO comment

  /**
   * Sets up internal UV loop.
   */
  void setupInternalUVLoop();

  /**
   * Current amount of handles which are performed on.
   */
  int mHandlesInUse = 0;

  /**
   * Multi handle which controlls all network flow.
   */
  CURLM* mCurlMultiHandle = nullptr;

  /**
   * The timeout clock that is be used by CURL.
   */
  uv_timer_t* mTimeoutTimer;

  /**
   * Queue of handles awaiting their transfers to start.
   */
  std::vector<CURL*> mHandlesToBeAdded;

  /**
   * Types of requests.
   */
  enum RequestType {
    BLOCKING,
    ASYNCHRONOUS,
    ASYNCHRONOUS_WITH_CALLBACK,
    REQUEST // TODO does that make sense?
  };

  /**
   * Information about a socket.
   */
  typedef struct curl_context_s {
    uv_poll_t* poll_handle;
    curl_socket_t sockfd = -1;
    CCDBDownloader* CD = nullptr;
  } curl_context_t;

  /**
   * Structure used for CURLMOPT_SOCKETDATA, which gives context for handleSocket
   */
  typedef struct DataForSocket {
    CCDBDownloader* CD;
    CURLM* curlm;
  } DataForSocket;

  DataForSocket mSocketData;

  /**
   * Structure which is stored in a easy_handle. It carries information about the request which the easy_handle is part of.
   */
  typedef struct PerformData {
    CURLcode* codeDestination;
    size_t* requestsLeft;
    RequestType type;
    vector<bool>::iterator transferFinished; // vector<bool> positions must be handled via iterators

    map<CURL*, vector<string>*>* locationsMap;
    int currentLocationIndex = -1;
    string hostUrl;
    void** objectPtr;
  } PerformData;

  /**
   * PerformData is stored within a CURL handle and is used to retrieve and modify information about the batch it belongs to.
   *
   * @param handleIndex Index of CURL handle for which the PerformData is created
   * @param results Structure holding the information about current state of the batch
   * @param requestType Type of request
   */
  CCDBDownloader::PerformData* createPerformData(uint handleIndex, TransferResults* results, RequestType requestType);

  /**
   * Creates an TransferResults stucture with default values.
   *
   * @param numberOfHandles Number of handles in batch.
   */
  TransferResults* prepareResultsStruct(size_t numberOfHandles);

  /**
   * Called by CURL in order to close a socket. It will be called by CURL even if a timeout timer closed the socket beforehand.
   *
   * @param clientp Pointer to the CCDBDownloader instance which controls the socket.
   * @param item File descriptor of the socket.
   */
  static void closesocketCallback(void* clientp, curl_socket_t item);

  /**
   * Is used to react to polling file descriptors in poll_handle.
   *
   * @param handle Handle assigned to this callback.
   * @param status Used to signal errors.
   * @param events Bitmask used to describe events on the socket.
   */
  static void curlPerform(uv_poll_t* handle, int status, int events);

  /**
   * Used by CURL to react to action happening on a socket.
   */
  static int handleSocket(CURL* easy, curl_socket_t s, int action, void* userp, void* socketp);

  /**
   * Close socket assigned to the timer handle.
   *
   * @param handle Handle which is assigned to this callback.
   */
  static void closeSocketByTimer(uv_timer_t* handle);

  /**
   * Start new transfers, terminate expired transfers.
   *
   * @param req Handle which is assigned to this callback.
   */
  static void curlTimeout(uv_timer_t* req);

  /**
   * Free curl context assigned to the handle.
   *
   * @param handle Handle assigned to this callback.
   */
  static void curlCloseCB(uv_handle_t* handle);

  /**
   * Close poll handle assigned to the socket contained in the context and free data within the handle.
   *
   * @param context Structure containing information about socket and handle to be closed.
   */
  static void destroyCurlContext(curl_context_t* context);

  /**
   * Connect curl timer with uv timer.
   *
   * @param multi Multi handle for which the timeout will be set
   * @param timeout_ms Time until timeout
   * @param userp Pointer to the uv_timer_t handle that is used for timeout.
   */
  static int startTimeout(CURLM* multi, long timeout_ms, void* userp);

  /**
   * Create a new multi_handle for the downloader
   */
  void initializeMultiHandle();

  /**
   * Release resources reserver for the transfer, mark transfer as complete, pass the CURLcode to the destination.
   *
   * @param handle The easy_handle for which the transfer completed
   * @param curlCode The code produced for the handle by the transfer
   */
  void transferFinished(CURL* handle, CURLcode curlCode);

  void deletePerformData(PerformData* data, CURLcode code); // TODO comment

  /**
   * Check message queue inside curl multi handle.
   */
  void checkMultiInfo();

  /**
   * Set openSocketCallback and closeSocketCallback with appropriate arguments. Stores data inside the CURL handle.
   */
  void setHandleOptions(CURL* handle, PerformData* data);

  /**
   * Create structure holding information about a socket including a poll handle assigned to it
   *
   * @param socketfd File descriptor of socket for which the structure will be created
   */
  curl_context_t* createCurlContext(curl_socket_t sockfd);

  /**
   * If multi_handles uses less then maximum number of handles then add handles from the queue.
   */
  void checkHandleQueue();
};

/**
 * Structure assigned to a uv_timer_t before adding it to socketTimerMap. It stores the information about the socket connected to the timer.
 */
typedef struct DataForClosingSocket {
  CCDBDownloader* CD;
  curl_socket_t socket;
} DataForClosingSocket;

} // namespace o2

#endif // O2_CCDB_CCDBDOWNLOADER_H
