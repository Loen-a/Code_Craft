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
#include <thread>
#include <mutex>
#include <future>
#include "data_class.h"


//lambda函数，谓词
struct DiskCompare {
    bool operator()(const std::pair<int, int>& a, const std::pair<int, int>& b) const {
        return (a.second > b.second) || (a.second == b.second && a.first < b.first);
    }
};

class StorgeManger{
private:    
    std::unordered_map<int, Object*> numObjects;    //存储对象容器  
    std::vector<Disk*> numDisks;        //磁盘容器
    std::unordered_map<int, ReadRequestNode*> numRequests;  //请求队列
    std::unordered_map<int, ReadRequestNode*> overtimeRequests; //过期的队列
    std::set<std::pair<int, int>, DiskCompare> diskQueue;
    std::vector<std::mutex> diskmutex;          //存放每个磁盘的mutex
    std::mutex ReqestMutex;     //读取队列锁
    std::condition_variable cv;       // 条件变量
    std::atomic<bool> isheadmove{false};
    std::atomic<bool> threadrunning{false};
    std::thread prirequest;
    std::priority_queue<ReadRequestNode*, std::vector<ReadRequestNode*>, RequestCompare> MinCostQueue;
    int disktokenisnull = 0;            //记录是否所有磁盘的token都消耗完毕
    int overtime = 5;
    const int T;    //时间
    const int M;     //对象标签数
    const int N;    //磁盘个数
    const int V;    //磁盘容量
    const int G;    //每个时间的令牌数
    int current_T;  //当前的时间片编号
public:

