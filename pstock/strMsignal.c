//
//  strMsignal.c
//  pstock
//
//  Created by takayoshi on 2016/01/24.
//  Copyright © 2016年 pgostation. All rights reserved.
//

#include <stdio.h>
#include <libiomp/omp.h>
#include "main.h"
#include "strMsignal.h"
#include "xmapString.h"
#include "calc.h"

static int dekidaka(strMdata* data, int dateIndex, int codeIndex, int codeRule);
static int filter(strMdata* data, int dateIndex, const char** sig2Words, int sig2WordCount);

strMsignalSum* strMsignal_start(const char *path, strMdata* data, int startDateIndex, int endDateIndex)
{
    char* logicWords[256] = {0x00};
    int wordCount = 0;
    const char* filterWords[64] = {0x00};
    int filterWordCount = 0;
    int codeRule = 0;
    
    //スペースで区切って単語配列取得
    {
        char filePath[256];
        snprintf(filePath, sizeof(filePath)-1, "%s/conf.xmap", path);
        xmap_t* xmap = xmap_load(filePath);
        
        if(xmap==NULL)
        {
            printf("Error: strMsignal_start(): conf.xmap is NULL\n");
            return NULL;
        }
        
        const char* buyRule = xmap_get(xmap, "buy");
        
        printf("buyRule:%s\n", buyRule);
        
        int quoteFlag = 0;
        int start = 0;
        const int buyRuleLen = (int)strlen(buyRule);
        for(int i=0; i<=buyRuleLen; i++)
        {
            if(!quoteFlag && ( buyRule[i]==' ' || buyRule[i]=='\n' || i==buyRuleLen) )
            {
                if(i-start==0)
                {
                    start++;
                    continue;
                }
                char *buf = malloc(i-start+1);
                strncpy(buf, &buyRule[start], i-start);
                buf[i-start] = '\0';
                logicWords[wordCount] = buf;
                wordCount++;
                start = i+1;
                continue;
            }
            if(buyRule[i]=='"')
            {
                quoteFlag = !quoteFlag;
            }
        }
        
        char* codeRuleStr = xmap_get(xmap, "code");
        if(strcmp(codeRuleStr,"すべて")==0){
            codeRule = 0;
        }
        if(strcmp(codeRuleStr,"大型株のみ")==0){
            codeRule = 1;
        }
        if(strcmp(codeRuleStr,"出来高が少ないのは避ける")==0){
            codeRule = 2;
        }
        if(strcmp(codeRuleStr,"出来高がとても少ないのは避ける")==0){
            codeRule = 3;
        }
        if(strcmp(codeRuleStr,"小型株のみ")==0){
            codeRule = 4;
        }
    
        {
            const char *filterRule = xmap_get(xmap, "filter");
            printf("filter:%s\n", filterRule);
            
            int quoteFlag = 0;
            int start = 0;
            const int filterRuleLen = (int)strlen(filterRule);
            for(int i=0; i<=filterRuleLen; i++)
            {
                if(!quoteFlag && ( filterRule[i]==' ' || filterRule[i]=='\n' || i==filterRuleLen) )
                {
                    if(i-start==0)
                    {
                        start++;
                        continue;
                    }
                    char *buf = malloc(i-start+1);
                    strncpy(buf, &filterRule[start], i-start);
                    buf[i-start] = '\0';
                    filterWords[filterWordCount] = buf;
                    filterWordCount++;
                    start = i+1;
                    continue;
                }
                if(filterRule[i]=='"')
                {
                    quoteFlag = !quoteFlag;
                }
            }
        }
    }

    const int dateCountMax = data->dailys[MAX_DATA_COUNT-1].count;
    
    strMsignalSum* sumData = malloc(sizeof(strMsignalSum));
    sumData->count = dateCountMax;
    sumData->dailyCount = malloc(sizeof(int) * dateCountMax);
    memset(sumData->dailyCount, 0x00, sizeof(int) * dateCountMax);
    sumData->dailySignal = malloc(sizeof(strMsignal*) * dateCountMax);
    memset(sumData->dailySignal, 0x00, sizeof(strMsignal*) * dateCountMax);

    if (startDateIndex < 5) {
         startDateIndex = 5; //0からだと過去のデータがなさすぎるので、5日目から開始
    }

    //1日ずつずらしながら式の評価
    #pragma omp parallel
    #pragma omp sections
    {
        #pragma omp section
        {
            for(int dateIndex=startDateIndex; dateIndex<startDateIndex+(endDateIndex-startDateIndex)/4; dateIndex++)
            {
                //フィルター
                int filterResult = filter(data, dateIndex, filterWords, filterWordCount);
                if(!filterResult)
                {
                    continue;
                }
                
                int dailyCount = 0;
                for(int codeIndex=0; codeIndex<data->count; codeIndex++)
                {
                    if(codeRule!=0 && dekidaka(data, dateIndex, codeIndex, codeRule)){
                        continue;
                    }
                    if(calc((const char **)logicWords, 0, wordCount-1, data, NULL, NULL, dateIndex, codeIndex) > 0)
                    {
                        if(data->dailys[codeIndex].count==0 || data->dailys[codeIndex].end[dateIndex]==0){
                            continue;
                        }
                        if(dateIndex+1 < dateCountMax && data->dailys[codeIndex].start[dateIndex+1] == 0){
                            continue;
                        }
                        //add signal
                        dailyCount++;
                        strMsignal* new = malloc(sizeof(strMsignal) * dailyCount);
                        if(sumData->dailySignal[dateIndex]!=NULL){
                            memcpy(new, sumData->dailySignal[dateIndex], sizeof(strMsignal) * (dailyCount-1));
                        }
                        free(sumData->dailySignal[dateIndex]);
                        new[dailyCount-1].buyDateIndex = dateIndex;
                        new[dailyCount-1].codeIndex = codeIndex;
                        if(dateIndex+1 >= dateCountMax){
                            new[dailyCount-1].buyValue = -1;
                        } else {
                            new[dailyCount-1].buyValue = data->dailys[codeIndex].start[dateIndex+1];
                        }
                        sumData->dailySignal[dateIndex] = new;
                    }
                }
                sumData->dailyCount[dateIndex] = dailyCount;
            }
        }
        
        #pragma omp section
        {
            for(int dateIndex=startDateIndex+(endDateIndex-startDateIndex)/4; dateIndex<startDateIndex+(endDateIndex-startDateIndex)/2; dateIndex++)
            {
                int dailyCount = 0;
                for(int codeIndex=0; codeIndex<data->count; codeIndex++)
                {
                    if(codeRule!=0 && dekidaka(data, dateIndex, codeIndex, codeRule)){
                        continue;
                    }
                    if(calc((const char **)logicWords, 0, wordCount-1, data, NULL, NULL, dateIndex, codeIndex) > 0)
                    {
                        if(data->dailys[codeIndex].count==0 || data->dailys[codeIndex].end[dateIndex]==0){
                            continue;
                        }
                        if(dateIndex+1 < dateCountMax && data->dailys[codeIndex].start[dateIndex+1] == 0){
                            continue;
                        }
                        //add signal
                        dailyCount++;
                        strMsignal* new = malloc(sizeof(strMsignal) * dailyCount);
                        if(sumData->dailySignal[dateIndex]!=NULL){
                            memcpy(new, sumData->dailySignal[dateIndex], sizeof(strMsignal) * (dailyCount-1));
                        }
                        free(sumData->dailySignal[dateIndex]);
                        new[dailyCount-1].buyDateIndex = dateIndex;
                        new[dailyCount-1].codeIndex = codeIndex;
                        if(dateIndex+1 >= dateCountMax){
                            new[dailyCount-1].buyValue = -1;
                        } else {
                            new[dailyCount-1].buyValue = data->dailys[codeIndex].start[dateIndex+1];
                        }
                        sumData->dailySignal[dateIndex] = new;
                    }
                }
                sumData->dailyCount[dateIndex] = dailyCount;
            }
        }
        
        #pragma omp section
        {
            for(int dateIndex=startDateIndex+(endDateIndex-startDateIndex)/2; dateIndex<startDateIndex+(endDateIndex-startDateIndex)*3/4; dateIndex++)
            {
                int dailyCount = 0;
                for(int codeIndex=0; codeIndex<data->count; codeIndex++)
                {
                    if(codeRule!=0 && dekidaka(data, dateIndex, codeIndex, codeRule)){
                        continue;
                    }
                    if(calc((const char **)logicWords, 0, wordCount-1, data, NULL, NULL, dateIndex, codeIndex) > 0)
                    {
                        if(data->dailys[codeIndex].count==0 || data->dailys[codeIndex].end[dateIndex]==0){
                            continue;
                        }
                        if(dateIndex+1 < dateCountMax && data->dailys[codeIndex].start[dateIndex+1] == 0){
                            continue;
                        }
                        //add signal
                        dailyCount++;
                        strMsignal* new = malloc(sizeof(strMsignal) * dailyCount);
                        if(sumData->dailySignal[dateIndex]!=NULL){
                            memcpy(new, sumData->dailySignal[dateIndex], sizeof(strMsignal) * (dailyCount-1));
                        }
                        free(sumData->dailySignal[dateIndex]);
                        new[dailyCount-1].buyDateIndex = dateIndex;
                        new[dailyCount-1].codeIndex = codeIndex;
                        if(dateIndex+1 >= dateCountMax){
                            new[dailyCount-1].buyValue = -1;
                        } else {
                            new[dailyCount-1].buyValue = data->dailys[codeIndex].start[dateIndex+1];
                        }
                        sumData->dailySignal[dateIndex] = new;
                    }
                }
                sumData->dailyCount[dateIndex] = dailyCount;
            }
        }
        
        #pragma omp section
        {
            for(int dateIndex=startDateIndex+(endDateIndex-startDateIndex)*3/4; dateIndex<endDateIndex; dateIndex++)
            {
                int dailyCount = 0;
                for(int codeIndex=0; codeIndex<data->count; codeIndex++)
                {
                    if(codeRule!=0 && dekidaka(data, dateIndex, codeIndex, codeRule)){
                        continue;
                    }
                    if(calc((const char **)logicWords, 0, wordCount-1, data, NULL, NULL, dateIndex, codeIndex) > 0)
                    {
                        if(data->dailys[codeIndex].count==0 || data->dailys[codeIndex].end[dateIndex]==0){
                            continue;
                        }
                        if(dateIndex+1 < dateCountMax && data->dailys[codeIndex].start[dateIndex+1] == 0){
                            continue;
                        }
                        //add signal
                        dailyCount++;
                        strMsignal* new = malloc(sizeof(strMsignal) * dailyCount);
                        if(sumData->dailySignal[dateIndex]!=NULL){
                            memcpy(new, sumData->dailySignal[dateIndex], sizeof(strMsignal) * (dailyCount-1));
                        }
                        free(sumData->dailySignal[dateIndex]);
                        new[dailyCount-1].buyDateIndex = dateIndex;
                        new[dailyCount-1].codeIndex = codeIndex;
                        if(dateIndex+1 >= dateCountMax){
                            new[dailyCount-1].buyValue = -1;
                        } else {
                            new[dailyCount-1].buyValue = data->dailys[codeIndex].start[dateIndex+1];
                        }
                        sumData->dailySignal[dateIndex] = new;
                    }
                }
                sumData->dailyCount[dateIndex] = dailyCount;
                
                if(dateIndex%10 == 0){
                    char buf[256];
                    sprintf(buf, "%d%%", (dateIndex-(startDateIndex+(endDateIndex-startDateIndex)*3/4))*100/((endDateIndex-startDateIndex)/4));
                    printCalc(buf, 0);
                    printf("%s\n", buf);
                }
            }
        }
    }

    //発注の価格がおかしいようなので、チェックする
    /*for(int dateIndex=0; dateIndex<dateCountMax; dateIndex++)
    {
        for(int i=0; i<sumData->dailyCount[dateIndex]; i++)
        {
            strMsignal* signal = &sumData->dailySignal[dateIndex][i];
            
            if(dateIndex != signal->buyDateIndex)
            {
                printf("dateIndex error 1\n");
            }
            
            if(signal->buyValue != data->dailys[signal->codeIndex].start[dateIndex+1])
            {
                printf("buyValue error 2\n");
            }
            
            if(signal->buyValue==0)
            {
                printf("buyValue error 0\n");
            }
            
            if(data->codes[signal->codeIndex]==6837){
                printf("buyValue %d\n",signal->buyValue);
                printf("dateIndex %d\n",dateIndex);
            }
        }
    }*/

    for(int i=0; i<wordCount; i++)
    {
        free(logicWords[i]);
    }

    return sumData;
}

