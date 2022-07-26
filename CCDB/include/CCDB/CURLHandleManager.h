// Copyright 2019-2021 CERN and copyright holders of ALICE O2.
// See https://alice-o2.web.cern.ch/copyright for details of the copyright holders.
// All rights not expressly granted are reserved.
//
// This software is distributed under the terms of the GNU General Public
// License v3 (GPL Version 3), copied verbatim in the file "COPYING".
//
// In applying this license CERN does not waive the privileges and immunities
// granted to it by virtue of its status as an Intergovernmental Organization
// or submit itself to any jurisdiction.

#ifndef CURL_HANDLE_MANAGER_H_
#define CURL_HANDLE_MANAGER_H_

#include <Rtypes.h>
#include <curl/curl.h>
#include <thread>
#include <string>

/*

https://curl.se/libcurl/c/threadsafe.html

Summary:
  - global_anyfunction are not safe, each must be called max once per program outside of CURLHandleManager
  - CURLOPT_NOSIGNAL must be set to 1 [curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1)] because signals are also not safe

TODO: 
  - Extending connection should be done during curl_easy_perform, and not durning getHandle();
  - Add a <std::string nameOfHandle, CURLHandleManager manager> map in CCDBApi in order to reuse handles
  
*/

namespace o2
{
namespace ccdb
{

class CURLHandleManager
{
 public:
  CURLHandleManager();
  ~CURLHandleManager();

  // Returns the managed CURL handle and extends its validity
  CURL* getHandle();

  // Describes the point when handle will be closed. It is postponed each time getHandle is called.
  std::chrono::time_point<std::chrono::steady_clock> expectedEndTime;

  void extendValidity(size_t seconds);

  size_t secondsOfBuffer = 3;
  double secondsUntilClosingHandle();

 private:

  CURL* curlHandle;
  std::thread *deleterThread;

  void sleepAndDelete();

  ClassDefNV(CURLHandleManager, 1);
};

} // namespace ccdb
} // namespace o2

#endif
