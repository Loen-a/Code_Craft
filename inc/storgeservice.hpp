#ifndef STORGESERVICE_H
#define STORGESERVICE_H
#include <iostream>
#include <vector>
#include <map>
#include <memory>
#include <algorithm>
#include <math.h>
#include <deque>
#include <unordered_map>
#include <queue>
#include <set>
#include "data_class.h"


//lambda函数，谓词
struct DiskCompare {
    bool operator()(const std::pair<int, int>& a, const std::pair<int, int>& b) const {
        return (a.second > b.second) || (a.second == b.second && a.first < b.first);
    }
};

class StorgeManger{
public:

    StorgeManger(int t, int m, int n,int v,int g)
        :N(n),T(t),M(m),V(v),G(g),current_T(0)
    {
        numDisks.reserve(N);
        for (int i = 0; i < N; i++) {
            numDisks.emplace_back(new Disk(i, v));
            diskQueue.emplace(i, v); // 初始化堆
        }
    }
    //每个时间片开始刷新令牌
    void RefreshTokenCost() noexcept{
        for (auto &disk : numDisks) {
            disk->token_cost = G;
        }
    }
    bool WriteAction() noexcept
    {
        int n_write;
        scanf("%d", &n_write);
        //时间复杂度O(n^4)
        for(int i = 0; i < n_write; i++)
        {
            int id ,size, tag;
            scanf("%d%d%d", &id, &size, &tag);
            Write(id, size, tag);
        }
        return true;
    }
    bool Write(int id, int size, int tag) noexcept
    {
        Object* obj = new Object(id, size, tag);
        
        int diskindex = choose_disk(id, size, tag);
        //打印存放的对象id
        printf("%d\n", id);
   
        //时间复杂度O(n²)
        for(int i  = 0; i < REP_NUM; i++)
        {
            WriteObject(*obj, diskindex, i);
            diskindex = (diskindex+ 1) % N;
        }
        // numObjects.push_back(obj);
        numObjects[id] = obj;
        fflush(stdout);
        return true;
    }
    bool WriteObject(Object& obj, int diskindex, int copy_index) noexcept
    {
        //TODO
        obj.save_disk[copy_index] = diskindex;

        //第几个副本
        printf("%d ", diskindex + 1);
        int obj_size = obj.size;
        //时间复杂度O(n)
        for(int ceil = 1, j = 1; ceil <= V; ceil++)
        {
            if(j > obj_size)
            {
                break;
            }
            if( numDisks[diskindex]->zone[ceil] == false)
            {
                obj.chunks[copy_index][j] = ceil;
                numDisks[diskindex]->zone[ceil] = true;
                j++;
                printf(" %d", ceil);
            }
        }
        printf("\n");
        fflush(stdout);
        diskQueue.erase({diskindex,numDisks[diskindex]->free_space});
        numDisks[diskindex]->free_space -= obj_size;
        // 更新堆
        diskQueue.emplace(diskindex, numDisks[diskindex]->free_space);
        return true;
    }
    //找出剩余磁盘空间最大的磁盘
    int max_disk() noexcept
    {
        return diskQueue.empty() ? 0 : diskQueue.begin()->first;

    }
    // 组合哈希函数
    size_t combineHash(size_t lhs, size_t rhs) {
        return lhs ^ (rhs + 0x9e3779b9 + (lhs << 6) + (lhs >> 2));
    }
    // 计算 id, size, tag 的哈希
    size_t calculateHash(int id, size_t size, int tag) {
        std::hash<int> hashInt;
        size_t hashValue = hashInt(id);
        hashValue = combineHash(hashValue, hashInt(size));
        hashValue = combineHash(hashValue, hashInt(tag));

        return hashValue;
    }
    int choose_disk(int id, size_t size, int tag)
    {
        // 初步只计算对象ID的哈希
        int num_disks = N;
        int candidate = calculateHash(id ,size, tag) % num_disks;
        for(int i = 0; i < num_disks; i++)
        {
            int idx = (candidate + i) % num_disks;
            if(numDisks[idx]->free_space > 0)
            {
                return idx;
            }
        }
        return 0;
    }
    //更新请求队列
    int UpdateRequestList()
    {
        int before_size = numRequests.size();
        
        for (auto it = numRequests.begin(); it != numRequests.end();) {
            if (it->second->status == RequestStatus::Cancelled || 
                it->second->status == RequestStatus::Completed) {
                delete it->second; // 释放内存
                it = numRequests.erase(it); // **正确删除元素**
            } else {
                ++it;
            }
        }
    
        int after_size = numRequests.size();
        return before_size - after_size;
    }
    //更新对象队列
    int UpdateObjectList()
    {
        int before_size = numObjects.size();

        for (auto it = numObjects.begin(); it != numObjects.end();) {
            if (it->second->is_delete) {
                delete it->second; // 释放对象
                it = numObjects.erase(it); // **删除元素**
            } else {
                ++it;
            }
        }

        int after_size = numObjects.size();
        return before_size - after_size;
    }
    int SwapOvertimeRequest()
    {
        int count = 0;
        for(auto it = numRequests.begin(); it != numRequests.end();)
        {
            if(current_T - it->second->begin_time > overtime)
            {
                overtimeRequests[it->first] = it->second;
                ++ count;
                it = numRequests.erase(it);  // erase 返回下一个有效迭代器
            }else{
                ++it;
            }    
        }
        return count;
    }
    //读取事件
    void ReadACtion() noexcept
    {
        SwapOvertimeRequest();

        int n_read;
        scanf("%d", &n_read);
        //待修改
        // auto it = numRequests.begin();
        // if(it != numRequests.end() && current_T - it->second->begin_time > 10)
        // {
        //     numRequests.clear();
        // }
        for(int i = 0; i < n_read; i++)
        {
            int reqid, objid;
            scanf("%d%d", &reqid, &objid);
            if (numObjects.find(objid) != numObjects.end()) {
                numRequests[reqid] = new ReadRequestNode(reqid, objid, current_T);
            }
        }
        //删除过期请求
        UpdateRequestList();
        //处理请求
        int completed_count = 0;
        std::vector<int> completed_requests;
        //时间复杂度O(n²)  待优化
        for (auto& [reqid, request] : numRequests) {
            if (request->status == RequestStatus::InProgress) {
                if (ReadObject(numObjects[request->obj_id], reqid)) {
                    completed_requests.push_back(reqid);
                    completed_count++;
                }
            }
        }
        //时间复杂度O(n)
        for(int i = 0; i < numDisks.size(); i ++)
        {
            if(numDisks[i]->read_action[0] != 'j')
            {
                numDisks[i]->read_action.append("#");
            }
            
            printf("%s\n", numDisks[i]->read_action.c_str());
            numDisks[i]->read_action.clear();
        }
        printf("%d\n", completed_count);
        if(completed_count > 0)
        {
            for(int num : completed_requests)
            {
                printf("%d\n", num);
            }
        }
        fflush(stdout);

    }
    //读取块的消耗
    int ReadUnitCost(int diskindex, int index) noexcept
    {
        int cost = 0;
        if(numDisks[diskindex]->last_action == 'r')
        {
            cost = std::max(16, static_cast<int>(std::ceil(numDisks[diskindex]->last_cost * 0.8)));

        }else{
            cost = 64;
        }

        return cost;
    }
    //找到token消耗最少的副本编号
    int mincost(Object* obj, int chunk_index) noexcept
    {
        int disk_index = 0;
        int cost_temp = G;
        int copy_index = 0;
        //时间复杂度O(n)
        for(int j = 0; j < REP_NUM; j ++)
        {
            int cost = tokencost(obj, j, chunk_index);
            if(cost_temp > cost)
            {
                cost_temp = cost;
                copy_index = j;
            }
        }

        return copy_index;

    }
    //返回读取副本第一个块的总token消耗
    int tokencost(Object* obj, int copy_index, int chunk_index) noexcept
    {
        int save_disk = obj->save_disk[copy_index];
        int diskhead = numDisks[save_disk]->head_position;

        int distance = obj->chunks[copy_index][chunk_index] - diskhead;        //确定第一个块的token消耗
        int abs_dist = 0;
        int cost_temp = 0; 
        if(distance == 0){
            cost_temp = 64;     //刚好读
            return cost_temp;
        }
        if(distance < 0)
        {
            abs_dist = this->V + distance; //磁头位置大于块的位置，相当于旋转一圈
        }else{
            abs_dist = distance;
        }
        if(abs_dist >= numDisks[save_disk]->token_cost)
        {
            //包含两种情况，1：distance>0, 2:distance<0,两种都消耗G
            cost_temp = G;
        }else if(numDisks[save_disk]->token_cost - abs_dist < 64){
            cost_temp = G;
        }else{
            cost_temp = abs_dist + 64;
        }

        return cost_temp;
    }

