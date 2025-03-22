#ifndef DATA_CLASS_H
#define DATA_CLASS_H
#include <iostream>
#include <vector>
#include <map>
#include <bitset>
#define MAX_DISK_NUM (10 + 1)
#define MAX_DISK_SIZE (16384 + 1)
#define MAX_REQUEST_NUM (30000000 + 1)
#define MAX_OBJECT_NUM (100000 + 1)
#define REP_NUM 3
#define FRE_PER_SLICING 1800
#define EXTRA_TIME 105

class Disk{
public:
    Disk(int id, int v):id(id), storage(v), free_space(v), head_position(1), tag_usage(0), 
          last_action('p'), last_cost(0), token_cost(0)
    {    
    }
    const int id;           //磁盘ID
    const int storage;      //磁盘容量
    int free_space;         //磁盘剩余空间
    // std::vector<bool> zone;        //记录每个空间的状态
    std::bitset<MAX_DISK_SIZE> bitmap; 
    int head_position;      //磁头位置
    int tag_usage;               // 主要存储的对象标签
    char last_action;            // 上次磁头动作 ('j', 'r', 'p')
    int last_cost;              //上次磁头读动作的消耗
    int token_cost;         //磁盘剩余令牌数，每个磁盘独立
    std::string read_action;       //记录每个时间片磁头的所有行动，方便打印
};

class Object{
public:
    Object(int id, int size, int tag):
         id(id), size(size), tag(tag), save_disk(REP_NUM), chunks(REP_NUM),is_delete(false){};

    bool is_delete;        //true 已被删除，false未被删除
    const int id;          //对象ID
    const int size;        //对象块数
    const int tag;         //对象标签
    // std::map<int, int> Object_chunks;   //对象块存储位置，
    std::vector<int> save_disk;          //各个对象副本存储在那个磁盘的
    std::vector<std::map<int, int>> chunks;      //各个副本对象块存储位置chunks[disk][unit]=n; 磁盘编号disk,对象块编号unit，磁盘单元编号n
};
enum class RequestStatus {Waitting, InProgress, Completed, Cancelled };
class ReadRequestNode{
public:
    ReadRequestNode(int reqid,int objid,int tag, int time):
            req_id(reqid),obj_id(objid),obj_tag(tag),begin_time(time)
    {
        this->status = RequestStatus::Waitting;
        this->already_read_units = 1; //第一块
    }

    int begin_time;         //读取请求的时间片
    int req_id;
    int obj_id;
    int obj_tag;
    int choose_disk;
    int already_read_units;  //已经读取了几个对象块
    int estimated_token_cost;  //预估token消耗
    RequestStatus status;     //0:未完成，1：已完成，2：被取消
};

// 优先级队列比较器，按estimated_token_cost排序
struct RequestCompare {
    bool operator()(const ReadRequestNode* a, const ReadRequestNode* b) const {
        // 优先级高的在队列顶部
        if (a->estimated_token_cost != b->estimated_token_cost)
            return a->estimated_token_cost > b->estimated_token_cost;
        // 如果估计消耗相同，优先处理等待时间更短的请求
        return a->begin_time < b->begin_time;
    }
};

#endif //DATA_CLASS_H