    StorgeManger(int t, int m, int n,int v,int g)
        :N(n),T(t),M(m),V(v),G(g),current_T(0)
    {
        numDisks.reserve(N);
        diskmutex = std::vector<std::mutex>(N);
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
        disktokenisnull = 0;
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
        
        // int diskindex = choose_disk(id, size, tag);
        //打印存放的对象id
        printf("%d\n", id);

        std::set<int> selectdisk;
        int hashindex = 0;
        while(selectdisk.size() < REP_NUM)
        {
            int rep_index = choose_disk(hashindex++, size, tag);
            if(!selectdisk.count(rep_index))
            {
                selectdisk.insert(rep_index);
            }
        }
        //时间复杂度O(n²)
        int rep_index = 0;
        for(auto& index : selectdisk)
        {
            if(rep_index < REP_NUM)
            {
                WriteObject(*obj, index, rep_index++);
            }

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
        printf("%d", diskindex + 1);
        int obj_size = obj.size;

        int start_pos = findContinuousBlocks(diskindex, obj_size);
        if (start_pos > 0) {
        // 找到连续块，进行连续分配
            for (int j = 1; j <= obj_size; j++) {
                obj.chunks[copy_index][j] = start_pos + j - 1;
                numDisks[diskindex]->bitmap[start_pos + j - 1] = true;
                printf(" %d", start_pos + j - 1);
            }
         }else {
            // 时间复杂度O(n)
            int block = 1;
            int maxdisk = max_disk();
            obj.save_disk[copy_index] = diskindex;
            for(int j = 1; block <= V; block++)
            {
                if(j > obj_size)
                {
                    break;
                }
                if( numDisks[diskindex]->bitmap[block] == false)
                {
                    obj.chunks[copy_index][j] = block;
                    numDisks[diskindex]->bitmap[block] = true;
                    j++;
                    printf(" %d", block);
                }
            }
        }

        printf("\n");
        fflush(stdout);
        diskQueue.erase({diskindex,numDisks[diskindex]->free_space});
        numDisks[diskindex]->free_space -= obj_size;
        // numDisks[diskindex]->free_space = numDisks[diskindex]->bitmap.size() - numDisks[diskindex]->bitmap.count();
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
        // 计算对象ID,size,tag 三个属性的哈希
        int num_disks = N;
        int candidate = calculateHash(id,size, tag) % num_disks;
        for(int i = 0; i < num_disks; i++)
        {
            int idx = (candidate + i) % num_disks;
            if(numDisks[idx]->free_space > size)
            {
                return idx;
            }
        }
        return 0;
    }
    //找出符合对象大小的连续块
    int findContinuousBlocks(int diskindex, int size) {
        int continuous_count = 0;
        int start_pos = -1;
        for (int i = 1; i <= V; i++) {
            if (!numDisks[diskindex]->bitmap[i]) {
                if (continuous_count == 0) {
                    start_pos = i;
                }
                continuous_count++;
                
                if (continuous_count >= size) {
                    return start_pos;
                }
            } else {
                continuous_count = 0;
            }
        }
        return -1; // 没找到足够的连续块，待优化
    }
    //更新请求队列
    int UpdateNewRequestList()
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
    int UpdateOverTimeRequestList()
    {
        int before_size = overtimeRequests.size();
        for (auto it = overtimeRequests.begin(); it != overtimeRequests.end();) {
            if (it->second->status == RequestStatus::Cancelled || 
                it->second->status == RequestStatus::Completed) {
                delete it->second; // 释放内存
                it = overtimeRequests.erase(it); // **正确删除元素**
            } else {
                ++it;
            }
        }
        int after_size = overtimeRequests.size();
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
        //移除过期请求
        for(auto it = numRequests.begin(); it != numRequests.end();)
        {
            if(current_T - it->second->begin_time > overtime && it->second->status != RequestStatus::InProgress)
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
    int mincost(int obj_id, int chunk_index) noexcept
    {
        int disk_index = 0;
        int cost_temp = G;
        int copy_index = 0;
        //时间复杂度O(n)
        for(int j = 0; j < REP_NUM; j ++)
        {
            int cost = tokencost(numObjects[obj_id], j, chunk_index);
            if(cost_temp > cost)
            {
                cost_temp = cost;
                copy_index = j;
            }
        }

        return copy_index;

    }
    //返回读取副本第一个块的token消耗
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
        //综合计算读取所有块的成本
        int jumps = 0;
        for (int j = chunk_index + 1; j <= obj->size; j++) {
                if (obj->chunks[copy_index][j] != obj->chunks[copy_index][j-1] + 1) {
                    jumps++;
                }
            }
        // 综合考虑移动成本和连续性
        int total_cost = cost_temp + jumps * 100; // 跳跃惩罚
        return total_cost;
    }
    //读取事件
    void ReadAction() noexcept
    {
        //删除过期请求
        UpdateNewRequestList();
        UpdateOverTimeRequestList();
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
                numRequests[reqid] = new ReadRequestNode(reqid, objid,numObjects[objid]->tag, current_T);
            }
        }
        // 按对象分组请求，优化读取顺序
        // std::unordered_map<int, std::vector<int>> requests_by_objid;
        // for (auto& [reqid, request] : numRequests) {
        //     if (request->status == RequestStatus::InProgress||request->status == RequestStatus::Waitting) {
        //         requests_by_objid[request->obj_id].push_back(reqid);
        //     }
        // }
   
        //本次时间片处理完成的请求
        int completed_count = 0;
        std::vector<int> completed_requests;
        // // 优先处理接近完成的请求
        // for(auto& [objid, request_ids] : requests_by_objid)
        // {
        //     std::sort(request_ids.begin(), request_ids.end(), 
        //                 [this](int a, int b) {
        //                     return numRequests[a]->already_read_units > numRequests[b]->already_read_units;
        //             });
        // } 
        // //将之前未读完的对象，先读取
        // std::unordered_map<int, std::vector<int>> InprogressRequest;
        // for(auto it = requests_by_objid.begin(); it != requests_by_objid.end();it++)
        // {
        //     if(it->second.size() > 0){
        //         if(numRequests[it->second[0]]->already_read_units > 0)
        //         {
        //             auto node = requests_by_objid.extract(it);
        //             InprogressRequest.insert(std::move(node));
        //         }
        //     }

        // }
        //时间复杂度O(n²)  待优化
        // for (auto& [reqid, request] : numRequests) {
        //     if (request->status == RequestStatus::InProgress || request->status == RequestStatus::Waitting) {
        //         if (ReadObject(numObjects[request->obj_id], reqid)) {
        //             completed_requests.push_back(reqid);
        //             completed_count++;
        //         }
        //     }
        // }
        //最小堆处理读请求
        for(int i = 0; i < MinCostQueue.size(); i++)
        {
            if(disktokenisnull >= N)
            {
                //所有磁盘toen都消耗完毕
                break;
            }
            std::unique_lock<std::mutex> lock(ReqestMutex);
            auto& request = MinCostQueue.top();
            MinCostQueue.pop();
            if(request->status == RequestStatus::Waitting || request->status == RequestStatus::InProgress)
            {
                if (ReadObject(std::ref(numObjects[request->obj_id]), request->req_id)) {
                    completed_requests.push_back(request->req_id);
                    completed_count++;
                }
            }
            
        }
        //将之前未读完的对象，先读取
        // for(auto& [objid, requestid] : InprogressRequest)
        // {
        //     for(int i = 0; i < requestid.size(); i++)
        //     {
        //         if (numRequests[requestid[i]]->status == RequestStatus::InProgress || numRequests[requestid[i]]->status == RequestStatus::Waitting) {
        //             if (ReadObject(numObjects[numRequests[requestid[i]]->obj_id], requestid[i])) {
        //                 completed_requests.push_back(requestid[i]);
        //                 completed_count++;
        //             }
        //         }
        //     }
        // }
        // for(auto& [objid, requestid] : requests_by_objid)
        // {
        //     if(disktokenisnull >= N)
        //     {
        //         break;
        //     }
        //     for(int i = 0; i < requestid.size(); i++)
        //     {
        //         if(disktokenisnull >= N)
        //         {
        //             break;
        //         }
        //         //token消耗完，仍然在循环，待优化
        //         if (numRequests[requestid[i]]->status == RequestStatus::InProgress || numRequests[requestid[i]]->status == RequestStatus::Waitting) {
        //             if (ReadObject(numObjects[numRequests[requestid[i]]->obj_id], requestid[i])) {
        //                 completed_requests.push_back(requestid[i]);
        //                 completed_count++;
        //             }
        //         }
        //     }
        // }
        //时间复杂度O(n)
        //返回磁盘移动结果
        for(int i = 0; i < numDisks.size(); i ++)
        {
            if(numDisks[i]->read_action[0] != 'j')
            {
                numDisks[i]->read_action.append("#");
            }
            
            printf("%s\n", numDisks[i]->read_action.c_str());
            numDisks[i]->read_action.clear();
        }

        //返回完成结果
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

    //读取对象
    //obj 对象指针，rep_index 对象副本编号，request_index请求编号
    bool ReadObject(Object* obj, int request_index) noexcept
    {

        int objid = obj->id;
        int objsize = obj->size;
        int already_read_units = this->numRequests[request_index]->already_read_units;
        //找到消耗最少的副本，进行读取
        int min_repindex = mincost(objid, already_read_units); 
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
                // HeadSet(true);
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
                    this->numRequests[request_index]->status =RequestStatus::InProgress;
                    numDisks[save_disk]->head_position = (numDisks[save_disk]->head_position + 1) % (V+1);
                    numDisks[save_disk]->read_action.append("r");
                    numDisks[save_disk]->last_action = 'r';
                    continue;
                }else{
                    numDisks[save_disk]->token_cost = 0; //清零，下次读取，（待优化）
                    disktokenisnull +=1;
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
                        numDisks[save_disk]->token_cost = 0;        //清空token下次读
                        disktokenisnull +=1;
                    }

                }else if(numDisks[save_disk]->token_cost - abs_dist < 64){
                    if(numDisks[save_disk]->token_cost == G)
                    {
                        numDisks[save_disk]->token_cost = 0;//消耗
                        numDisks[save_disk]->head_position = obj->chunks[min_repindex][j];//磁头位置
                        numDisks[save_disk]->read_action.append("j ").append(std::to_string(obj->chunks[min_repindex][j]));
                        numDisks[save_disk]->last_action = 'j';
                    }else{
                        numDisks[save_disk]->token_cost = 0; //清空token下次读
                        disktokenisnull +=1;
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
            HeadSet(true);
            cv.notify_one();         // 唤醒等待线程
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
        int abort1 = DeleteNewCall(cancel_id, cancelled_reqid);
        int abort2 = DeleteOverTimeCall(cancel_id, cancelled_reqid);
        //恢复磁盘空间
        n_abort = abort1 + abort2;

        
        printf("%d\n", n_abort);
        for (int id : cancelled_reqid) {
            printf("%d\n", id);
        }
        fflush(stdout);
        //清除被删除对象元数据
        UpdateObjectList();
    }
    //过期的请求
    int DeleteNewCall(std::vector<int>& cancel_id, std::vector<int>& cancelled_reqid)
    {
        int n_abort = 0;
        for (int objid : cancel_id) {
            if (numObjects.count(objid)) {
                for (auto& [reqid, request] : numRequests) {
                    if (request->obj_id == objid && (request->status == RequestStatus::InProgress 
                            || request->status == RequestStatus::Waitting)) {
                        request->status = RequestStatus::Cancelled;
                        cancelled_reqid.push_back(reqid);
                        n_abort++;
                    }
                }
                numObjects[objid]->is_delete = true;
            }
        }
        return n_abort;
    }
    int DeleteOverTimeCall(std::vector<int>& cancel_id, std::vector<int>& cancelled_reqid)
    {
        int n_abort = 0;
        for (int objid : cancel_id) {
            if (numObjects.count(objid)) {
                for (auto& [reqid, request] : overtimeRequests) {
                    if (request->obj_id == objid && (request->status == RequestStatus::InProgress
                            || request->status == RequestStatus::Waitting)) {
                        request->status = RequestStatus::Cancelled;
                        cancelled_reqid.push_back(reqid);
                        n_abort++;
                    }
                }
                numObjects[objid]->is_delete = true;
            }
        }
        return n_abort;
    }
    //恢复磁盘空间
    int RestoreDiskCall(std::vector<int>& cancel_id)
    {
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
                        numDisks[diskindex]->bitmap[ceil] = false;
                    }
                }
            }
        }
        return 0;
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
    //
    bool IsHeadMove()
    {
        return isheadmove;
    }
    void HeadSet(bool flag)
    {
        this->isheadmove = flag;
    }
    //多线程来更新优先级请求队列
    void startthread()
    {
        if (threadrunning.load()) {
            return;  // 避免重复启动线程
        }
        threadrunning.store(true);
        this->prirequest = std::thread(&StorgeManger::ThreadPriorityReuests, this);
    }
    void stopthread()
    {
       threadrunning.store(false);
        cv.notify_all();  // 唤醒可能在 wait 中的线程
        if(prirequest.joinable()){
            prirequest.join();
        }
    }
    void ThreadPriorityReuests()
    {
        std::priority_queue<ReadRequestNode*, std::vector<ReadRequestNode*>, RequestCompare> tempqueue;
        while(threadrunning.load())
        {
            if (!threadrunning.load()) {
                break;  // 线程安全退出
            }
            if(numRequests.size() > 0)
            {
                std::unique_lock<std::mutex> lock(ReqestMutex);
                cv.wait(lock, [this]{return IsHeadMove() || !threadrunning.load();});
                for(auto& [reqid , requires]:numRequests)
                {
                    int already_read_units = requires->already_read_units;
                    int minrepcost = mincost(requires->obj_id, already_read_units);
                    requires->estimated_token_cost = minrepcost;
                    tempqueue.push(requires);
                }
            }
            HeadSet(false);
            MinCostQueue.swap(tempqueue);
        }
    }
        
};



#endif //STORGESERVICE_H