    //读取对象
    //obj 对象指针，rep_index 对象副本编号，request_index请求编号
    bool ReadObject(Object* obj, int request_index) noexcept
    {

        int objid = obj->id;
        int objsize = obj->size;
        //待修改
        if(this->numRequests[request_index]->status == RequestStatus::Cancelled)
        {
        
            return false;
        }
 
        int already_read_units = this->numRequests[request_index]->already_read_units;
        //找到消耗最少的副本，进行读取
        int min_repindex = mincost(obj, already_read_units); 
        int save_disk = obj->save_disk[min_repindex];
        /*
        顺序读取对象块
        时间复杂度O(n)
        */
        for(int j = already_read_units; j <= objsize;)
        {
            int head = numDisks[save_disk]->head_position;
            //待修改
            if(numDisks[save_disk]->token_cost <= 0)
            {
                return false;
            }
            //找到位置，读取对象块
            if(head == obj->chunks[min_repindex][j])
            {
                int cost = ReadUnitCost(save_disk, j);
                if(numDisks[save_disk]->token_cost >= cost)
                {
                    numDisks[save_disk]->last_cost = cost;
                    numDisks[save_disk]->token_cost -= cost;
                    j++;
                    already_read_units += 1;
                    this->numRequests[request_index]->already_read_units +=1;
                    numDisks[save_disk]->head_position = (numDisks[save_disk]->head_position + 1) % (V+1);
                    numDisks[save_disk]->read_action.append("r");
                    numDisks[save_disk]->last_action = 'r';
                    continue;
                }else{
                    numDisks[save_disk]->token_cost = 0; //清零，下次读取，（待优化）
                    //费用不够，下次再读
                    // this->numRequests[request_index]->status = RequestStatus::InProgress;
                    break;
                }
            }else{
                //执行Jump 或者Pass
                int distance = obj->chunks[min_repindex][j] - head;
                int abs_dist = 0;
                if(distance < 0)
                {
                    abs_dist = this->V + distance;
                }else{
                    abs_dist = distance;
                }

                if(abs_dist >= numDisks[save_disk]->token_cost)
                {
                    if(numDisks[save_disk]->token_cost == G)
                    {
                        numDisks[save_disk]->token_cost = 0; //消耗
                        numDisks[save_disk]->head_position = obj->chunks[min_repindex][j];//磁头位置
                        numDisks[save_disk]->read_action.append("j ").append(std::to_string(obj->chunks[min_repindex][j]));
                        numDisks[save_disk]->last_action = 'j';
                    }else{
                        numDisks[save_disk]->token_cost = 0;
                    }

                }else if(numDisks[save_disk]->token_cost - abs_dist < 64){
                    if(numDisks[save_disk]->token_cost == G)
                    {
                        numDisks[save_disk]->token_cost = 0;//消耗
                        numDisks[save_disk]->head_position = obj->chunks[min_repindex][j];//磁头位置
                        numDisks[save_disk]->read_action.append("j ").append(std::to_string(obj->chunks[min_repindex][j]));
                        numDisks[save_disk]->last_action = 'j';
                    }else{
                        numDisks[save_disk]->token_cost = 0;
                    }

                }else{
                    std::string str(abs_dist, 'p');
                    numDisks[save_disk]->token_cost -= abs_dist ;//消耗
                    numDisks[save_disk]->head_position = obj->chunks[min_repindex][j];//磁头位置
                    numDisks[save_disk]->read_action.append(str);
                    numDisks[save_disk]->last_action = 'p';
                }
            }

        }
        //读取完毕
        if(already_read_units > objsize)
        {
            this->numRequests[request_index]->already_read_units = objsize;
            this->numRequests[request_index]->status = RequestStatus::Completed;
            return true;
        }
        return false;
    }

