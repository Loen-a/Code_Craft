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

    StorgeManger(int t, int m, int n,int v,int g):N(n),T(t+105),M(m),V(v),G(g) 
    {

        for(int i = 0; i < N; i ++)
        {
            Disk* disk = new Disk(i ,v);
            numDisks.push_back(disk);
            
        }
        token_cost = g;         //初始化时间令牌
    }
    bool WriteAction() noexcept
    {
        int n_write;
        scanf("%d", &n_write);
        for(int i = 0; i < n_write; i++)
        {
            int id ,size, tag;
            scanf("%d%d%d", &id, &size, &tag);
            Write(id, size, tag);

            // printf("%d\n", id);
            // for(int i = 0; i < REP_NUM; i++)
            // {
            //     printf("%d", )
            // }
        }
        return true;
    }
    bool Write(int id, int size, int tag) noexcept
    {
        Object* obj = new Object(id, size, tag);
        if(obj->save_disk.size() < this->N)
        {
            obj->save_disk.reserve(this->N);
        }
        int diskindex = max_disk();
        printf("%d\n", id);
        // 确保 chunks 的大小与磁盘数量一致
        if (obj->chunks.size() != REP_NUM) {
            obj->chunks.resize(REP_NUM);
        }
        
        for(int i  = 0; i < REP_NUM; i++)
        {
            WriteObject(obj, diskindex, i);
            diskindex = (diskindex+ 1) % N;
        }
        numObjects.push_back(obj);
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
        for(int i = 0, j = 0; i < V; i++)
        {
            if( numDisks[diskindex]->zone[i] == '0')
            {
                obj->chunks[copy_index][j] = i;
                numDisks[diskindex]->zone[i] = '1';
                j++;
                printf(" %d", i + 1);
                if(j >= obj_size)
                {
                    break;
                }
            }
        }
        printf("\n");
        fflush(stdout);
        numDisks[diskindex]->ReduceSpace(obj_size);
        return true;
    }

    int max_disk() noexcept
    {
        int max_v = 0;
        
        for(int i = 1; i < N; i++)
        {
            max_v = numDisks[i]->free_space > numDisks[max_v]->free_space ? i:max_v;
        }

        return max_v;
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
            numRequests.push_back(rrn);
        }
        //删除过期请求
        for(auto it = numRequests.begin(); it != numRequests.end(); it ++)
        {
            if((*it)->status != RequestStatus::Cancelled && (*it)->status != RequestStatus::Completed)
            {
                break;
            }
            numRequests.pop_front();
        }
        //处理请求
        int completed_count = 0;
        std::vector<int> request_id_vec;

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
                }
            }
            
            if(token_cost <= 0)
            {
                break;
            }

        }
        printf("%d\n", completed_count);
        if(completed_count > 0)
        {
            for(int num : request_id_vec)
            {
                printf("%d\n", num);
            }
        }

    }
    //读取块的消耗
    int ReadUnit(int diskindex, int index) noexcept
    {
        int cost = 0;
        if(numDisks[diskindex]->last_action == 'R')
        {
            cost = std::max(16, static_cast<int>(std::ceil(numDisks[diskindex]->last_cost * 0.8)));

        }else{
            cost = 64;
        }

        return cost;
    }
    //找到token消耗最少的副本编号
    int mincost(Object* obj) noexcept
    {
        int disk_index = 0;
        int cost_temp = G;
        int copy_index = 0;
        for(int j = 0; j < REP_NUM; j ++)
        {
            int cost = tokencost(obj, j);
            if(cost_temp < cost)
            {
                cost_temp = cost;
                copy_index = j;
            }
        }

        return copy_index;

    }
    //返回读取副本第一个块的总token消耗
    int tokencost(Object* obj, int copy_index) noexcept
    {
        int save_disk = obj->save_disk[copy_index];
        int diskhead = numDisks[save_disk]->head_position;

        int j = 0;  //第一个块。方便后续修改
        int distance = obj->chunks[copy_index][j] - diskhead;        //确定第一个块的token消耗
        int abs_dist = 0;
        if(distance < 0)
        {
            abs_dist = this->V + distance; //磁头位置大于块的位置，相当于旋转一圈
        }else{
            abs_dist = distance;
        }
        int cost_temp = 0;
        if(abs_dist >= this->token_cost)
        {
            //包含两种情况，1：distance>0, 2:distance<0,两种都消耗G
            cost_temp = G;
        }else if(this->token_cost - abs_dist < 64){
            cost_temp = G;
        }else{
            cost_temp = this->token_cost - abs_dist + 64;
        }

        return cost_temp;
    }

    //读取对象
    //obj 对象指针，rep_index 对象副本编号，request_index请求编号
    bool ReadObject(Object* obj, int request_index) noexcept
    {

        int objid = obj->id;
        int objsize = obj->size;
        //找到消耗最少的副本，进行读取
        int min_repindex = mincost(obj); 
        int save_disk = obj->save_disk[min_repindex];
        //待修改
        if(this->numRequests[request_index]->status == RequestStatus::Cancelled)
        {
        
            return false;
        }
 
        int already_read_units = this->numRequests[request_index]->already_read_units;
        // int begin = this->numRequests[request_index]->already_read_units;
        //顺序读取对象块
        for(int j = already_read_units; j < objsize;)
        {
            int head = numDisks[save_disk]->head_position;
            //待修改
            if(token_cost <= 0)
            {
                return false;
            }
            //找到位置，读取对象块
            if(head == obj->chunks[min_repindex][j])
            {
                int cost = ReadUnit(save_disk, j);
                if(this->token_cost >= cost)
                {
                    numDisks[save_disk]->last_cost = cost;
                    this->token_cost -= cost;
                    already_read_units += 1;
                    numDisks[save_disk]->head_position = (numDisks[save_disk]->head_position + 1) % V;
                    printf("r");
                    j++;
                    continue;
                }else{
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

                if(abs_dist >= this->token_cost)
                {
                    this->token_cost = 0; //消耗
                    numDisks[save_disk]->head_position = obj->chunks[min_repindex][j];//磁头位置
                    printf("j %d", obj->chunks[min_repindex][j]);               //输出结果
                }else if(this->token_cost - abs_dist < 64){
                    this->token_cost = 0;//消耗
                    numDisks[save_disk]->head_position = obj->chunks[min_repindex][j];//磁头位置
                    printf("j %d", obj->chunks[min_repindex][j]);
                }else{
                    std::string str(abs_dist, 'p');
                    this->token_cost -= abs_dist ;//消耗
                    numDisks[save_disk]->head_position = obj->chunks[min_repindex][j];//磁头位置
                    printf("%s", str.c_str());//输出结果
                }
            }

        }
        printf("#\n");
                    //读取完毕
        if(already_read_units >= objsize)
        {
            this->numRequests[request_index]->already_read_units = objsize;
            this->numRequests[request_index]->status = RequestStatus::Completed;
        }
        return true;
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
    int token_cost;  //每个时间片的令牌消耗
};


#endif //STORGESERVICE_H