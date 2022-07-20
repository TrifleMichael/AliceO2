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
#include <unordered_map>
#include "rapidjson/document.h"

namespace o2
{
namespace ccdb
{

class CCDBResponse
{
 public:
  CCDBResponse(const std::string& jsonString);
  ~CCDBResponse() = default;

  char* JsonToString(rapidjson::Document *document);
  char* toString();
  int objectNum;
  std::unordered_map<std::string, std::string> idHashmap;
  std::unordered_map<std::string, std::string> pathHashmap;
  void refreshIdHashmap();
  void refreshPathHashmap();

  void browse(CCDBResponse* other);

  void removeObject(rapidjson::Document *document, int ind);
  int countObjects();
  bool mergeObjects(rapidjson::Value &dstObject, rapidjson::Value &srcObject, rapidjson::Document::AllocatorType &allocator);

  std::string getStringAttribute(int ind, std::string attributeName);
  long getLongAttribute(int ind, std::string attributeName);
  void latestAndMerge(CCDBResponse* other);
  void browseAndMerge(CCDBResponse* other);
  void removeObjects(rapidjson::Document *document, std::vector<bool> toBeRemoved);
  rapidjson::Document *getDocument();
  std::string sanitizeObjectName(const std::string& objectName);
  rapidjson::Document document; // should be moved to private
  void printObjectAttributes(rapidjson::Document *document);

 private:

  ClassDefNV(CCDBResponse, 1);
};

} // namespace ccdb
} // namespace o2

#endif