    //删除事件
    void DeleteAction()
    {
        int n_delet;
        scanf("%d", &n_delet);
        std::vector<int> cancel_id(n_delet, -1);
        for(int i = 0; i < n_delet; i++)
        {
            int objid;
            scanf("%d", &objid);
            cancel_id[i] = objid;
        }
        std::vector<int> cancelled_reqid;
        int n_abort = 0;
        //清除请求
        for (int objid : cancel_id) {
            if (numObjects.count(objid)) {
                for (auto& [reqid, request] : numRequests) {
                    if (request->obj_id == objid && request->status == RequestStatus::InProgress) {
                        request->status = RequestStatus::Cancelled;
                        cancelled_reqid.push_back(reqid);
                        n_abort++;
                    }
                }

                for (auto& [reqid, request] : overtimeRequests) {
                    if (request->obj_id == objid && request->status == RequestStatus::InProgress) {
                        request->status = RequestStatus::Cancelled;
                        cancelled_reqid.push_back(reqid);
                        n_abort++;
                    }
                }
                numObjects[objid]->is_delete = true;
            }
        }
        //恢复磁盘空间
        for (int objid : cancel_id) 
        {
            if(numObjects.count(objid))
            {
                for(int rep_index = 0; rep_index < REP_NUM; rep_index++)
                {
                    int diskindex = numObjects[objid]->save_disk[rep_index];
                    for(int i = 1; i <= numObjects[objid]->size; i ++)
                    {
                        int ceil = numObjects[objid]->chunks[rep_index][i];
                        numDisks[diskindex]->zone[ceil] = false;
                    }
                }
            }
        }

        
        printf("%d\n", n_abort);
        for (int id : cancelled_reqid) {
            printf("%d\n", id);
        }
        fflush(stdout);
        //清除被删除对象元数据
        UpdateObjectList();
    }

    void SetTime(int t)
    {
        current_T = t;
    }

    
    void timestamp_action()
    {
        int timestamp;
        scanf("%*s%d", &timestamp);
        printf("TIMESTAMP %d\n", timestamp);
        RefreshTokenCost();
        fflush(stdout);
    }

private:    
    std::unordered_map<int, Object*> numObjects;    //存储对象容器  
    std::vector<Disk*> numDisks;        //磁盘容器
    std::unordered_map<int, ReadRequestNode*> numRequests;  //请求队列
    std::unordered_map<int, ReadRequestNode*> overtimeRequests; //过期的队列
    std::set<std::pair<int, int>, DiskCompare> diskQueue;
    int overtime = 10;
    const int T;    //时间
    const int M;     //对象标签数
    const int N;    //磁盘个数
    const int V;    //磁盘容量
    const int G;    //每个时间的令牌数
    int current_T;  //当前的时间片编号
};



#endif //STORGESERVICE_H