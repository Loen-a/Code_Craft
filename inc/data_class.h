#ifndef DATA_CLASS_H
#define DATA_CLASS_H
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
        head_position = 0;
        tag_usage = 0;
        last_action = 'P';
        last_cost = 0;
        zone = std::string(v, '0');
        read_action = "";
        // std::cout<<zone<<std::endl;
        
    }
    const int id;           //磁盘ID
    const int storage;      //磁盘容量
    int free_space;         //磁盘剩余空间
    std::string zone;        //记录每个空间的状态
    int head_position;      //磁头位置
    int tag_usage;               // 主要存储的对象标签
    char last_action;            // 上次磁头动作 ('J', 'R', 'P')
    int last_cost;              //上次磁头读动作的消耗
    int token_cost;         //磁盘剩余令牌数，每个磁盘独立
    std::string read_action;
    // struct Request* pending_requests; // 读取请求队列
};

class Object{
public:
    Object(int id, int size, int tag):id(id), size(size), tag(tag){};

    // void save_chunks(int obj, int disk, int v)
    // {
    //     chunks[obj][disk] = v;
    // }
    bool is_delete;        //true 已被删除，false未被删除
    const int id;          //对象ID
    const int size;        //对象块数
    const int tag;         //对象标签
    // std::map<int, int> Object_chunks;   //对象块存储位置，
    std::vector<int> save_disk;          //各个对象副本存储在那个磁盘
    std::vector<std::map<int, int>> chunks;      //各个副本对象块存储位置 1：5，第一个块存在5号区
};
enum class RequestStatus { InProgress, Completed, Cancelled };
class ReadRequestNode{
public:
    ReadRequestNode(int reqid,int objid):req_id(reqid),obj_id(objid)
    {
        this->status = RequestStatus::InProgress;
        this->already_read_units = 0;
    }

    int begin_time;
    int req_id;
    int obj_id;
    int already_read_units;  //已经读取了几个对象块
    RequestStatus status;     //0:未完成，1：已完成，2：被取消
};

#endif //DATA_CLASS_H