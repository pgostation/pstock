//
//  strMdata.h
//  pstock
//
//  Created by takayoshi on 2016/01/24.
//  Copyright © 2016年 pgostation. All rights reserved.
//

#ifndef strMdata_h
#define strMdata_h

#define MAX_DATA_COUNT 4500 //株価データの銘柄数
#define MAX_THREAD_COUNT 8 //最大並列処理数。クアッドコアのHTで8個
#define SPLIT_MAX 30

typedef struct {
    int count;
    //int* date;
    int* start;
    int* end;
    int* max;
    int* min;
    int* volume;
} strMdaily;

typedef struct {
    int unit;
    char isKarauriOk;
    char shijou; //0:データなし  1:東証1部  2:東証2部  3:その他
} strMcompany;

typedef struct {
    int count;
    //int* date;
    float* asset;
    float* netasset;
    float* pureProfit;
    float* keijouProfit;
    float* sales;
    float* eigyouCf;
    float* eps;
    float* bps;
} strMkessan;

typedef struct {
    short count; //銘柄数
    int* codes;
    int* date;
    strMdaily* dailys;
    strMcompany* companys;
    strMkessan* kessans;
} strMdata;

extern strMdata* strMdata_initData(const char *path);
extern void strMdata_loadData(const char *path, strMdata* data);
extern void strMdata_loadKessan(const char *path, strMdata* data);

#endif /* strMdata_h */
