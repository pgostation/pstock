//
//  strLbacktestInit.c
//  pstock
//
//  Created by takayoshi on 2016/02/01.
//  Copyright © 2016年 pgostation. All rights reserved.
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "strLbacktestInit.h"
#include "strLbacktestRun.h"
#include "strMsignal.h"
#include "xmapString.h"
#include "calc.h"
#include "main.h"

static strLdata* strLbacktest_init(const char* path);
static void strLbacktest_loadStrMConfig(const char* path, strLdata* dataL);
static void strLbacktest_loadSignalData(const char* path, int dateMax, strMdata* data, strLdata* dataL);

void strLbacktest_start(const char* path, strMdata* data, int startDateIndex, int endDateIndex)
{
    //合成ストラテジーの設定を読み込む
    strLdata* dataL = strLbacktest_init(path);
    if (dataL==NULL) {
        return;
    }
    
    {
        {
            //各ストラテジーの設定を読み込む
            strLbacktest_loadStrMConfig(path, dataL);
        }
        
        {
            //シグナルデータを読み込む
            strLbacktest_loadSignalData(path, data->dailys[MAX_DATA_COUNT-1].count, data, dataL);
        }
    }

    printCalc(" strategyL バックテスト中", 1);
    
    //合成ストラテジーのテスト開始
    strLbacktest_run(path, data, dataL, startDateIndex, endDateIndex);
    
    apr_pool_clear(dataL->pool);
}

static strLdata* strLbacktest_init(const char* path)
{
    strLdata* dataL = malloc(sizeof(strLdata));
    memset(dataL, 0x00, sizeof(strLdata));
    
    apr_pool_create(&dataL->pool, NULL);
    
    xmap_t* xmap;
    
    //xmap取得
    {
        char filePath[256];
        snprintf(filePath, sizeof(filePath)-1, "%s/conf.xmap", path);
        xmap = xmap_load(filePath);
        
        if(xmap==NULL)
        {
            printf("Error: strLbacktest_init(): conf.xmap is NULL\n");
            return NULL;
        }
    }
    
    //スペースで区切って単語配列取得
    for(int i=0; i<10; i++)
    {
        char key[16];
        sprintf(key, "rule%d", i+1);
        const char *bigRule = xmap_get(xmap, key);
        if(bigRule==NULL) {
            continue;
        }
        printf("rule%d:%s\n", i+1, bigRule);
        
        int quoteFlag = 0;
        int start = 0;
        const int bigRuleLen = (int)strlen(bigRule);
        for(int c=0; c<=bigRuleLen; c++)
        {
            if(!quoteFlag && ( bigRule[c]==' ' || bigRule[c]=='\n' || c==bigRuleLen) )
            {
                if(c-start==0)
                {
                    start++;
                    continue;
                }
                char *buf = apr_palloc(dataL->pool, c-start+1);
                strncpy(buf, &bigRule[start], c-start);
                buf[c-start] = '\0';
                dataL->bigRule[i][dataL->bigRuleWordCount[i]] = buf;
                dataL->bigRuleWordCount[i]++;
                start = c+1;
                continue;
            }
            if(bigRule[c]=='"')
            {
                quoteFlag = !quoteFlag;
            }
        }
    }
    
    //スペースで区切って単語配列取得
    for(int i=0; i<10; i++)
    {
        for(int j=0; j<10; j++)
        {
            char key[16];
            sprintf(key, "rule%d_%d", i+1, j+1);
            const char *smallRule = xmap_get(xmap, key);
            if(smallRule==NULL) {
                continue;
            }
            printf("rule%d_%d:%s\n", i+1, j+1, smallRule);
            
            int quoteFlag = 0;
            int start = 0;
            const int smallRuleLen = (int)strlen(smallRule);
            for(int c=0; c<=smallRuleLen; c++)
            {
                if(!quoteFlag && ( smallRule[c]==' ' || smallRule[c]=='\n' || c==smallRuleLen) )
                {
                    if(c-start==0)
                    {
                        start++;
                        continue;
                    }
                    char *buf = apr_palloc(dataL->pool, c-start+1);
                    strncpy(buf, &smallRule[start], c-start);
                    buf[c-start] = '\0';
                    dataL->smallRule[i][j][dataL->smallRuleWordCount[i][j]] = buf;
                    dataL->smallRuleWordCount[i][j]++;
                    start = c+1;
                    continue;
                }
                if(smallRule[c]=='"')
                {
                    quoteFlag = !quoteFlag;
                }
            }
        }
    }
    
    for(int i=0; i<10; i++)
    {
        for(int j=0; j<10; j++)
        {
            for(int k=0; k<10; k++)
            {
                {
                    char key[20];
                    sprintf(key, "strategy%d_%d_%d", i+1, j+1, k+1);
                    const char *strategyStr = xmap_get(xmap, key);
                    if(strategyStr==NULL) {
                        continue;
                    }
                    printf("strategy%d_%d_%d:%s  ", i+1, j+1, k+1, strategyStr);
                    
                    char *buf = apr_palloc(dataL->pool, strlen(strategyStr)+1);
                    strcpy(buf, strategyStr);
                    dataL->strategy[i][j][k] = buf;
                }
                
                {
                    char key[20];
                    sprintf(key, "position%d_%d_%d", i+1, j+1, k+1);
                    const char *positionStr = xmap_get(xmap, key);
                    if(positionStr==NULL) {
                        continue;
                    }
                    printf("%s\n", positionStr);
                    
                    int intValue = 0;
                    sscanf(positionStr, "%d", &intValue);
                    dataL->position[i][j][k] = intValue;
                }
            }
        }
    }
    
    {
        dataL->split = 10;
        const char *splitStr = xmap_get(xmap, "split");
        if(splitStr!=NULL)
        {
            sscanf(splitStr, "%d", &dataL->split);
            if(dataL->split > SPLIT_MAX){
                dataL->split = SPLIT_MAX;
            }
        }
    }
    
    xmap_release(xmap);
    
    return dataL;
}

