#ifndef STORGESERVICE_H
#define STORGESERVICE_H
#include <iostream>
#include <vector>
#include <map>
#include <memory>
#include <algorithm>
#include <math.h>
#include <deque>
#include "data_class.h"

class StorgeManger{
public:

    StorgeManger(int t, int m, int n,int v,int g):N(n),T(t),M(m),V(v),G(g) 
    {

        for(int i = 0; i < N; i ++)
        {
            Disk* disk = new Disk(i ,v);
            numDisks.push_back(disk);
            
        }
        current_T = 0;
    }
    //刷新每个时间片的令牌
    void RefreshTokenCost()
    {
        for(int i = 0; i < numDisks.size(); i++)
        {
            numDisks[i]->token_cost = G;
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
        obj->is_delete = false; //
        if(obj->save_disk.size() < this->N)
        {
            obj->save_disk.reserve(this->N);
        }
        int diskindex = max_disk();
        //打印存放的对象id
        printf("%d\n", id);
   
        //时间复杂度O(n²)
        for(int i  = 0; i < REP_NUM; i++)
        {
            WriteObject(obj, diskindex, i);
            diskindex = (diskindex+ 1) % N;
        }
        numObjects.push_back(obj);
        fflush(stdout);
        return true;
    }
    bool WriteObject(Object* obj, int diskindex, int copy_index) noexcept
    {
        //找出磁盘容量最大的磁盘
        // int index = max_disk();
        
        if(numDisks[diskindex]->free_space < obj->size)
        {
            return false;
        }
        //TODO
        obj->save_disk[copy_index] = diskindex;
        // int chunk_indx = 0;
        //第几个副本
        printf("%d ", diskindex + 1);
        int obj_size = obj->size;
        //时间复杂度O(n)
        for(int i = 1, j = 1; i <= V; i++)
        {
            if(j > obj_size)
            {
                break;
            }
            if( numDisks[diskindex]->zone[i] == '0')
            {
                obj->chunks[copy_index][j] = i;
                numDisks[diskindex]->zone[i] = '1';
                j++;
                printf(" %d", i);
            }
        }
        printf("\n");
        fflush(stdout);
        numDisks[diskindex]->free_space -= obj_size;
        return true;
    }
    //找出剩余磁盘空间最大的磁盘
    int max_disk() noexcept
    {
        int max_v = 0;
        
        for(int i = 1; i < N; i++)
        {
            max_v = numDisks[i]->free_space > numDisks[max_v]->free_space ? i:max_v;
        }

        return max_v;
    }
    //更新请求队列
    int UpdateRequestList()
    {
        int front = numRequests.size();
        //时间复杂度O(n)
        auto it = std::remove_if(numRequests.begin(), numRequests.end(), [](ReadRequestNode* node){
            if(node->status == RequestStatus::Cancelled || node->status == RequestStatus::Completed)
            {
                delete node;
                return true;
            }
            return false;
        });
        numRequests.erase(it, numRequests.end());
        int back = numRequests.size();
        return back - front;
    }
    //更新对象队列
    int UpdateObjectList()
    {
        int front = numObjects.size();
        //时间复杂度O(n)
        auto it = std::remove_if(numObjects.begin(), numObjects.end(), [](Object* obj){
            if(obj->is_delete == true)
            {
                delete obj;
                return true;
            }
            return false;
        });
        numObjects.erase(it, numObjects.end());
        int back = numObjects.size();
        return back - front;

    }
    //读取事件
    void ReadACtion() noexcept
    {
        int n_read;
        scanf("%d", &n_read);
        for(int i = 0; i < n_read; i++)
        {
            int reqid, objid;
            scanf("%d%d", &reqid, &objid);
            ReadRequestNode* rrn = new ReadRequestNode(reqid, objid);
            rrn->begin_time = current_T;
            numRequests.push_back(rrn);
        }
        //删除过期请求
        UpdateRequestList();
        //处理请求
        int completed_count = 0;
        std::vector<int> request_id_vec;
        //时间复杂度O(n²)  待优化
        for(int i = 0; i < numRequests.size(); i++)
        {
            if(numRequests[i]->status != RequestStatus::InProgress)
            {
                continue;;
            }
            for(auto obj : numObjects)
            {
                if(obj->id == numRequests[i]->obj_id)
                {
                    bool flag = ReadObject(obj, i);
                    if(flag)
                    {
                        completed_count +=1;
                        request_id_vec.push_back(numRequests[i]->req_id);
                    }
                    break;
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
            for(int num : request_id_vec)
            {
                printf("%d\n", num);
            }
        }
        fflush(stdout);

    }
    //读取块的消耗
    int ReadUnit(int diskindex, int index) noexcept
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
                int cost = ReadUnit(save_disk, j);
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
        std::vector<std::vector<int>> cancelled_reqid;
        int n_abort = 0;
        //时间复杂度O(n²)
        for(int i  = 0; i < n_delet; i++)
        {
            std::vector<int> temp = DeleteObject(cancel_id[i]);
            if(temp.size() > 0)
            {
                cancelled_reqid.push_back(temp);
                n_abort += temp.size();
            }
             //删除对象
            for(auto obj:numObjects)
            {
                if(obj->id == cancel_id[i])
                {
                    obj->is_delete = true;
                }
            }
        }
        printf("%d\n", n_abort);
        for(int i = 0; i < cancelled_reqid.size(); i++)
        {
            for(auto id : cancelled_reqid[i])
            {
                printf("%d\n", id);
            }
        }

       fflush(stdout);
    }

    std::vector<int> DeleteObject(int objid)
    {
        // int objid = obj->id;
        std::vector<int> cance_id;
        //时间复杂度O(n)
        for(auto node : numRequests)
        {
            if(node->obj_id == objid)
            {
                if(node->status == RequestStatus::InProgress)
                {
                    node->status = RequestStatus::Cancelled;
                    cance_id.push_back(node->req_id);
                }
            }
        }
        return cance_id;
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
    std::vector<Object*> numObjects;    //存储对象容器
    std::vector<Disk*> numDisks;        //磁盘容器
    std::deque<ReadRequestNode*> numRequests;  //请求队列
    const int T;    //时间
    const int M;     //对象标签数
    const int N;    //磁盘个数
    const int V;    //磁盘容量
    const int G;    //每个时间的令牌数
    int current_T;  //当前的时间片编号
};


#endif //STORGESERVICE_H