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

  char* toString();

  /**
   * Debug method. Print all attributes of all objects inside the document.
   */
  void printObjectAttributes(rapidjson::Document* document);

  /**
   * The number of objects inside the document.
   */
  int objectNum;

  /**
   * Return attribute in string type.
   *
   * @param ind - index of object inside document
   * @param attributeName - name of attribute to be retrieved
   */
  std::string getStringAttribute(int ind, std::string attributeName);

  /**
   * Return attribute in long type.
   *
   * @param ind - index of object inside document
   * @param attributeName - name of attribute to be retrieved
   */
  long getLongAttribute(int ind, std::string attributeName);

  /**
   * Merges objects with unique IDs and paths from another document into this one.
   *
   * @param other - Other CCDBResponse to be merged into this one.
   */
  void latestAndMerge(CCDBResponse* other);

  /**
   * Merges objects with unique IDs from another document into this one.
   *
   * @param other - Other CCDBResponse to be merged into this one.
   */
  void browseAndMerge(CCDBResponse* other);

 private:
  rapidjson::Document document;
  rapidjson::Document* getDocument();

  char* JsonToString(rapidjson::Document* document);

  void browse(CCDBResponse* other);
  void refreshObjectNum();
  int countObjects();
  void removeObjects(rapidjson::Document* document, std::vector<bool> toBeRemoved);
  bool mergeObjects(rapidjson::Value& dstObject, rapidjson::Value& srcObject, rapidjson::Document::AllocatorType& allocator);

  std::unordered_map<std::string, std::string> idHashmap;
  std::unordered_map<std::string, std::string> pathHashmap;

  void refreshIdHashmap();
  void refreshPathHashmap();

  ClassDefNV(CCDBResponse, 1);
};

} // namespace ccdb
} // namespace o2

#endif