static int dekidaka(strMdata* data, int dateIndex, int codeIndex, int codeRule)
{
    if(codeIndex==0) return 1;
    if(data->dailys[codeIndex].end==NULL) return 1;
    long volume = ((long)data->dailys[codeIndex].end[dateIndex]) * data->dailys[codeIndex].volume[dateIndex];
    
    if(codeRule==1) //大型株のみ
    {
        if(dateIndex-10 < 0){
            return 0;
        }
        long volume2 = ((long)data->dailys[codeIndex].end[dateIndex-1]) * data->dailys[codeIndex].volume[dateIndex-5];
        long volume3 = ((long)data->dailys[codeIndex].end[dateIndex-2]) * data->dailys[codeIndex].volume[dateIndex-10];
        if(volume > 500000 * 500 && volume2 > 500000 * 500 && volume3 > 500000 * 500){ //ここ最近の出来高が投資金額の500倍は欲しい
            return 1;
        } else {
            return 0;
        }
    }
    if(codeRule==2) //出来高が少ないのは避ける
    {
        if(volume > 500000 * 100){ //出来高が投資金額の100倍は欲しい
            return 1;
        } else {
            return 0;
        }
    }
    if(codeRule==3) //出来高がとても少ないのは避ける
    {
        if(volume > 500000 * 30){ //出来高が投資金額の30倍は欲しい
            return 1;
        } else {
            return 0;
        }
    }
    if(codeRule==4) //小型株のみ
    {
        if(dateIndex-2 < 0){
            return 0;
        }
        long volume2 = ((long)data->dailys[codeIndex].end[dateIndex-1]) * data->dailys[codeIndex].volume[dateIndex-5];
        long volume3 = ((long)data->dailys[codeIndex].end[dateIndex-2]) * data->dailys[codeIndex].volume[dateIndex-10];
        if(volume2 + volume3 < 500000 * 600){ //ここ最近の出来高が投資金額の300倍以下
            return 1;
        } else {
            return 0;
        }
    }
    return 1;
}

static int filter(strMdata* data, int dateIndex, const char** sig2Words, int sig2WordCount)
{
    if(sig2WordCount==0) return 1;

    int dummyCodeIndex = -1;
    
    return calc(sig2Words, 0, sig2WordCount-1, data, NULL, NULL, dateIndex, dummyCodeIndex) > 0.0;
}

