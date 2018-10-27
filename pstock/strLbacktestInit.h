//
//  strLbacktestInit.h
//  pstock
//
//  Created by takayoshi on 2016/02/01.
//  Copyright © 2016年 pgostation. All rights reserved.
//

#ifndef strLbacktestInit_h
#define strLbacktestInit_h

#include <apr-1/apr_pools.h>
#include "strMdata.h"

typedef struct {
    //strategyL conf.xmap
    const char* bigRule[10][64];
    int bigRuleWordCount[10];
    
    const char* smallRule[10][10][64];
    int smallRuleWordCount[10][10];
    
    char* strategy[10][10][10];
    int position[10][10][10];
    
    int split;
    
    //strategyM conf.xmap
    const char* sellWords[10][10][10][64];
    int sellWordCount[10][10][10];
    const char* lcWords[10][10][10][64];
    int lcWordCount[10][10][10];
    const char* orderWords[10][10][10][64];
    int orderWordCount[10][10][10];
    int isKarauri[10][10][10];
    
    //strategyM signal
    int** codes[10][10][10];
    int* codesCount[10][10][10];
    int* filter2History[10][10][10];
    
    apr_pool_t* pool;
} strLdata;

extern void strLbacktest_start(const char* path, strMdata* data, int startDateIndex, int endDateIndex);

#endif /* strLbacktestInit_h */