static void strLbacktest_loadStrMConfig(const char* path, strLdata* dataL)
{
    for(int i=0; i<10; i++)
    {
        for(int j=0; j<10; j++)
        {
            for(int k=0; k<10; k++)
            {
                xmap_t* xmap;
                char* strategyStr = dataL->strategy[i][j][k];
                
                if(strategyStr==NULL || strategyStr[0]=='\0') continue;
                
                {
                    char filePath[256];
                    snprintf(filePath, sizeof(filePath)-1, "%s/../../strategyM/%s/conf.xmap", path, strategyStr);
                    xmap = xmap_load(filePath);
                    
                    if(xmap==NULL)
                    {
                        printf("Error: strLbacktest_loadStrMConfig(): conf.xmap is NULL\n");
                        continue;
                    }
                }
                            
                //スペースで区切って単語配列取得
                {
                    const char *sellRule = xmap_get(xmap, "sell");
                    //printf("sellRule:%s\n", sellRule);
                    
                    int quoteFlag = 0;
                    int start = 0;
                    const int sellRuleLen = (int)strlen(sellRule);
                    for(int c=0; c<=sellRuleLen; c++)
                    {
                        if(!quoteFlag && ( sellRule[c]==' ' || sellRule[c]=='\n' || c==sellRuleLen) )
                        {
                            if(c-start==0)
                            {
                                start++;
                                continue;
                            }
                            char *buf = apr_palloc(dataL->pool, c-start+1);
                            strncpy(buf, &sellRule[start], c-start);
                            buf[c-start] = '\0';
                            dataL->sellWords[i][j][k][dataL->sellWordCount[i][j][k]] = buf;
                            dataL->sellWordCount[i][j][k]++;
                            start = c+1;
                            continue;
                        }
                        if(sellRule[c]=='"')
                        {
                            quoteFlag = !quoteFlag;
                        }
                    }
                }
                
                {
                    const char *lcRule = xmap_get(xmap, "lc");
                    //printf("lcRule:%s\n", lcRule);
                    
                    int quoteFlag = 0;
                    int start = 0;
                    const int lcRuleLen = (int)strlen(lcRule);
                    for(int c=0; c<=lcRuleLen; c++)
                    {
                        if(!quoteFlag && ( lcRule[c]==' ' || lcRule[c]=='\n' || c==lcRuleLen) )
                        {
                            if(c-start==0)
                            {
                                start++;
                                continue;
                            }
                            char *buf = apr_palloc(dataL->pool, c-start+1);
                            strncpy(buf, &lcRule[start], c-start);
                            buf[c-start] = '\0';
                            dataL->lcWords[i][j][k][dataL->lcWordCount[i][j][k]] = buf;
                            dataL->lcWordCount[i][j][k]++;
                            start = c+1;
                            continue;
                        }
                        if(lcRule[c]=='"')
                        {
                            quoteFlag = !quoteFlag;
                        }
                    }
                }
                
                {
                    const char *orderRule = xmap_get(xmap, "order");
                    //printf("order:%s\n", orderRule);
                    
                    int quoteFlag = 0;
                    int start = 0;
                    const int orderRuleLen = (int)strlen(orderRule);
                    for(int c=0; c<=orderRuleLen; c++)
                    {
                        if(!quoteFlag && ( orderRule[c]==' ' || orderRule[c]=='\n' || c==orderRuleLen) )
                        {
                            if(c-start==0)
                            {
                                start++;
                                continue;
                            }
                            char *buf = apr_palloc(dataL->pool, c-start+1);
                            strncpy(buf, &orderRule[start], c-start);
                            buf[c-start] = '\0';
                            dataL->orderWords[i][j][k][dataL->orderWordCount[i][j][k]] = buf;
                            dataL->orderWordCount[i][j][k]++;
                            start = c+1;
                            continue;
                        }
                        if(orderRule[c]=='"')
                        {
                            quoteFlag = !quoteFlag;
                        }
                    }
                }
                
                {
                    const char *xRule = xmap_get(xmap, "rule");
                    //printf("rule:%s\n", xRule);
                    
                    if(strcmp(xRule,"空売り")==0)
                    {
                        dataL->isKarauri[i][j][k] = 1;
                    }
                }

                xmap_release(xmap);
            }
        }
    }
}

