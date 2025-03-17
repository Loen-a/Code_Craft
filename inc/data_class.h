#include <iostream>
#include <vector>
#include <map>
#define MAX_DISK_NUM (10 + 1)
#define MAX_DISK_SIZE (16384 + 1)
#define MAX_REQUEST_NUM (30000000 + 1)
#define MAX_OBJECT_NUM (100000 + 1)
#define REP_NUM 3
#define FRE_PER_SLICING 1800
#define EXTRA_TIME 105

class Disk{
public:
    Disk(int id, int v):id(id), storage(v)
    {
        free_space = v;
        head_position = 1;
        tag_usage = 0;
        last_action = 'P';
        last_cost = 0;
        zone = std::string(v+1, '0');
        
    }

    const int id;           //磁盘ID
    const int storage;      //磁盘容量
    int free_space;         //磁盘剩余空间
    std::string zone;        //记录每个空间的状态
    int head_position;      //磁头位置
    int tag_usage;               // 主要存储的对象标签
    char last_action;            // 上次磁头动作 ('J', 'R', 'P')
    int last_cost;              //上次磁头读动作的消耗
    // struct Request* pending_requests; // 读取请求队列
};

class Object{
public:
    Object(int id, int size, int tag):id(id), size(size), tag(tag){};

    void save_chunks(int obj, int disk, int v)
    {
        chunks[obj][disk] = v;
    }

    const int id;          //对象ID
    const int size;        //对象块数
    const int tag;         //对象标签
    // std::map<int, int> Object_chunks;   //对象块存储位置，
    int save_disk;          //存储在那个磁盘
    std::vector<std::map<int, int>> chunks;      //各个副本对象块存储位置 1：5，第一个块存在5号区
};

class ReadRequestNode{
public:

private:
    
};