#include <CCDB/CCDBResponse.h>

#include <string>
#include <vector>
#include <cstdio>
#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"
#include <algorithm>
#include <iostream> // debug
#include <chrono> // debug and optimization

using namespace rapidjson;

namespace o2
{
namespace ccdb
{

CCDBResponse::CCDBResponse(const std::string& jsonString)
{
  auto start = std::chrono::high_resolution_clock::now();
  document.Parse(jsonString.c_str());
  objectNum = countObjects();
  auto stop = std::chrono::high_resolution_clock::now();
  auto duration = std::chrono::duration_cast<std::chrono::microseconds>(stop - start);
  std::cout << std::endl;
  std::cout << "Parsing and counting objects: " << duration.count() << std::endl;

  std::cout << "\n\nObjects just after parsing: " << objectNum << std::endl;
  refreshIdHashmap();
}

void CCDBResponse::refreshPathHashmap()
{ 
  auto start2 = std::chrono::high_resolution_clock::now();  
  for(int i = 0; i < objectNum; i++)
  {
    std::string path = getStringAttribute(i, "path");
    pathHashmap[path] = path;
  }
  auto stop2 = std::chrono::high_resolution_clock::now();
  auto duration2 = std::chrono::duration_cast<std::chrono::microseconds>(stop2 - start2);
  std::cout << std::endl;
  std::cout << "Filling pathHashmap: " << duration2.count() << std::endl;
}

void CCDBResponse::refreshIdHashmap()
{ 
  auto start2 = std::chrono::high_resolution_clock::now();  
  for(int i = 0; i < objectNum; i++)
  {
    std::string id = getStringAttribute(i, "id");
    idHashmap[id] = id;
  }
  auto stop2 = std::chrono::high_resolution_clock::now();
  auto duration2 = std::chrono::duration_cast<std::chrono::microseconds>(stop2 - start2);
  std::cout << std::endl;
  std::cout << "Filling idHashmap: " << duration2.count() << std::endl;
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

void CCDBResponse::removeObject(rapidjson::Document *document, int ind)
{
    rapidjson::Value &objects = (*document)["objects"];
    if (objects.Size() > 0) {
        int i = 0;
        for (rapidjson::Value::ConstValueIterator object = objects.Begin(); object != objects.End(); object++) {
            if (i++ == ind) {
                objects.Erase(object);
                objectNum -= 1;
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
  int count = 0;
  for(int i = 0; i < toBeRemoved.size(); i++)
  {
    if (toBeRemoved[i]) count++;
  }
  std::cout << "\n\nTo be removed count: " << count << std::endl;

  auto start = std::chrono::high_resolution_clock::now();
  rapidjson::Value& objects = (*document)["objects"];
  if (objects.Size() > 1) {
    int i = 1;
    rapidjson::Value::ConstValueIterator pastObject = objects.Begin();
    rapidjson::Value::ConstValueIterator nextObject = pastObject + 1;
    while (pastObject != objects.End() && nextObject != objects.End())
    {
        if (toBeRemoved[i]) {
            objects.Erase(nextObject);
            nextObject = pastObject + 1;
            objectNum -= 1;
        } else {
            pastObject++;
            nextObject = pastObject + 1;
        }
        i++;
    }
  }
  if (toBeRemoved[0])
  {
    objects.Erase(objects.Begin());
    objectNum -= 1;
  }
  auto stop = std::chrono::high_resolution_clock::now();
  auto duration = std::chrono::duration_cast<std::chrono::microseconds>(stop - start);
  std::cout << std::endl;
  std::cout << "Remove objects duration: " << duration.count() << std::endl;
}

// Returns string attribute of object at a given index
std::string CCDBResponse::getStringAttribute(int ind, std::string attributeName)
{
    const char* attrNameChar = attributeName.c_str();
    if ( objectNum <= ind )
    {
        return "";
    }
    else
    {
        return document["objects"][ind][attrNameChar].GetString();
    }
    return "";
}

long CCDBResponse::getLongAttribute(int ind, std::string attributeName)
{
    const char* attrNameChar = attributeName.c_str();
    if ( objectNum <= ind )
    {
        // needs proper error handling
        return -9999;
    }
    else
    {
        return document["objects"][ind][attrNameChar].GetUint64();
    }
    return -9999;
}

void CCDBResponse::browse(CCDBResponse* other)
{
    std::cout << std::endl;
    std::cout << "This objectNum " << objectNum << ". That object num: " << other->objectNum << std::endl;
    std::cout << std::endl;

    auto start1 = std::chrono::high_resolution_clock::now();
    std::vector<bool> toBeRemoved(other->objectNum, false);
    for(int i = 0; i < other->objectNum; i++)
    {
        std::string id = other->getStringAttribute(i, "id");
        if (idHashmap.find(id) != idHashmap.end() && idHashmap[id].compare(id) == 0)
        {
            toBeRemoved[i] = true;
        }
    }
    auto stop1 = std::chrono::high_resolution_clock::now();
    auto duration1 = std::chrono::duration_cast<std::chrono::microseconds>(stop1 - start1);
    std::cout << std::endl;
    std::cout << "To remove create duration: " << duration1.count() << std::endl;

    removeObjects(other->getDocument(), toBeRemoved);
}

// Concatenates other response into this response according to browse
void CCDBResponse::browseAndMerge(CCDBResponse* other)
{
    browse(other);
    auto start2 = std::chrono::high_resolution_clock::now();
    mergeObjects(document, *(other->getDocument()), document.GetAllocator());
    auto stop2 = std::chrono::high_resolution_clock::now();
    auto duration2 = std::chrono::duration_cast<std::chrono::microseconds>(stop2 - start2);
    std::cout << std::endl;
    std::cout << "Merge objects duration: " << duration2.count() << std::endl;
    objectNum = countObjects();
}

// Concatenates other response into this response according to latest
void CCDBResponse::latestFromTwoServers(CCDBResponse* other)
{
    browse(other);
    other->objectNum = other->countObjects();
    refreshPathHashmap();
    other->refreshPathHashmap();

    std::vector<bool> toBeRemoved(other->objectNum, false);
    for(int i = 0; i < other->objectNum; i++)
    {
        std::string path = other->getStringAttribute(i, "path");
        if (pathHashmap.find(path) != pathHashmap.end() && pathHashmap[path].compare( other->pathHashmap[path] ) == 0)
        {
            toBeRemoved[i] = true;
        }
    }

    removeObjects(other->getDocument(), toBeRemoved);
    mergeObjects(document, *(other->getDocument()), document.GetAllocator());
    objectNum = countObjects();
}

rapidjson::Document *CCDBResponse::getDocument()
{
  return &document;
}

} // namespace ccdb
} // namespace o2
