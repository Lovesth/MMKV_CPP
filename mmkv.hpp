//
// Created by xmh on 23-11-17.
//

#ifndef MMKV_MMKV_HPP
#define MMKV_MMKV_HPP

#include "file_struct.h"

class MMKV {
private:
    std::string filename;
    void *fileHead;
    //fileSize、实际文件大小和内存映射长度始终保持一致
    mmkv_size fileSize;
    // MMKV Table
    UsedTable usedTable;
    UsedAreaList usedAreaList;
    bool iniFlag = false;
    //文件指针
    FILE *fp;
    //文件描述符
    int fd;
public:

    //构造函数
    MMKV() = default;

    //为了简化控制逻辑，禁止拷贝和赋值操作
    MMKV(const MMKV &m) = delete;

    MMKV &operator=(const MMKV &m) = delete;

    MMKV(MMKV &&m) = delete;

    MMKV &operator=(MMKV &&m) = delete;

    MMKV(const std::string &filename) : filename(filename) {};

    mmkv_size getFileSize();

    void setString(const mmkv_key &key, std::string value);

    template<typename T>
    void setCommonValue(const mmkv_key &key, T value);

    void setData(const mmkv_key &key, std::string &value);

    template<typename T>
    T get(const mmkv_key &key, Data<T> ret);

    bool find(const mmkv_key &key);

    void delete_(const mmkv_key &key);

    void clear();

    bool empty();

    void initialize();

    void mmkvSync();

    void reOrganizeMem();

    template<typename T>
    friend std::string serializeData(Data<T> data);

    //析构函数
    ~MMKV() { mmkvSync(); };
};

mmkv_size MMKV::getFileSize() {
    return fileSize;
}

bool MMKV::empty() {
    return usedTable.used_table.empty();
}


void MMKV::setString(const mmkv_key &key, std::string value) {
    if (!iniFlag)
        initialize();
    StringData data(std::move(value));
    std::string content = serializeData(data);
    setData(key, content);
}

template<typename T>
void MMKV::setCommonValue(const mmkv_key &key, T value) {
    if (!iniFlag)
        initialize();
    Data<T> data(value);
    std::string content = serializeData(data);
    setData(key, content);
}

