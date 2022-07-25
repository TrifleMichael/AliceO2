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

namespace o2
{
namespace ccdb
{

class CURLHandleManager
{
 public:
  CURLHandleManager();
  ~CURLHandleManager() = default;


 private:

  CURL* curl;

  ClassDefNV(CURLHandleManager, 1);
};

} // namespace ccdb
} // namespace o2

#endif
