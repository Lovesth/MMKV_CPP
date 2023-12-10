#include <iostream>
#include "mmkv.hpp"

class Test{
public:
    int a = 5;
    int b = 10;
    template<typename Archive>
    void serialize(Archive & ar, const unsigned int version){
        ar & a;
        ar & b;
    }
};

int main() {
    Test test;
    MMKV mmkv("mmkv_test");
    mmkv.initialize();
    //设置字符串value
    mmkv.setString("key1", "value");
    //设置整型value
    mmkv.setCommonValue("key2", 123321);
    //设置char型value
    mmkv.setCommonValue("key3", 'c');
    //设置float型value
    mmkv.setCommonValue("key4", 4523.1332);
    //设置bool型value
    mmkv.setCommonValue("key5", true);
    //设置自定义类型value，test是 class Test类型
    mmkv.setCommonValue("key6", test);


    //获取key1对应value，其中value是string类型
    std::cout << mmkv.get("key1", Data<std::string>()) << std::endl;
    //获取key2对应value，其中value是int类型
    std::cout << mmkv.get("key2", Data<int>()) << std::endl;
    //获取key4对应value，其中value是char类型
    std::cout << mmkv.get("key3", Data<char>()) << std::endl;
    //获取key4对应value，其中value是float类型
    std::cout << mmkv.get("key4", Data<float>()) << std::endl;
    //获取key4对应value，其中value是bool类型
    std::cout << mmkv.get("key5", Data<bool>()) << std::endl;
    //获取key5对应value，其中value是Test类型
    Test test1 = mmkv.get("key6", Data<Test>());
    std::cout << test1.a << " " << test1.b << std::endl;
    //删除key1对应的value
    mmkv.delete_("key1");
    //清空内存和文件
    mmkv.clear();
    return 0;
}