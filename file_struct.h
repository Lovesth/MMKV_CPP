//
// Created by xmh on 23-11-18.
//

#ifndef MMKV_FILE_STRUCT_H
#define MMKV_FILE_STRUCT_H

#include <string>
#include <map>
#include <list>
#include <cstdio>
#include <iostream>
#include <fstream>
#include <sstream>
#include <cstring>
#include <algorithm>
#include <unistd.h>
#include <sys/mman.h>
#include <boost/serialization/map.hpp>
#include <boost/serialization/list.hpp>
#include <boost/archive/binary_iarchive.hpp>
#include <boost/archive/binary_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/serialization/array.hpp>

//key固定是string
//value使用boost_serialization来序列化
//一个MMKV对象对应3个文件：filename（存储序列化的数据）
//filename.used（存储used_map）
//filename.list（存储used区域key的链表）

using offset = unsigned long long;
using mmkv_size = unsigned long long;
using mmkv_key = std::string;

struct Area{
    offset of1;
    offset of2;
    Area()=default;
    Area(offset o1, offset o2):of1(o1), of2(o2){};
};

//左闭右开
struct UsedContentArea{
    offset of1;
    offset of2;
    std::string valueType;
    UsedContentArea()=default;
    UsedContentArea(offset o1, offset o2):of1(o1), of2(o2){}
    UsedContentArea(offset o1, offset o2, std::string type):of1(o1), of2(o2), valueType(type){}
    template<typename Archive>
    void serialize(Archive & ar, const unsigned int version){
        ar & of1;
        ar & of2;
        ar & valueType;
    }
};

//左闭右开
struct FreeContentArea{
    offset of1;
    offset of2;
    FreeContentArea()=default;
    FreeContentArea(offset o1, offset o2):of1(o1), of2(o2){}
    template<typename Archive>
    void serialize(Archive & ar, const unsigned int version){
        ar & of1;
        ar & of2;
    }
};

struct UsedTable{
    std::map<std::string, UsedContentArea> used_table;
    template<typename Archive>
    void serialize(Archive & ar, const unsigned int version){
        ar & used_table;
    }
};

struct FreeTable{
    std::multimap<mmkv_size, FreeContentArea> free_table;
    template<typename Archive>
    void serialize(Archive & ar, const unsigned int version){
        ar & free_table;
    }
};

struct UsedAreaList{
    std::list<std::string> usedList;
    template<typename Archive>
    void serialize(Archive & ar, const unsigned int version){
        ar & usedList;
    }
};

struct StringData{
    std::string value;
    StringData(std::string&& d):value(std::move(d)){}
    StringData()= default;
    template<typename Archive>
    void serialize(Archive & ar, const unsigned int version){
        ar & value;
    }
};

template<typename T>
struct Data{
    T value;
    Data(T d):value(d){};
    Data()= default;
    template<typename Archive>
    void serialize(Archive & ar, const unsigned int version){
        ar & value;
    }
};

struct Tag{
    std::string tag;
    template<typename Archive>
    void serialize(Archive & ar, const unsigned int version){
        ar & tag;
    }
};

template<typename T>
std::string serializeData(T data);

#endif //MMKV_FILE_STRUCT_H
