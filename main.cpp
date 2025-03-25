#include <iostream>
#include <cstdio>
#include <cassert>
#include <cstdlib>
#include "data_class.h"
#include "storgeservice.hpp"


int main()
{
    int Tt,Mm,Nn,Vv,Gg;
    scanf("%d %d %d %d %d", &Tt, &Mm, &Nn,&Vv,&Gg);
    for (int i = 1; i <= Mm; i++) {
        for (int j = 1; j <= (Tt - 1) / FRE_PER_SLICING + 1; j++) {
            scanf("%*d");
            // scanf("%d", &num);
        }

    }
    //每个标签i写入总数
    for (int i = 1; i <= Mm; i++) {
        for (int j = 1; j <= (Tt - 1) / FRE_PER_SLICING + 1; j++) {
            // scanf("%d", &num);
            scanf("%*d");
        }
    }
     //每个标签i读取总数
    for (int i = 1; i <= Mm; i++) {
        for (int j = 1; j <= (Tt - 1) / FRE_PER_SLICING + 1; j++) {
            scanf("%*d");
        }
    }
    printf("OK\n");
    fflush(stdout);
    StorgeManger stm(Tt, Mm, Nn, Vv ,Gg);
    // stm.init();
    for(int t = 1; t <= Tt + EXTRA_TIME; t++)
    {
        stm.SetTime(t);
        stm.timestamp_action();
        stm.DeleteAction();
        stm.WriteAction();
        stm.ReadAction();
    }
    return 0;
}