#ifndef DATA_CLASS_H
#define DATA_CLASS_H
#include <iostream>
#include <vector>
#include <map>
#include <bitset>
#include <memory>
#include <algorithm>
#include <math.h>
#include <deque>
#include <unordered_map>
#include <queue>
#include <unordered_set>
#include <set>
#include <thread>
#include <mutex>
#include <future>
#include <vector>
#include <map>
#define MAX_DISK_NUM (10 + 1)
#define MAX_DISK_SIZE (16384 + 1)
#define MAX_REQUEST_NUM (30000000 + 1)
#define MAX_OBJECT_NUM (100000 + 1)
#define REP_NUM 3
#define FRE_PER_SLICING 1800
#define EXTRA_TIME 105
#define MAX_OBJECT_SIZE (5 + 1)
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
    int head_position;      //磁头位置
    int tag_usage;               // 主要存储的对象标签
    char last_action;            // 上次磁头动作 ('j', 'r', 'p')
    int last_cost;              //上次磁头读动作的消耗
    int token_cost;         //磁盘剩余令牌数，每个磁盘独立
    std::bitset<MAX_DISK_SIZE> bitmap; //记录每个空间使用情况
    std::string read_action;       //记录每个时间片磁头的所有行动，方便打印
};

class Object{
public:
    Object(int id, int size, int tag):
         id(id), size(size), tag(tag), save_disk(REP_NUM),start_pos(REP_NUM) , chunks(REP_NUM),is_delete(false){};

    bool is_delete;        //true 已被删除，false未被删除
    const int id;          //对象ID
    const int size;        //对象块数
    const int tag;         //对象标签
    std::vector<int> start_pos;          //第一个块的磁盘单元索引
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
    int selectd_replica;       //选择读取的副本编号，从0开始数
    int already_read_units;  //已经读取了几个对象块,从1开始数
    int estimated_token_cost;  //预估token消耗
    std::bitset<MAX_OBJECT_SIZE> obj_units;   //记录每个块的读取情况,从1开始数
    RequestStatus status;     //0:未完成，1：已完成，2：被取消
};
//按照对象分组的读取队列
class ReadRequestManager {
public:
    std::unordered_map<int, std::unordered_set<int>> obj_read_queues; // obj_id -> req_id 集合
    std::vector<int> obj_id_list;  // 存储所有 obj_id，后续排序使用
    std::mutex queue_mutex;
     // 添加请求
    void addRequest(int obj_id, int req_id) {
        std::lock_guard<std::mutex> lock(queue_mutex);
        
        // 如果 obj_id 是新出现的，加入 vector 进行排序
        if (obj_read_queues.find(obj_id) == obj_read_queues.end()) {
            obj_id_list.push_back(obj_id);
        }

        obj_read_queues[obj_id].insert(req_id);
    }
    // 取消请求
    void cancelRequest(int obj_id, int req_id) {
        std::lock_guard<std::mutex> lock(queue_mutex);
        auto it = obj_read_queues.find(obj_id);
        if (it != obj_read_queues.end()) {
            it->second.erase(req_id);
            if (it->second.empty()) {
                obj_read_queues.erase(it);
                // 移除 obj_id_list 里的 obj_id
                obj_id_list.erase(std::remove(obj_id_list.begin(), obj_id_list.end(), obj_id), obj_id_list.end());
            }
        }
    }
    // 读取成功后，所有未取消的请求都完成
    std::vector<int> markReadSuccess(int obj_id) {
        std::lock_guard<std::mutex> lock(queue_mutex);
        std::vector<int> completed_requests;
        auto it = obj_read_queues.find(obj_id);
        if (it != obj_read_queues.end()) {
            completed_requests.assign(it->second.begin(), it->second.end());
            obj_read_queues.erase(it);
            removeObjIdFromList(obj_id);
        }
        return completed_requests;
    }
    // 清除所有已完成的请求
    void clearCompletedRequests() {
        std::lock_guard<std::mutex> lock(queue_mutex);
        obj_id_list.erase(std::remove_if(obj_id_list.begin(), obj_id_list.end(),
                          [this](int obj_id) { return obj_read_queues.find(obj_id) == obj_read_queues.end(); }),
                          obj_id_list.end());
    }
    // 移除 obj_id_list 中的 obj_id
    void removeObjIdFromList(int obj_id) {
        obj_id_list.erase(std::remove(obj_id_list.begin(), obj_id_list.end(), obj_id), obj_id_list.end());
    }
    // 获取排序后的 obj_id 列表
    std::vector<int> getSortedObjIds(std::unordered_map<int, ReadRequestNode*>& numRequests) {
        std::lock_guard<std::mutex> lock(queue_mutex);
        std::vector<int> sorted_obj_ids = obj_id_list;
        std::sort(sorted_obj_ids.begin(), sorted_obj_ids.end(),
                [&](int reqId1, int reqId2) {
                    // 计算每个请求需要消耗的token
                    int tokenCost1 = 0;
                    int tokenCost2 = 0;
                    if(numRequests[reqId1] != nullptr)
                    {
                        tokenCost1 = numRequests[reqId1]->estimated_token_cost;
                    }
                    if(numRequests[reqId2] != nullptr)
                    {
                        tokenCost2 = numRequests[reqId2]->estimated_token_cost;
                    }
                    
                    // 按token消耗从大到小排序
                    return tokenCost1 > tokenCost2;
                }
            ); // 排序 obj_id
        return sorted_obj_ids;
    }
};
//标签的信息
class TagInfo{
public:
    TagInfo(int m, int i):tag(i)
    {
        total_delete.resize(m+1);
        total_write.resize(m+1);
        total_read.resize(m+1);
    }
    int tag;
    std::vector<int> total_delete;  //每个时间片的总delete数量从1开始，脚标0表示所有时间片总数
    std::vector<int> total_write;  //每个时间片的总write数量从1开始，脚标0表示所有时间片总数
    std::vector<int> total_read;  //每个时间片的总read数量从1开始，脚标0表示所有时间片总数
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