static void strLbacktest_loadSignalData(const char* path, int dateMax, strMdata* data, strLdata* dataL)
{
    for(int i=0; i<10; i++)
    {
        for(int j=0; j<10; j++)
        {
            for(int k=0; k<10; k++)
            {
                xmap_t* xmap;
                char* strategyStr = dataL->strategy[i][j][k];
                
                if(strategyStr==NULL || strategyStr[0]=='\0') continue;
                
                {
                    char filePath[256];
                    snprintf(filePath, sizeof(filePath)-1, "%s/../../strategyM/%s/extraSignals.xmap", path, strategyStr);
                    xmap = xmap_load(filePath);
                    
                    if(xmap==NULL)
                    {
                        printf("Error: strLbacktest_loadStrMConfig(): extraSignals.xmap is NULL\n");
                        continue;
                    }
                }
                
                dataL->codes[i][j][k] = apr_palloc(dataL->pool, sizeof(int*)*dateMax);
                dataL->codesCount[i][j][k] = apr_palloc(dataL->pool, sizeof(int)*dateMax);
                for(int dateIndex=0; dateIndex<dateMax; dateIndex++)
                {
                    char key[16];
                    sprintf(key, "codes%d", data->date[dateIndex]);
                    char* codesStr = xmap_get(xmap, key);
                    if(codesStr==NULL){
                        continue;
                    }
                    
                    int signalCountMax = (int)(strlen(codesStr)+1)/5;
                    dataL->codes[i][j][k][dateIndex] = apr_palloc(dataL->pool, sizeof(int)*signalCountMax);
                    dataL->codesCount[i][j][k][dateIndex] = signalCountMax;
                    for(int signalCount=0; signalCount<signalCountMax; signalCount++)
                    {
                        int intValue = 0;
                        sscanf(&codesStr[signalCount*5], "%d ", &intValue);
                        dataL->codes[i][j][k][dateIndex][signalCount] = intValue;
                    }
                }
                
                dataL->filter2History[i][j][k] = apr_palloc(dataL->pool, sizeof(int)*dateMax);
                for(int dateIndex=0; dateIndex<dateMax; dateIndex++)
                {
                    char key[16];
                    sprintf(key, "filter2%d", data->date[dateIndex]);
                    char* filter2Str = xmap_get(xmap, key);
                    if(filter2Str==NULL){
                        dataL->filter2History[i][j][k][dateIndex] = 1;
                        continue;
                    }
                    
                    if(strcmp(filter2Str, "true")==0) {
                        dataL->filter2History[i][j][k][dateIndex] = 1;
                    }
                    else {
                        dataL->filter2History[i][j][k][dateIndex] = 0;
                    }
                }
            }
        }
    }
}
