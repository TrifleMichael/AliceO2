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

#ifndef CCDB_RESPONSE_H_
#define CCDB_RESPONSE_H_

#include <Rtypes.h>
#include <map>
#include <vector>
#include <string>
#include "rapidjson/document.h"

namespace o2
{
namespace ccdb
{



class CCDBResponse
{
 public:
  CCDBResponse() = default;
  CCDBResponse(const std::string &json);
  //CCDBResponse(rapidjson::Document _document);
  ~CCDBResponse() = default;

  const char *JsonToString(rapidjson::Document *document);
  void printObjectAttributes(rapidjson::Document *document);

  void removeObject(rapidjson::Document *document, int ind);
  int countObjects(rapidjson::Document *document);
  bool mergeObjects(rapidjson::Value &dstObject, rapidjson::Value &srcObject, rapidjson::Document::AllocatorType &allocator);

  std::string getStringAttribute(rapidjson::Document *document, int ind, std::string attributeName);
  long getLongAttribute(rapidjson::Document *document, int ind, std::string attributeName);
  void browse(rapidjson::Document *document);
  void latest(rapidjson::Document *document);
  void latestFromTwoServers(rapidjson::Document *documentFirst, rapidjson::Document *documentSecond);
  void removeObjects(rapidjson::Document *document, std::vector<bool> toBeRemoved);

  rapidjson::Document document; // should be moved to private

  // UNCOMMENT SUBFOLDERS IN CCDBAPI

 private:


  ClassDefNV(CCDBResponse, 1);
};

} // namespace ccdb
} // namespace o2

#endif
