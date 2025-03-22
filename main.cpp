#include <iostream>
#include <cstdio>
#include <cassert>
#include <cstdlib>
#include "data_class.h"
#include "storgeservice.hpp"

int main()
{
    int T,M,N,V,G;
    scanf("%d %d %d %d %d", &T, &M, &N,&V,&G);
    
    //下面三个for待修改
    for (int i = 1; i <= M; i++) {
        for (int j = 1; j <= (T - 1) / FRE_PER_SLICING + 1; j++) {
            scanf("%*d");
        }
    }

    for (int i = 1; i <= M; i++) {
        for (int j = 1; j <= (T - 1) / FRE_PER_SLICING + 1; j++) {
            scanf("%*d");
        }
    }

    for (int i = 1; i <= M; i++) {
        for (int j = 1; j <= (T - 1) / FRE_PER_SLICING + 1; j++) {
            scanf("%*d");
        }
    }

    printf("OK\n");
    fflush(stdout);

    StorgeManger stm(T, M, N, V ,G);
    // std::thread prireuests;
    for(int t = 1; t <= T + EXTRA_TIME; t++)
    {
        // time_t start;
        // time(&start);
        stm.SetTime(t);
        stm.timestamp_action();
        stm.DeleteAction();
        // time_t endtime1;
        // time(&endtime1);
        // if(endtime1 - start > 1)
        // {
        //     printf("DeleteAction");
        // }
        stm.WriteAction();
        // prireuests = new std::thread(stm.ThreadPriorityReuests());
        // time_t endtime2;
        // time(&endtime2);
        // if(endtime2 - start > 2)
        // {
        //     printf("WriteAction");
        // }
        stm.ReadAction();
        // time_t endtime3;
        // time(&endtime3);
        // if(endtime3 - start > 3)
        // {
        //     printf("ReadAction");
        // }
    }
    return 0;
}