//not completed
//a simplified version
void MMKV::setData(const mmkv_key &key, std::string &value) {
    mmkv_size valueLen = value.size();
    auto it = usedTable.used_table.find(key);
    if (it == usedTable.used_table.end()) {
        //key不在文件中
        if (usedAreaList.usedList.empty()) {
            munmap(fileHead, fileSize);
            fp = fopen((filename + ".mmkv").c_str(), "a+");
            fd = fileno(fp);
            fileSize = 2 * valueLen;
            ftruncate(fd, fileSize);
            fileHead = mmap(NULL, fileSize, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
            fclose(fp);
            //更新usedAreaList
            usedAreaList.usedList.push_back(key);
            //更新内存
            memcpy(fileHead, value.c_str(), valueLen);
            //更新usedTable
            usedTable.used_table.emplace(key, UsedContentArea(0, valueLen));
            mmkvSync();
            return;
        } else {
            auto lastKey = *usedAreaList.usedList.rbegin();
            auto lastIt = usedTable.used_table.find(lastKey);
            if (fileSize - lastIt->second.of2 >= valueLen) {
                //更新usedAreaList
                usedAreaList.usedList.push_back(key);
                //更新内存
                memcpy((void *) ((char *) fileHead + lastIt->second.of2), value.c_str(),
                       valueLen);
                //更新usedTable
                usedTable.used_table.emplace(key, UsedContentArea(lastIt->second.of2, lastIt->second.of2 + valueLen));
                mmkvSync();
                return;
            } else {
                munmap(fileHead, fileSize);
                fp = fopen((filename + ".mmkv").c_str(), "a+");
                fd = fileno(fp);
                fileSize = (2 * fileSize > fileSize + valueLen) ? 2 * fileSize : fileSize + valueLen;
                ftruncate(fd, fileSize);
                fileHead = mmap(NULL, fileSize, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
                fclose(fp);
                //更新usedAreaList
                usedAreaList.usedList.push_back(key);
                //更新内存
                memcpy((void *) ((char *) fileHead + lastIt->second.of2), value.c_str(),
                       valueLen);
                //更新usedTable
                usedTable.used_table.emplace(key, UsedContentArea(lastIt->second.of2, lastIt->second.of2 + valueLen));
                mmkvSync();
                return;
            }
        }
    } else {
        //key在文件中
        mmkv_size oldValueLen = it->second.of2 - it->second.of1;
        auto lastKey = *usedAreaList.usedList.rbegin();
        auto lastIt = usedTable.used_table.find(lastKey);
        if (valueLen > oldValueLen) {
            auto diff = valueLen - oldValueLen;
            auto diff1 = fileSize - lastIt->second.of2;
            if (diff1 < diff) {
                //扩容
                munmap(fileHead, fileSize);
                fp = fopen((filename + ".mmkv").c_str(), "a+");
                fd = fileno(fp);
                fileSize = (2 * fileSize > fileSize + diff) ? 2 * fileSize : fileSize + diff;
                ftruncate(fd, fileSize);
                fileHead = mmap(NULL, fileSize, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
                fclose(fp);
            }
            auto it0 = std::find(usedAreaList.usedList.begin(), usedAreaList.usedList.end(), key);
            it0++;
            auto nextIt = usedTable.used_table.find(*it0);
            char *tmp = new char[lastIt->second.of2 - nextIt->second.of1]();
            memcpy(tmp, (void *) ((char *) fileHead + nextIt->second.of1), lastIt->second.of2 - nextIt->second.of1);
            memcpy((void *) ((char *) fileHead + nextIt->second.of1 + diff), tmp,
                   lastIt->second.of2 - nextIt->second.of1);
            memcpy((void *) ((char *) fileHead + it->second.of1), value.c_str(), valueLen);
            it->second.of2 += diff;
            delete[] tmp;
            while (it0 != usedAreaList.usedList.end()) {
                auto it1 = usedTable.used_table.find(*it0);
                it1->second.of1 += diff;
                it1->second.of2 += diff;
                it0++;
            }
            return;
        } else if (valueLen < oldValueLen) {
            auto diff = oldValueLen - valueLen;
            auto it0 = std::find(usedAreaList.usedList.begin(), usedAreaList.usedList.end(), key);
            it0++;
            auto nextIt = usedTable.used_table.find(*it0);
            char *tmp = new char[lastIt->second.of2 - nextIt->second.of1]();
            memcpy(tmp, (void *) ((char *) fileHead + nextIt->second.of1), lastIt->second.of2 - nextIt->second.of1);
            memcpy((void *) ((char *) fileHead + nextIt->second.of1 - diff), tmp,
                   lastIt->second.of2 - nextIt->second.of1);
            memcpy((void *) ((char *) fileHead + it->second.of1), value.c_str(), valueLen);
            it->second.of2 += diff;
            delete[] tmp;
            while (it0 != usedAreaList.usedList.end()) {
                auto it1 = usedTable.used_table.find(*it0);
                it1->second.of1 -= diff;
                it1->second.of2 -= diff;
                it0++;
            }
            return;
        } else {
            memcpy((void *) ((char *) fileHead + it->second.of1), value.c_str(), valueLen);
            return;
        }
    }
}

template<typename T>
T MMKV::get(const mmkv_key &key, Data<T> ret) {
    if (!iniFlag)
        initialize();
    auto it = usedTable.used_table.find(key);
    if (it == usedTable.used_table.end()) {
        throw std::runtime_error("key doesn't exist in the file");
    }
    char *tmp = new char[it->second.of2 - it->second.of1 + 1]();
    memcpy(tmp, (char *) fileHead + it->second.of1, it->second.of2 - it->second.of1);
    std::string content;
    for (int i = 0; i < it->second.of2 - it->second.of1; i++)
        content += tmp[i];

    std::istringstream iss(content);
    boost::archive::text_iarchive ia(iss);
    ia >> ret;
    return ret.value;
}

bool MMKV::find(const mmkv_key &key) {
    auto it = usedTable.used_table.find(key);
    return it != usedTable.used_table.end();
}

void MMKV::delete_(const mmkv_key &key) {
    auto it = usedTable.used_table.find(key);
    if (it == usedTable.used_table.end())
        return;

    auto lastKey = *usedAreaList.usedList.rbegin();
    auto lastIt = usedTable.used_table.find(lastKey);
    auto diff = it->second.of2 - it->second.of1;
    auto it0 = std::find(usedAreaList.usedList.begin(), usedAreaList.usedList.end(), key);
    it0++;
    auto nextIt = usedTable.used_table.find(*it0);
    char *tmp = new char[lastIt->second.of2 - nextIt->second.of1]();
    memcpy(tmp, (void *) ((char *) fileHead + nextIt->second.of1),
           lastIt->second.of2 - nextIt->second.of1);
    memcpy((void *) ((char *) fileHead + it->second.of1), tmp,
           lastIt->second.of2 - nextIt->second.of1);
    delete[] tmp;
    while (it0 != usedAreaList.usedList.end()) {
        auto it1 = usedTable.used_table.find(*it0);
        it1->second.of1 -= diff;
        it1->second.of2 -= diff;
        it0++;
    }
    auto it1 = std::find(usedAreaList.usedList.begin(), usedAreaList.usedList.end(), key);
    usedAreaList.usedList.erase(it1);
    usedTable.used_table.erase(it);
    mmkvSync();
    return;
}

void MMKV::mmkvSync() {
    //更新used文件
    std::fstream fs1(filename + ".used", std::ios::out);
    boost::archive::text_oarchive oa1(fs1);
    oa1 << usedTable;
    fs1.close();

    //更新list文件
    std::fstream fs3(filename + ".list", std::ios::out);
    boost::archive::text_oarchive oa3(fs3);
    oa3 << usedAreaList;
    fs3.close();

    //同步内存和filename文件
    int r = msync(fileHead, fileSize, MS_SYNC);
    if (r == -1) {
        std::cerr << "msync failed" << std::endl;
        exit(-1);
    }
}

void MMKV::clear() {
    usedTable.used_table.clear();
    usedAreaList.usedList.clear();
    munmap(fileHead, fileSize);
    fileHead = nullptr;
    fileSize = 0;
    iniFlag = false;

    std::string usedFileName = filename + ".used";
    std::string listFileName = filename + ".list";

    FILE *fp1 = fopen(usedFileName.c_str(), "a+");
    int r1 = ftruncate(fileno(fp1), 0);
    FILE *fp3 = fopen(listFileName.c_str(), "a+");
    int r3 = ftruncate(fileno(fp3), 0);
    FILE *fp4 = fopen((filename + ".mmkv").c_str(), "a+");
    int r4 = ftruncate(fileno(fp4), 0);

    if (r1 == -1 || r3 == -1 || r4 == -1) {
        std::cerr << "clear failed" << std::endl;
        exit(-1);
    }
}

void MMKV::initialize() {
    //打开文件
    std::fstream fs(filename + ".mmkv", std::ios::in | std::ios::out | std::ios::app);
    if (!fs) {
        std::cerr << "fopen" << std::endl;
        exit(-1);
    }
    //获取文件字节数
    fs.seekg(0, std::ios::end);
    fileSize = fs.tellg();
    fs.seekg(0, std::ios::beg);

    if (fileSize == 0)
        return;

    std::fstream fs1(filename + ".used", std::ios::in | std::ios::out | std::ios::app);
    if (!fs1) {
        std::cerr << filename + ".used" << " doesn't exist" << std::endl;
        exit(-1);
    }
    boost::archive::text_iarchive ia1(fs1);
    ia1 >> usedTable;
    fs1.close();

    std::fstream fs2(filename + ".list", std::ios::in | std::ios::out | std::ios::app);
    if (!fs2) {
        std::cerr << filename + ".list" << " doesn't exist" << std::endl;
        exit(-1);
    }
    boost::archive::text_iarchive ia2(fs2);
    ia2 >> usedAreaList;
    fs2.close();
    fs.close();

    fp = fopen((filename + ".mmkv").c_str(), "a+");
    fd = fileno(fp);
    //将文件映射到内存
    fileHead = mmap(NULL, fileSize, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    fclose(fp);
    iniFlag = true;
}

template<typename T>
std::string serializeData(T data) {
    std::ostringstream oss;
    boost::archive::text_oarchive oa(oss);
    oa << data;
    std::string content = oss.str();
    return content;
}

#endif //MMKV_MMKV_HPP
