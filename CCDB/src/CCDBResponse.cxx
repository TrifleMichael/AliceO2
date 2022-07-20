#include <CCDB/CCDBResponse.h>

#include <string>
#include <vector>
#include <cstdio>
#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"
#include <algorithm>
#include <iostream>

using namespace rapidjson;

namespace o2
{
namespace ccdb
{

char* CCDBResponse::toString()
{
  return JsonToString(&document);
}

char* CCDBResponse::JsonToString(rapidjson::Document* document)
{
  rapidjson::StringBuffer buffer;
  buffer.Clear();
  rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
  (*document).Accept(writer);
  return strdup(buffer.GetString());
}

CCDBResponse::CCDBResponse(const std::string& jsonString)
{
  document.Parse(jsonString.c_str());
  refreshObjectNum();
  refreshIdHashmap();
}

void CCDBResponse::refreshObjectNum()
{
  objectNum = countObjects();
}

void CCDBResponse::refreshPathHashmap()
{
  for (int i = 0; i < objectNum; i++) {
    std::string path = getStringAttribute(i, "path");
    pathHashmap[path] = path;
  }
}

void CCDBResponse::refreshIdHashmap()
{
  for (int i = 0; i < objectNum; i++) {
    std::string id = getStringAttribute(i, "id");
    idHashmap[id] = id;
  }
}

int CCDBResponse::countObjects()
{
  auto objectsArray = document["objects"].GetArray();
  return objectsArray.Size();
}

bool CCDBResponse::mergeObjects(rapidjson::Value& dstObject, rapidjson::Value& srcObject, rapidjson::Document::AllocatorType& allocator)
{
  for (auto srcIt = srcObject.MemberBegin(); srcIt != srcObject.MemberEnd(); ++srcIt) {
    auto dstIt = dstObject.FindMember(srcIt->name);
    if (dstIt == dstObject.MemberEnd()) {
      rapidjson::Value dstName;
      dstName.CopyFrom(srcIt->name, allocator);
      rapidjson::Value dstVal;
      dstVal.CopyFrom(srcIt->value, allocator);

      dstObject.AddMember(dstName, dstVal, allocator);

      dstName.CopyFrom(srcIt->name, allocator);
      dstIt = dstObject.FindMember(dstName);
      if (dstIt == dstObject.MemberEnd()) {
        return false;
      }
    } else {
      auto srcT = srcIt->value.GetType();
      auto dstT = dstIt->value.GetType();
      if (srcT != dstT) {
        return false;
      }

      if (srcIt->value.IsArray()) {
        for (auto arrayIt = srcIt->value.Begin(); arrayIt != srcIt->value.End(); ++arrayIt) {
          rapidjson::Value dstVal;
          dstVal.CopyFrom(*arrayIt, allocator);
          dstIt->value.PushBack(dstVal, allocator);
        }
      } else if (srcIt->value.IsObject()) {
        if (!mergeObjects(dstIt->value, srcIt->value, allocator)) {
          return false;
        }
      } else {
        dstIt->value.CopyFrom(srcIt->value, allocator);
      }
    }
  }
  return true;
}

void CCDBResponse::removeObjects(rapidjson::Document* document, std::vector<bool> toBeRemoved)
{
  rapidjson::Value& objects = (*document)["objects"];
  if (objects.Size() > 1) {
    int i = 1;
    rapidjson::Value::ConstValueIterator pastObject = objects.Begin();
    rapidjson::Value::ConstValueIterator nextObject = pastObject + 1;
    while (pastObject != objects.End() && nextObject != objects.End()) {
      if (toBeRemoved[i]) {
        objects.Erase(nextObject);
      } else {
        pastObject++;
      }
      nextObject = pastObject + 1;
      i++;
    }
  }
  if (toBeRemoved[0]) {
    objects.Erase(objects.Begin());
  }
}

std::string CCDBResponse::getStringAttribute(int ind, std::string attributeName)
{
  if (ind < objectNum) {
    const char* attrNameChar = attributeName.c_str();
    return document["objects"][ind][attrNameChar].GetString();
  }
  return "";
}

long CCDBResponse::getLongAttribute(int ind, std::string attributeName)
{
  if (ind < objectNum) {
    const char* attrNameChar = attributeName.c_str();
    return document["objects"][ind][attrNameChar].GetUint64();
  }
  return -9999;
}

void CCDBResponse::browse(CCDBResponse* other)
{
  std::vector<bool> toBeRemoved(other->objectNum, false);
  for (int i = 0; i < other->objectNum; i++) {
    std::string id = other->getStringAttribute(i, "id");
    if (idHashmap.find(id) != idHashmap.end() && idHashmap[id].compare(id) == 0) {
      toBeRemoved[i] = true;
    }
  }
  removeObjects(other->getDocument(), toBeRemoved);
  refreshObjectNum();
}

void CCDBResponse::browseAndMerge(CCDBResponse* other)
{
  browse(other);
  mergeObjects(document, *(other->getDocument()), document.GetAllocator());
  refreshObjectNum();
}

void CCDBResponse::latestAndMerge(CCDBResponse* other)
{
  browse(other);
  other->refreshObjectNum();
  refreshPathHashmap();
  other->refreshPathHashmap();

  std::vector<bool> toBeRemoved(other->objectNum, false);
  for (int i = 0; i < other->objectNum; i++) {
    std::string path = other->getStringAttribute(i, "path");
    if (pathHashmap.find(path) != pathHashmap.end() && pathHashmap[path].compare(other->pathHashmap[path]) == 0) {
      toBeRemoved[i] = true;
    }
  }
  removeObjects(other->getDocument(), toBeRemoved);
  mergeObjects(document, *(other->getDocument()), document.GetAllocator());
  refreshObjectNum();
}

rapidjson::Document* CCDBResponse::getDocument()
{
  return &document;
}

void CCDBResponse::printObjectAttributes(rapidjson::Document* document)
{
  auto objectsArray = (*document)["objects"].GetArray();
  if (objectsArray.Size() > 0) {
    for (rapidjson::Value::ConstValueIterator object = objectsArray.begin(); object != objectsArray.end(); object++) {
      for (rapidjson::Value::ConstMemberIterator entry = object->MemberBegin(); entry != object->MemberEnd(); entry++) {
        auto& value = entry->value;
        auto name = entry->name.GetString();
        if (value.IsString()) {
          std::cout << name << " " << value.GetString() << std::endl;
        }
      }
      std::cout << "\n";
    }
  }
}

} // namespace ccdb
} // namespace o2
