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

    for(int t = 1; t <= T + EXTRA_TIME; t++)
    {

        stm.SetTime(t);
        stm.timestamp_action();
        stm.DeleteAction();
        stm.WriteAction();
        stm.ReadACtion();

    }
    return 0;
}