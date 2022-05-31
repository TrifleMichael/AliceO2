#include <CCDB/CCDBResponse.h>

#include <string>
#include <vector>
#include <cstdio>
#include "rapidjson/document.h"
#include <algorithm>

using namespace rapidjson;

namespace o2
{
namespace ccdb
{

CCDBResponse::CCDBResponse(const std::string& jsonString)
{
  document.Parse(jsonString.c_str());
}

// CCDBResponse::CCDBResponse(CCDBResponse::CCDBResponse& other)
// {
//   document.CopyFrom((*other).document, document.GetAllocator());
// }

// const char* CCDBResponse::JsonToString(rapidjson::Document *document)
// {
//   rapidjson::StringBuffer buffer;
//   buffer.Clear();
//   rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
//   (*document).Accept(writer);
//   return strdup( buffer.GetString() );
// }

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

void CCDBResponse::removeObjects(rapidjson::Document *document, std::vector<bool> toBeRemoved)
{
    int objectsIndex = 0;
    int length = (*document)["objects"].GetArray().Size();
    for (int removedListIndex = 0; removedListIndex < length; removedListIndex++)
    {
        if (toBeRemoved[removedListIndex])
        {
            removeObject(document, objectsIndex);
        }
        else
        {
            objectsIndex++;
        }
    }
}

std::string CCDBResponse::getStringAttribute(rapidjson::Document *document, int ind, std::string attributeName)
{
    auto objArray = (*document)["objects"].GetArray();
    const char* attrNameChar = attributeName.c_str();
    if ( objArray.Size() <= ind )
    {
        // needs proper error handleing
        return "ATTRIBUTE_NOT_FOUND";
    }
    else
    {
        return objArray[ind][attrNameChar].GetString();
    }
}

long CCDBResponse::getLongAttribute(rapidjson::Document *document, int ind, std::string attributeName)
{
    auto objArray = (*document)["objects"].GetArray();
    const char* attrNameChar = attributeName.c_str();
    if ( objArray.Size() <= ind )
    {
        // needs proper error handleing
        return -9999;
    }
    else
    {
        return objArray[ind][attrNameChar].GetUint64();
    }
}

// removes elements according to browse
void CCDBResponse::browse()
{
    auto objArray = document["objects"].GetArray(); // what about subfolders
    int length = objArray.Size();    
    std::vector<bool> toBeRemoved(length, false);

    for (int i = 0; i < length; i++)
    {
        for (int j = i + 1; j < length; j++)
        {
            if (getStringAttribute(&document, i, "id").compare(getStringAttribute(&document, j, "id")) == 0)
            {
                toBeRemoved[j] = true;
            }
        }
    }
    removeObjects(&document, toBeRemoved);
}

// removes elements according to latest
// assumes document contains response from only one server
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
            if (getStringAttribute(&document, i, "path").compare(getStringAttribute(&document, j, "path")) == 0)
            {
                if (getLongAttribute(&document, i, "createTime") > getLongAttribute(&document, j, "createTime"))
                    toBeRemoved[j] = true;
                else
                    toBeRemoved[i] = true;
            }
        }
    }    
    removeObjects(&document, toBeRemoved);
}

// void CCDBResponse::latestFromTwoServers(rapidjson::Document *documentFirst, rapidjson::Document *documentSecond)
// {
//     latest(documentFirst);
//     latest(documentSecond);
//     int firstLength = (*documentFirst)["objects"].GetArray().Size();
//     int secondLength = (*documentSecond)["objects"].GetArray().Size();
//     std::vector<bool> toBeRemoved(secondLength, false);

//     for (int i = 0; i < firstLength; i++) {
//         for (int j = 0; j < secondLength; j++) {
//             if (getStringAttribute(documentFirst, i, "path").compare(getStringAttribute(documentSecond, j, "path")) == 0)
//             {
//                 toBeRemoved[j] = true;
//             }
//         }
//     }

//     removeObjects(documentSecond, toBeRemoved);
//     mergeObjects(*documentFirst, *documentSecond, (*documentFirst).GetAllocator());
// }

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
