#include <iostream>
#include "data_class.h"
#include "storgeservice.hpp"
int main()
{
    int T,M,N,V,G;
    scanf("%d %d %d %d %d", &T, &M, &N,&V,&G);
    StorgeManger stm(T, M, N, V ,G);

    stm.WriteAction();
    stm.ReadACtion();
    return 0;
}