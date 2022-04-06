#include <CCDB/CCDBResponse.h>

#include <string>
#include <vector>
#include <cstdio>
#include "rapidjson/document.h"
#include <algorithm>

namespace o2
{
namespace ccdb
{
std::vector<std::string> CCDBResponse::getSubFolders()
{
  return subfolders;
}

std::vector<CCDBObjectDescription> CCDBResponse::getObjects()
{
  return objects;
}

void CCDBResponse::concatenateBrowse(CCDBResponse other)
{
  auto otherObjects = other.getObjects();
  objects.insert(objects.end(), otherObjects.begin(), otherObjects.end()); // placeholder, should return not overwrite
  objects = browseObjects();
}

void CCDBResponse::concatenateLatest(CCDBResponse other)
{
  auto otherObjects = other.getObjects();
  objects.insert(objects.end(), otherObjects.begin(), otherObjects.end()); // placeholder, should return not overwrite
  objects = latestObjects();
}

std::vector<CCDBObjectDescription> CCDBResponse::browseObjects() {
  return browseObjects(objects);
}

// Returns vector of objects of unique id's
std::vector<CCDBObjectDescription> CCDBResponse::browseObjects(std::vector<CCDBObjectDescription> objectVector)
{
  std::vector<int> unique;
  for(int i = 0; i < objectVector.size(); i++) {
    unique.push_back(1);
  }

  for(int i = 0; i < objectVector.size(); i++) {
    for(int j = i+1; j < objectVector.size(); j++) {
      if (objectVector[i].getProperty("id").compare(objectVector[j].getProperty("id")) == 0) {
        unique[j] = 0;
      }
    }
  }

  std::vector<CCDBObjectDescription> result;
  for(int i = 0; i < objectVector.size(); i++) {
    if (unique[i]) {
      result.push_back(objectVector[i]);
    }
  }

  return result;
}

std::vector<CCDBObjectDescription> CCDBResponse::latestObjects() {
  return latestObjects(objects);
}

// Returns vector of latest objects for each directory path
std::vector<CCDBObjectDescription> CCDBResponse::latestObjects(std::vector<CCDBObjectDescription> objectVector)
{
  std::vector<int> latest;
  std::vector<CCDBObjectDescription> uniqueObjects = CCDBResponse::browseObjects(objectVector);

  for(int i = 0; i < uniqueObjects.size(); i++) {
    latest.push_back(1);
  }

  for(int i = 0; i < uniqueObjects.size(); i++) {
    for(int j = i+1; j < uniqueObjects.size(); j++) {
      if (uniqueObjects[i].getProperty("path").compare(uniqueObjects[j].getProperty("path")) == 0) {
        if (uniqueObjects[i].getProperty("validFrom").compare(uniqueObjects[j].getProperty("validFrom")) >= 0) {
          latest[j] = 0;
        }
      }
    }
  }

  std::vector<CCDBObjectDescription> result;
  for(int i = 0; i < uniqueObjects.size(); i++) {
    if (latest[i]) {
      result.push_back(uniqueObjects[i]);
    }
  }

  return result;
}


std::string CCDBObjectDescription::getProperty(const std::string& propertyName)
{
  return stringValues[propertyName];
}

CCDBResponse::CCDBResponse(const std::string& responseAsString)
{
  using namespace rapidjson;
  Document jsonDocument;
  jsonDocument.Parse(responseAsString.c_str()); 
  auto objectsArray = jsonDocument["objects"].GetArray(); // creates array of object descriptions
  if (objectsArray.Size() > 0) {
    for (Value::ConstValueIterator object = objectsArray.begin(); object != objectsArray.end(); object++) {
      objects.push_back(CCDBObjectDescription(object));
    }
  }

  auto subfoldersArray = jsonDocument["subfolders"].GetArray(); // creates array of subforlder paths
  if (subfoldersArray.Size() > 0) {
    for (Value::ConstValueIterator subfolder = subfoldersArray.begin(); subfolder != subfoldersArray.end(); subfolder++) {
      assert(subfolder->IsString());
      this->subfolders.push_back(subfolder->GetString());
    }
  }
}

CCDBObjectDescription::CCDBObjectDescription(rapidjson::Value::ConstValueIterator jsonObject)
{
  using namespace rapidjson;
  for (Value::ConstMemberIterator entry = jsonObject->MemberBegin(); entry != jsonObject->MemberEnd(); entry++) {
    auto& value = entry->value;
    auto name = entry->name.GetString();
    if (value.IsString()) {
      stringValues[name] = value.GetString();
    } else if (value.IsNumber()) {
      if (value.IsInt64()) {
        intValues[name] = value.GetInt64();
      } else if(value.IsDouble()) {
        doubleValues[name] = value.GetDouble();
      } else {
        //unserved number!
      }
    } else if(value.IsBool()) {
      booleanValues[name] = value.GetBool();
    } else {
      // unserved type!
    }
  }
}

std::string CCDBResponse::toString()
{
  return toString(objects);
}

std::string CCDBResponse::toString(std::vector<CCDBObjectDescription> descriptions)
{
  std::string descriptionsToString = "{\n";
  for(int i = 0; i < objects.size(); i++) {
    descriptionsToString += descriptions[i].toString();
  }
  descriptionsToString += "}";

  return descriptionsToString;
  // rapidjson::Document document;
  // document.Parse(descriptionsToString.c_str());
  // return document;
}

std::string CCDBObjectDescription::toString() // tested and works as intended
{
  std::string result = "";
  for (auto it = stringValues.begin(); it != stringValues.end(); it++)
  {
    result += "\"" + it->first + "\": \"" + it->second + "\",\n";
    // "first": "second",
  }
  if (!result.empty()) {
    result.pop_back();
    result.pop_back(); // removing trailing comma and newline
  }
  return result;
}






/**
 * Keep only the alphanumeric characters plus '_' plus '/' from the string passed in argument.
 * @param objectName
 * @return a new string following the rule enounced above.
 */
std::string CCDBResponse::sanitizeObjectName(const std::string& objectName)
{
  std::string tmpObjectName = objectName;
  tmpObjectName.erase(std::remove_if(tmpObjectName.begin(), tmpObjectName.end(),
                                     [](auto const& c) -> bool { return (!std::isalnum(c) && c != '_' && c != '/'); }),
                      tmpObjectName.end());
  return tmpObjectName;
}

} // namespace ccdb
} // namespace o2
