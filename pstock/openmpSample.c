//
//  openmpSample.c
//  pstock
//
//  Created by takayoshi on 2016/01/21.
//  Copyright © 2016年 pgostation. All rights reserved.
//

#include <stdio.h>
#include <sys/time.h>
#include <libiomp/omp.h>

int xmain(int argc, const char * argv[])
{
    short a[60000];
    struct timeval startTime, endTime;
 
    #ifdef _OPENMP
    printf("omp_get_num_procs:%d\n", omp_get_num_procs());
    #endif
    
    gettimeofday(&startTime, NULL);
    
    #pragma omp parallel
    for(short j=0;j<15000;j++)
    {
        #pragma omp for
        for(short i=-30000;i<30000;i++)
        {
            a[i+30000] += i*5;
        }
    }
    
    gettimeofday(&endTime, NULL);
    if(endTime.tv_usec/1000 - startTime.tv_usec/1000>0){
        printf("end:%ld.%03d\n", endTime.tv_sec - startTime.tv_sec, endTime.tv_usec/1000 - startTime.tv_usec/1000);
    }else{
        printf("end:%ld.%03d\n", endTime.tv_sec - startTime.tv_sec -1, 1000 + endTime.tv_usec/1000 - startTime.tv_usec/1000);
    }
    
    return 0;
}

// openmpなし　9.338秒  0.016
// 1thread 11.571秒  0.021
// 2thread 5.326秒  0.010
// 4thread 5.019秒

// no simd       6.573 6.883
// simd          6.571 6.608
// declare simd  6.117 6.341

//