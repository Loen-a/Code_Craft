#ifndef STORGESERVICE_H
#define STORGESERVICE_H
#include <iostream>
#include "data_class.h"
//调试
//调试
#include <fstream>
#include <csignal>
//lambda函数，谓词
struct DiskCompare {
    bool operator()(const std::pair<int, int>& a, const std::pair<int, int>& b) const {
        return (a.second > b.second) || (a.second == b.second && a.first < b.first);
    }
};
class DiskManger{
private:
    std::vector<Disk*> numDisks;        //磁盘容器
    int Ndisks;
    int Volum;
public:
    DiskManger(int N, int V):Ndisks(N), Volum(V)
    {
        
    }
};
class StorgeManger{
private:    
    std::unordered_map<int, Object*> numObjects;    //存储对象容器,按照存放磁盘进行分组
    std::vector<Disk*> numDisks;        //磁盘容器
    std::unordered_map<int, ReadRequestNode*> numRequests;  //请求队列 ，按照读取磁盘进行分组
    std::unordered_map<int, ReadRequestNode*> overtimeRequests; //过期的队列
    std::set<std::pair<int, int>, DiskCompare> diskQueue;
    //每种标签的TagInfo
    std::unordered_map<int, TagInfo*> numTagInfo;
    // std::priority_queue<ReadRequestNode*, std::vector<ReadRequestNode*>, RequestCompare> MinCostQueue;
    std::vector<int> ReadRequestId;
    // std::unique_ptr<ReadRequestManager> requestManager; // 在堆上分配
     std::unordered_map<int, std::vector<int>> obj_read_queues; // obj_id -> 请求 req_id 列表
    // std::vector<int> obj_id_list;  // 存储所有 obj_id，后续排序使用
    int disktokenisnull = 0;            //记录是否所有磁盘的token都消耗完毕
    int overtime = 20;
    const int T;    //时间
    const int M;     //对象标签数
    const int N;    //磁盘个数
    const int V;    //磁盘容量
    const int G;    //每个时间的令牌数
    int current_T;  //当前的时间片编号

    //调试
    std::unique_ptr<std::ofstream> logFile2;
public:

