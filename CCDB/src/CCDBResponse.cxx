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

std::vector<CCDBObjectDescription> CCDBResponse::getUniqueObjects()
{
  std::vector<int> unique;
  for(int i = 0; i < objects.size(); i++) {
    unique.push_back(1);
  }

  for(int i = 0; i < objects.size(); i++) {
    for(int j = i+1; j < objects.size(); j++) {
      if (objects[i].getProperty("id").compare(objects[j].getProperty("id")) == 0 && i != j) {
        unique[i] = 0;
        unique[j] = 0;
      }
    }
  }

  std::vector<CCDBObjectDescription> result;
  for(int i = 0; i < objects.size(); i++) {
    if (unique[i]) {
      result.push_back(objects[i]);
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
