#include <iostream>
#include <vector>
#include <map>
#include <memory>
#include <algorithm>
#include "data_class.h"

class StorgeManger{
public:

    StorgeManger(int n, int v, int t,int m,int g):N(n),T(t),M(m),V(v),G(g)
    {
        for(int i = 1; i <= N; i ++)
        {
            numDisks.push_back(std::make_unique<Disk>(i, v));
        }
    }
    bool WriteObject(std::unique_ptr<Object>& obj, int index, int copy_index)
    {
        // int index = max_disk();
        //找出磁盘容量最大的磁盘
        if(numDisks[index]->free_space < obj->size)
        {
            return false;
        }
        //TODO
        obj->save_disk = index;
        // int chunk_indx = 0;
        //找到空位置
        for(int i = 1, j = 1; i < V+1; i++)
        {
            if( numDisks[index]->zone[i] == '0')
            {
                obj->chunks[copy_index][j] = i;
                j++;
                
                if(j == obj->size)
                {
                    break;
                }
            }

        }
        
        numObjects.push_back(obj);
        return true;
    }

    int max_disk()
    {
        int max_v = 1;
        for(int i = 1; i <= N; i++)
        {
            max_v = numDisks[i]->storage > numDisks[max_v]->storage ? i:max_v;
        }
        return max_v;
    }

private:    
    std::vector<std::unique_ptr<Object>> numObjects;    //存储对象
    std::vector<std::unique_ptr<Disk>> numDisks;
    const int T;
    const int M;
    const int N;
    const int V;
    const int G;
};