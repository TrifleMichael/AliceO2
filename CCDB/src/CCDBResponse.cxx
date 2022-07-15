#include <CCDB/CCDBResponse.h>

#include <string>
#include <vector>
#include <cstdio>
#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"
#include <algorithm>
#include <iostream> // debug

using namespace rapidjson;

namespace o2
{
namespace ccdb
{

CCDBResponse::CCDBResponse(const std::string& jsonString)
{
  document.Parse(jsonString.c_str());
}

char* CCDBResponse::JsonToString(rapidjson::Document *document)
{
  rapidjson::StringBuffer buffer;
  buffer.Clear();
  rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
  (*document).Accept(writer);
  return strdup(buffer.GetString());
}

char* CCDBResponse::toString()
{
  return JsonToString(&document);
}

// void CCDBResponse::printObjectAttributes(rapidjson::Document *document)
// {
//     auto objectsArray = (*document)["objects"].GetArray();
//     if (objectsArray.Size() > 0) {
//         for (rapidjson::Value::ConstValueIterator object = objectsArray.begin(); object != objectsArray.end(); object++) { // over objects
            
//             for (rapidjson::Value::ConstMemberIterator entry = object->MemberBegin(); entry != object->MemberEnd(); entry++) { // over attributes
//                 auto& value = entry->value;
//                 auto name = entry->name.GetString();
//                 if (value.IsString()) {
//                     std::cout << name << " " << value.GetString() << std::endl;
//                 }
//             }

//         }
//     }
// }

void CCDBResponse::removeObject(rapidjson::Document *document, int ind)
{
    rapidjson::Value &objects = (*document)["objects"];
    if (objects.Size() > 0) {
        int i = 0;
        for (rapidjson::Value::ConstValueIterator object = objects.Begin(); object != objects.End(); object++) {
            if (i++ == ind) {
                objects.Erase(object);
                return;
            }
        }
    }
}

int CCDBResponse::countObjects() 
{
    auto objectsArray = document["objects"].GetArray();
    return objectsArray.Size();
}

// Merges two rapidjson objects
bool CCDBResponse::mergeObjects(rapidjson::Value &dstObject, rapidjson::Value &srcObject, rapidjson::Document::AllocatorType &allocator)
{    
    for (auto srcIt = srcObject.MemberBegin(); srcIt != srcObject.MemberEnd(); ++srcIt)
    {
        auto dstIt = dstObject.FindMember(srcIt->name);
        if (dstIt == dstObject.MemberEnd())
        {
            rapidjson::Value dstName;
            dstName.CopyFrom(srcIt->name, allocator);
            rapidjson::Value dstVal;
            dstVal.CopyFrom(srcIt->value, allocator);

            dstObject.AddMember(dstName, dstVal, allocator);

            dstName.CopyFrom(srcIt->name, allocator);
            dstIt = dstObject.FindMember(dstName);
            if (dstIt == dstObject.MemberEnd())
                return false;
        }
        else
        {
            auto srcT = srcIt->value.GetType();
            auto dstT = dstIt->value.GetType();
            if(srcT != dstT)
                return false;

            if (srcIt->value.IsArray())
            {
                for (auto arrayIt = srcIt->value.Begin(); arrayIt != srcIt->value.End(); ++arrayIt)
                {
                    rapidjson::Value dstVal;
                    dstVal.CopyFrom(*arrayIt, allocator);
                    dstIt->value.PushBack(dstVal, allocator);
                }
            }
            else if (srcIt->value.IsObject())
            {
                if(!mergeObjects(dstIt->value, srcIt->value, allocator))
                    return false;
            }
            else
            {
                dstIt->value.CopyFrom(srcIt->value, allocator);
            }
        }
    }
    return true;
}

// Removes objects at indexes holding "true" value in the toBeRemoved vector.
void CCDBResponse::removeObjects(rapidjson::Document *document, std::vector<bool> toBeRemoved)
{
  int objectsIndex = 0;
  int length = (*document)["objects"].GetArray().Size();
  for (int removedListIndex = 0; removedListIndex < length; removedListIndex++) {
    if (toBeRemoved[removedListIndex]) {
      removeObject(document, objectsIndex);
    } else {
      objectsIndex++;
    }
  }
}

// Returns string attribute of object at a given index
std::string CCDBResponse::getStringAttribute(int ind, std::string attributeName)
{
    auto objArray = document["objects"].GetArray();
    const char* attrNameChar = attributeName.c_str();
    if ( objArray.Size() <= ind )
    {
        return "";
    }
    else
    {
        std::string arg = objArray[ind][attrNameChar].GetString();
        //delete attrNameChar; should be restored!!
        return arg;
    }
}

long CCDBResponse::getLongAttribute(int ind, std::string attributeName)
{
    auto objArray = document["objects"].GetArray();
    const char* attrNameChar = attributeName.c_str();
    if ( objArray.Size() <= ind )
    {
        // needs proper error handleing
        return -9999;
    }
    else
    {
        long attr = objArray[ind][attrNameChar].GetUint64();
        //delete attrNameChar; should be restored!!
        return attr;
    }
}

// Removes elements according to browse
void CCDBResponse::browse()
{
  auto objArray = document["objects"].GetArray(); // what about subfolders
  int length = objArray.Size();
  std::vector<bool> toBeRemoved(length, false);

  for (int i = 0; i < length; i++) {
    for (int j = i + 1; j < length; j++) {
      if (getStringAttribute(i, "id").compare(getStringAttribute(j, "id")) == 0) {
        toBeRemoved[j] = true;
      }
    }
  }
  removeObjects(&document, toBeRemoved);
}

// Removes elements according to latest
// Assumes document contains response from only one server
void CCDBResponse::latest()
{
    browse();

    auto objArray = document["objects"].GetArray(); // what about subfolders
    int length = objArray.Size();    
    std::vector<bool> toBeRemoved(length, false);

    for (int i = 0; i < length; i++)
    {
        for (int j = i + 1; j < length; j++)
        {
            if (getStringAttribute(i, "path").compare(getStringAttribute(j, "path")) == 0)
            {
                if (getLongAttribute(i, "createTime") > getLongAttribute(j, "createTime"))
                    toBeRemoved[j] = true;
                else
                    toBeRemoved[i] = true;
            }
        }
    }    
    removeObjects(&document, toBeRemoved);
}

// Concatenates other response into this response according to browse
void CCDBResponse::browseFromTwoServers(CCDBResponse* other)
{
    browse();
    other->browse();
    int thisLength = countObjects();
    int otherLength = other->countObjects();
    std::vector<bool> toBeRemoved(otherLength, false);

    for (int i = 0; i < thisLength; i++) {
        for (int j = 0; j < otherLength; j++) {
            if (getStringAttribute(i, "id").compare(getStringAttribute(j, "id")) == 0)
            {
                toBeRemoved[j] = true;
            }
        }
    }

    removeObjects(other->getDocument(), toBeRemoved);
    mergeObjects(document, *(other->getDocument()), document.GetAllocator());
}

// Concatenates other response into this response according to latest
void CCDBResponse::latestFromTwoServers(CCDBResponse* other)
{
    latest();
    other->latest();
    int thisLength = countObjects();
    int otherLength = other->countObjects();
    std::vector<bool> toBeRemoved(otherLength, false);

    for (int i = 0; i < thisLength; i++) {
        for (int j = 0; j < otherLength; j++) {
            if (getStringAttribute(i, "path").compare(other->getStringAttribute(j, "path")) == 0 
                || getStringAttribute(i, "id").compare(getStringAttribute(j, "id")) == 0)
            {
                toBeRemoved[j] = true;
            }
        }
    }

    removeObjects(other->getDocument(), toBeRemoved);
    mergeObjects(document, *(other->getDocument()), document.GetAllocator());
}

rapidjson::Document *CCDBResponse::getDocument()
{
  return &document;
}

} // namespace ccdb
} // namespace o2