    StorgeManger(int t, int m, int n,int v,int g)
        :T(t),M(m),N(n),V(v),G(g),current_T(0)
    {
        
        numDisks.resize(N);
        numObjects.reserve(N);
        numRequests.reserve(N);
        for (int i = 0; i < N; i++) {
            // numDisks.emplace_back(new Disk(i, v));
            numDisks[i] = new Disk(i, v);
            diskQueue.emplace(i, v); // 初始化堆
        }

        logFile2 = std::make_unique<std::ofstream>("errorlog.txt", std::ios::app);
    }
    ~StorgeManger() {
        for (auto& disk : numDisks) delete disk;   

        // for(auto& vec:numObjects)
        // {
           for (auto& [id, obj] : numObjects) delete obj;
        // }
        // for(auto& vec:numRequests)
        // {
           for (auto& [id, req] : numRequests) delete req;
        // }
        // for(auto& vec:overtimeRequests)
        // {
            for (auto& [id, req] : overtimeRequests) delete req;
        // }
        
    }
    void init()
    {
        //下面三个for待修改
        //每个标签i删除总数
        for (int i = 1; i <= M; i++) {
            int total = 0;
            int num = 0;
            TagInfo* taginfo = new TagInfo(M,i);
            for (int j = 1; j <= (T - 1) / FRE_PER_SLICING + 1; j++) {
                // scanf("%*d");
                scanf("%d", &num);
                total += num;

                taginfo->total_delete[j] = num;
            }
            taginfo->total_delete[0] = total;
            numTagInfo[i] = taginfo;
        }

        //每个标签i写入总数
        for (int i = 1; i <= M; i++) {
            int total = 0;
            int num = 0;
            for (int j = 1; j <= (T - 1) / FRE_PER_SLICING + 1; j++) {
                scanf("%d", &num);
                // scanf("%*d");
                total += num;
                numTagInfo[i]->total_write[j] = num; 
            }
            numTagInfo[i]->total_write[0] = total;
        }

        //每个标签i读取总数
        for (int i = 1; i <= M; i++) {
            int total = 0;
            int num = 0;
            for (int j = 1; j <= (T - 1) / FRE_PER_SLICING + 1; j++) {
                scanf("%d", &num);
                // scanf("%*d");
                total += num;
                numTagInfo[i]->total_read[j] = num; 
            }
            numTagInfo[i]->total_read[0] = total;
        }
        printf("OK\n");
        fflush(stdout);
    }
    //每个时间片开始刷新令牌
    void RefreshTokenCost() noexcept{
        for (auto &disk : numDisks) {
            disk->token_cost = G;
        }
        disktokenisnull = 0;
    }
    //写入事件
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
    bool Write(int objid, int objsize, int objtag) noexcept
    {
        Object* obj = new Object(objid, objsize, objtag);
        numObjects[objid] = obj;
        //打印存放的对象id
        printf("%d\n", objid);

        std::set<int> selectdisk;
        int hashindex = 0;
        for(int i = 0; i < REP_NUM;)
        {
            int rep_index = choose_disk(objid, hashindex++, objtag, i);
            if(!selectdisk.count(rep_index))
            {
                selectdisk.insert(rep_index);
                i++;
            }
            if(rep_index < 0)
            {
                //待优化 异常退出
                break;
            }
        }
        //时间复杂度O(n²)
        // int rep_index = 0;
        // for(auto& index : selectdisk)
        // {
        //     if(rep_index < REP_NUM)
        //     {
        //         WriteObject(objid, index, rep_index++);
        //     }

        // }
        for(int i = 0; i < REP_NUM; i++)
        {
                WriteObject(objid, obj->save_disk[i], i);
        }
        // numObjects.push_back(obj);

        fflush(stdout);
        return true;
    }
    bool WriteObject(int objid, int diskindex, int copy_index) noexcept
    {
        auto& obj = numObjects[objid];
        //TODO
        // obj->save_disk[copy_index] = diskindex;

        //第几个副本
        printf("%d", diskindex + 1);
        int obj_size = obj->size;

        // int start_pos = findContinuousBlocks(diskindex, obj_size);s
        int start_pos = obj->start_pos[copy_index];
        if (start_pos > 0) {
        // 找到连续块，进行连续分配
            for (int j = 1; j <= obj_size; j++) {
                obj->chunks[copy_index][j] = start_pos + j - 1;
                numDisks[diskindex]->bitmap[start_pos + j - 1] = true;
                printf(" %d", start_pos + j - 1);
            }
         }else {
            // 时间复杂度O(n)
            int block = 1;
            int maxdisk = max_disk();
            obj->save_disk[copy_index] = diskindex;
            for(int j = 1; block <= V; block++)
            {
                if(j > obj_size)
                {
                    break;
                }
                if( numDisks[diskindex]->bitmap[block] == false)
                {
                    obj->chunks[copy_index][j] = block;
                    numDisks[diskindex]->bitmap[block] = true;
                    j++;
                    printf(" %d", block);
                }
            }
        }

        printf("\n");
        fflush(stdout);
        //更新磁盘空间
        UpdateDiskSpace(diskindex, obj_size);
        return true;
    }
    //更新磁盘空间
    void UpdateDiskSpace(int diskindex, int obj_size)
    {
        auto old_key = std::make_pair(diskindex, numDisks[diskindex]->free_space);
        diskQueue.erase(old_key);
        numDisks[diskindex]->free_space -= obj_size;
        diskQueue.emplace(diskindex, numDisks[diskindex]->free_space);
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
    //返回候选磁盘索引
    int choose_disk(int objid, size_t size, int tag, int rep_index)
    {
        auto& obj = numObjects[objid];
        // 计算对象ID,size,tag 三个属性的哈希
        int num_disks = N;
        int candidate = calculateHash(0, size, tag) % num_disks;
        for(int i = 0; i < num_disks; i++)
        {
            int idx = (candidate + i) % num_disks;
            int start_pos = findContinuousBlocks(idx, obj->size);
            if( start_pos > 0)
            {
                obj->save_disk[rep_index] = idx;
                obj->start_pos[rep_index] = start_pos;
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
    int mincost(int obj_id, int chunk_index, int req_id) noexcept
    {
        int disk_index = 0;
        int cost_temp = G;
        int copy_index = 0;
        //时间复杂度O(n)
        for(int j = 0; j < REP_NUM; j++)
        {
            int cost = tokencost(numObjects[obj_id], j, chunk_index);
 
            if(cost_temp > cost)
            {
                cost_temp = cost;
                copy_index = j;
            }
        }
        numRequests[req_id]->estimated_token_cost = cost_temp;
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
    //针对读请求id排序
    int sortReqidByTokenCost(std::vector<int>& sortlist)
    {

        if(sortlist.size() <= 1)
        {
            return -1;
        }
        // 使用自定义比较函数进行排序
        std::sort(sortlist.begin(), sortlist.end(), 
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
        );
        return 0;
    }
    //针对对象id排序
    int sortobjByTokenCost(std::vector<int>& sortlist)
    {
        if(sortlist.size() <= 1)
        {
            return -1;
        }
        // 使用自定义比较函数进行排序
        std::sort(sortlist.begin(), sortlist.end(), 
            [&](int objid1, int objid2) {
                // 计算每个请求需要消耗的token
                int tokenCost1 = 0;
                int tokenCost2 = 0;
                int reqId1 = 0;
                int reqId2 = 0;
                if(obj_read_queues[objid1].empty() == false)
                {
                    reqId1 = obj_read_queues[objid1][0];
                    tokenCost1 = numRequests[reqId1]->estimated_token_cost;
                }

                if(obj_read_queues[objid2].empty() == false)
                {
                    reqId2 = obj_read_queues[objid2][0];
                    tokenCost2 = numRequests[reqId2]->estimated_token_cost;
                }

                // 按token消耗从大到小排序
                return tokenCost1 > tokenCost2;
            }
        );
        return 0;
    }
     // 读取成功后，所有未取消的请求都完成
    std::vector<int> markReadSuccess(int obj_id) {
        std::vector<int> completed_requests;
        auto it = obj_read_queues.find(obj_id);
        if (it != obj_read_queues.end()) {
            completed_requests = std::move(it->second); // 直接移动数据，避免拷贝
            obj_read_queues.erase(it);
        }
        return completed_requests;
    }
    std::vector<int> markReadSuccess_2(int obj_id) {
        std::vector<int> completed_requests;
        //保存已经完成的请求，方便打印
         std::vector<int> temp_requests;
        auto it = obj_read_queues.find(obj_id);
        if (it != obj_read_queues.end()) {
            temp_requests = std::move(it->second); // 直接移动数据，避免拷贝
            auto it_complete = std::remove_if(it->second.begin(), it->second.end(),
                [this, &completed_requests](int reqid){
                    if(numRequests[reqid]->status == RequestStatus::Completed)
                    {
                        completed_requests.push_back(reqid);
                        return true;
                    }else{
                        return false;
                    }
                });
            //删除已经完成的请求
            it->second.erase(it_complete, it->second.end());
            if(it->second.empty())
            {
                obj_read_queues.erase(it);
            }

        }
        return completed_requests;
    }
    // 取消请求
    void cancelRequest(int obj_id, int req_id) {
        auto it = obj_read_queues.find(obj_id);
        if (it != obj_read_queues.end()) {
            auto& req_list = it->second;
            req_list.erase(std::remove(req_list.begin(), req_list.end(), req_id), req_list.end()); // 删除 req_id

            if (req_list.empty()) { // 如果该 obj_id 没有剩余请求
                obj_read_queues.erase(it);
                removeObjIdFromList(obj_id);
            }
        }
    }
    // 移除 obj_id_list 中的 obj_id
    void removeObjIdFromList(int obj_id) {
        ReadRequestId.erase(std::remove(ReadRequestId.begin(), ReadRequestId.end(), obj_id), ReadRequestId.end());
    }

    //读取事件
    void ReadAction() noexcept
    {
        //删除过期请求
        UpdateNewRequestList();
        UpdateOverTimeRequestList();
        // SwapOvertimeRequest();
        int n_read;
        scanf("%d", &n_read);

        for(int i = 0; i < n_read; i++)
        {
            int reqid, objid;
            scanf("%d%d", &reqid, &objid);
            if (numObjects.find(objid) != numObjects.end()) {
                numRequests[reqid] = new ReadRequestNode(reqid, objid,numObjects[objid]->tag, current_T);
                int repindex = mincost(objid, 1, reqid);
                numRequests[reqid]->selectd_replica = repindex;
                // 如果 objid 是新出现的，加入 vector 进行排序
                if (obj_read_queues.find(objid) == obj_read_queues.end()) {
                    ReadRequestId.push_back(objid);
                }
                //按照对象分组请求
                obj_read_queues[objid].push_back(reqid);
            }
        }
        
        //调试
        if(current_T > 12000)
            *logFile2 <<"current_T: "<<current_T<<",ReadRequestId size: "<< ReadRequestId.size()<<std::endl;

        //根据token消耗排序
        sortobjByTokenCost(ReadRequestId);

        //本次时间片处理完成的请求
        int completed_count = 0;
        std::vector<std::vector<int>> completed_requestslist;
       //最小堆处理读请求
        for(int i = ReadRequestId.size() - 1; i >= 0; i--)
        {
            if(disktokenisnull >= N)
            {
                //所有磁盘toen都消耗完毕
                break;
            }
            int objid = ReadRequestId[i];
            int reqid = 0;
            if(obj_read_queues[objid].empty() == false)
            {
                reqid = obj_read_queues[objid][0];

            }else{
                ReadRequestId.erase(std::remove(ReadRequestId.begin(), ReadRequestId.end(), objid), ReadRequestId.end());
                continue;
            }
            if(numRequests[reqid]->status == RequestStatus::Waitting || 
                    numRequests[reqid]->status == RequestStatus::InProgress)
            {
                
                if (ReadObject(objid, reqid)) {
                    std::vector<int> completed_requests = markReadSuccess(objid);
                    completed_requestslist.push_back(completed_requests);
                    // completed_count += completed_requests.size();
                    //只有相同obj对象的全部请求完成才删除这个待读对象
                    if(obj_read_queues.find(objid) == obj_read_queues.end())
                    {
                         ReadRequestId.erase(std::remove(ReadRequestId.begin(), ReadRequestId.end(), objid), ReadRequestId.end());
                    }
                   
                    for(int reqid : completed_requests)
                    {
                        if(numRequests[reqid]->obj_units.count() >= numObjects[objid]->size)
                        {
                            completed_count +=1;
                            numRequests[reqid]->already_read_units = numObjects[objid]->size;
                            numRequests[reqid]->status = RequestStatus::Completed;
                        }

                    }
                }
            }
        }
 
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
        //返回完成结果 O(n²)
        printf("%d\n", completed_count);
        if(completed_count > 0)
        {
            for(auto reqlist : completed_requestslist)
            {
                for(int reqid : reqlist)
                {
                    if(numRequests[reqid]->status == RequestStatus::Completed)
                        printf("%d\n", reqid);
                }

            }
        }
        fflush(stdout);
    }

    //读取对象
    //obj 对象指针，rep_index 对象副本编号，request_index请求编号
    bool ReadObject(int objid, int request_index) noexcept
    {
        auto& obj = numObjects[objid];
        int objsize = obj->size;
        int already_read_units = this->numRequests[request_index]->already_read_units;
        //找到消耗最少的副本，进行读取
        int min_repindex = numRequests[request_index]->selectd_replica; 
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

                    already_read_units += 1;
                    auto& reqvec = obj_read_queues[objid];
                    // for(int i = 1; i < reqvec.size() - 1; i++)
                    // {
                    //     if(this->numRequests[request_index]->already_read_units == 1)
                    //     {
                    //         this->numRequests[reqvec[i]]->status =RequestStatus::InProgress;
                    //     }
                    // }
                    for(int reqid : reqvec)
                    {
                        this->numRequests[reqid]->already_read_units +=1;
                        this->numRequests[reqid]->status =RequestStatus::InProgress;
                        this->numRequests[reqid]->obj_units[j] = true;
                    }
                    // this->numRequests[request_index]->already_read_units +=1;
                    // this->numRequests[request_index]->status =RequestStatus::InProgress;

                    numDisks[save_disk]->head_position = (numDisks[save_disk]->head_position %V ) + 1;
                    numDisks[save_disk]->read_action.append("r");
                    numDisks[save_disk]->last_action = 'r';
                    j++;
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
                        this->numRequests[request_index]->estimated_token_cost = 0;
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
                        this->numRequests[request_index]->estimated_token_cost = 0;
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
                    this->numRequests[request_index]->estimated_token_cost = 0;

                }
            }

        }
        //读取完毕
        if(this->numRequests[request_index]->obj_units.count() >= objsize)
        {
            // this->numRequests[request_index]->already_read_units = objsize;
            // this->numRequests[request_index]->status = RequestStatus::Completed;
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
                        cancelRequest(objid,reqid);
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
                        cancelRequest(objid,reqid);
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
    // void PriorityReuests()
    // {
    //     std::priority_queue<ReadRequestNode*, std::vector<ReadRequestNode*>, RequestCompare> tempqueue;
    //     if(numRequests.size() > 0)
    //     {
    //         for(auto& [reqid , requires]:numRequests)
    //         {
    //             int already_read_units = requires->already_read_units;
    //             int minrepcost = mincost(requires->obj_id, already_read_units);
    //             requires->estimated_token_cost = minrepcost;
    //             tempqueue.push(requires);
    //         }
    //     }
    //     MinCostQueue.swap(tempqueue);
    
    // }
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
        
};



#endif //STORGESERVICE_H