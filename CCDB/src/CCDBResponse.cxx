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

// Concatenates otherResponse into this response according to  /latest/. this.objects take priority in case of duplicate id's
std::vector<CCDBObjectDescription> CCDBResponse::concatenateLatest(CCDBResponse otherResponse) {
  std::vector<CCDBResponse> newObjects = objects;
  newObjects.insert(objects.end(), otherResponse.getObjects.begin(), otherResponse.getObjects.end());
  newObjects = latestObjects();
}


// Concatenates otherResponse into this response according to  /browse/. this.objects take priority in case of duplicate id's
std::vector<CCDBObjectDescription> CCDBResponse::concatenateBrowse(CCDBResponse otherResponse) {
  std::vector<CCDBResponse> newObjects = objects;
  newObjects.insert(objects.end(), otherResponse.getObjects.begin(), otherResponse.getObjects.end());
  newObjects = browseObjects();
}

// Returns vector of objects of unique id's
std::vector<CCDBObjectDescription> CCDBResponse::browseObjects() {
  return CCDBResponse::browseObjects(objects);
}

std::vector<CCDBObjectDescription> CCDBResponse::browseObjects(std::vector<CCDBObjectDescription> objectsToParse) // TODO: Rapidjson variant without going through string
{
  std::vector<int> unique;
  for(int i = 0; i < objectsToParse.size(); i++) {
    unique.push_back(1);
  }

  for(int i = 0; i < objectsToParse.size(); i++) {
    for(int j = i+1; j < objectsToParse.size(); j++) {
      if (objectsToParse[i].getProperty("id").compare(objectsToParse[j].getProperty("id")) == 0) {
        unique[j] = 0;
      }
    }
  }

  std::vector<CCDBObjectDescription> result;
  for(int i = 0; i < objectsToParse.size(); i++) {
    if (unique[i]) {
      result.push_back(objectsToParse[i]);
    }
  }

  return result;
}

// Returns vector of latest objects for each directory path, only if unique ID
std::vector<CCDBObjectDescription> CCDBResponse::latestObjects() {
  return CCDBResponse::latestObjects(objects);
}

std::vector<CCDBObjectDescription> CCDBResponse::latestObjects(CCDBResponse otherResponse) // TODO: Rapidjson variant without going through string
{
  std::vector<int> latest;
  std::vector<CCDBObjectDescription> uniqueObjects = CCDBResponse::browseObjects(otherResponse);

  for(int i = 0; i < uniqueObjects.size(); i++) {
    latest.push_back(1);
  }

  for(int i = 0; i < uniqueObjects.size(); i++) {
    for(int j = i+1; j < uniqueObjects.size(); j++) {
      if (uniqueObjects[i].getProperty("path").compare(uniqueObjects[j].getProperty("path")) == 0) {
        if (uniqueObjects[i].getProperty("validFrom").compare(uniqueObjects[j].getProperty("validFrom")) >= 0) { // double check if the newest is taken
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

rapidjson::Document CCDBResponse::toRapidJsonDocument(std::vector<CCDBObjectDescription> descriptions)
{
  std::string descriptionsToString = "{\n";
  for(int i = 0; i < objects.size(); i++) {
    descriptionsToString += descriptions[i].toString();
  }
  descriptionsToString += "}";

  rapidjson::Document document;
  document.Parse(descriptionsToString.c_str());
  return document;
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

  auto subfoldersArray = jsonDocument["subfolders"].GetArray(); // creates array of subfolder paths
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
