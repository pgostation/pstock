//
//  strLbacktestRun.c
//  pstock
//
//  Created by takayoshi on 2016/02/02.
//  Copyright © 2016年 pgostation. All rights reserved.
//

#include <stdio.h>
#include "strLbacktestRun.h"
#include "strLbacktestInit.h"
#include "strMsignal.h"
#include "xmapString.h"
#include "calc.h"

static void tejimai(int dateIndex, int datemax, strMdata* data, strMsignal* positionSignal, int i, long* pCache, long* pYearProfit, float sellCalc, float lcCalc, strMsignal* positionSignalList, int positionSignalListMax, int* pPositionCount);
static int codeIndexFromCode(strMdata* data, int code);

void strLbacktest_run(const char* path, strMdata* data, strLdata* dataL, int startDateIndex, int endDateIndex)
{
    const long initialCache = 10000000;
    long cache = initialCache;
    int positionCount = 0;
    strMsignal positionSignal[dataL->split];
    const int positionSignalListMax = 20000;
    strMsignal positionSignalList[positionSignalListMax] = {0x00};
    int positionSignalListCounter = 0;
    
    const int datemax = data->dailys[MAX_DATA_COUNT-1].count;
    long cacheHistory[datemax];
    int positionCountHistory[datemax];
    strMsignal positionSignalHistory[datemax][dataL->split];
    char* strategyHistory[datemax][10];
    
    char* todayStrategy[10];
    int todayStrategyPosition[10];
    
    long yearProfit = 0;
    
    memset(positionSignal, 0x00, sizeof(strMsignal) * dataL->split);
    
    //1日ずつずらしながらバックテスト
    for (int dateIndex=startDateIndex; dateIndex<endDateIndex; dateIndex++)
    {
        strategyHistory[dateIndex][0] = NULL;
        cacheHistory[dateIndex] = -1;
        
        //本日のポジション
        int big, mid=0;
        {
            for (big=0; big<10; big++)
            {
                float ruleResult = 1;
                if (dataL->bigRuleWordCount[big]>0)
                {
                    ruleResult = calc(dataL->bigRule[big], 0, dataL->bigRuleWordCount[big]-1, data, NULL, NULL, dateIndex, -1);
                }
                if (ruleResult>0)
                {
                    for (mid=0; mid<10; mid++)
                    {
                        float smallRuleResult = 1;
                        if (dataL->smallRuleWordCount[big][mid]>0)
                        {
                            smallRuleResult = calc(dataL->smallRule[big][mid], 0, dataL->smallRuleWordCount[big][mid]-1, data, NULL, NULL, dateIndex, -1);
                        }
                        if (smallRuleResult>0)
                        {
                            break;
                        }
                    }
                    break;
                }
            }
            if((data->date[dateIndex]&0xff) > (data->date[dateIndex+1]&0xff)){
                printf("%d/%d %d %d\n", (data->date[dateIndex]>>16)+1900, (data->date[dateIndex]>>8)&0xff, big, mid);
            }
            
            for (int small=0; small<10; small++)
            {
                todayStrategy[small] = dataL->strategy[big][mid][small];
                todayStrategyPosition[small] = dataL->position[big][mid][small];
            }
            /*if((data->date[dateIndex]&0xff) > (data->date[dateIndex+1]&0xff) && mid==4){
                for (int small=0; small<10; small++)
                {
                    printf("todayStrategy[%d]=%s - %d\n", small, todayStrategy[small], todayStrategyPosition[small]);
                }
            }*/
        }
        if (big==10) {
            printf("Error: bigRule overflow\n");
            continue;
        }
        
        //枠が空いていれば、発注する
        if (positionCount < dataL->split && cache>0)
        {
            for (int small=0; small<10; small++)
            {
                if (dataL->filter2History[big][mid][small]==NULL) {
                    continue;
                }
                
                //このストラテジー用の枠は余っているのか？
                if (todayStrategyPosition[small] == 0) {
                    continue;
                }
                int thisStrategyCount = 0;
                for (int i=0; i<dataL->split && i<positionCount; i++)
                {
                    if (strcmp(todayStrategy[small], positionSignal[i].strategy)==0)
                    {
                        thisStrategyCount++;
                    }
                }
                if (todayStrategyPosition[small] <= thisStrategyCount) {
                    continue;
                }
                
                //シグナル数フィルター
                int signal2filterResult = dataL->filter2History[big][mid][small][dateIndex];
                if (!signal2filterResult)
                {
                    break;
                }
                if (dataL->codesCount[big][mid][small][dateIndex]==0)
                {
                    break;
                }
                
                {
                    double orderList[dataL->codesCount[big][mid][small][dateIndex]];
                    int codeIndexs[1000];
                
                    //最も優先度の高いものを発注する
                    {
                        for (int i=0; i<dataL->codesCount[big][mid][small][dateIndex] && i<1000; i++)
                        {
                            codeIndexs[i] = codeIndexFromCode(data, dataL->codes[big][mid][small][dateIndex][i]);
                            
                            orderList[i] = -9999999999999.9;
                            
                            //すでに保有していれば発注しない
                            int flag = 0;
                            for (int j=0; j<positionCount; j++)
                            {
                                if (data->codes[positionSignal[j].codeIndex] == dataL->codes[big][mid][small][dateIndex][i])
                                {
                                    flag = 1;
                                    break;
                                }
                            }
                            if (flag) {
                                orderList[i] = -9999999999999.9;
                                continue;
                            }
                            
                            //変な銘柄は発注しない
                            int code = dataL->codes[big][mid][small][dateIndex][i];
                            if (code==1773) {
                                continue;
                            }
                            
                            //貸借銘柄じゃないので空売りできない
                            if (dataL->isKarauri[big][mid][small])
                            {
                                int karauriOk = data->companys[codeIndexs[i]].isKarauriOk;
                                if(!karauriOk){
                                    continue;
                                }
                            }
                            
                            //優先度を計算
                            orderList[i] = calc(dataL->orderWords[big][mid][small], 0, dataL->orderWordCount[big][mid][small]-1, data, NULL, NULL, dateIndex, codeIndexs[i]);
                            
                        }
                    }
                
                    
                    while(positionCount < dataL->split && cache>0)
                    {
                        double orderMax = -9999999999999.9;
                        int codeIndexMax = -1;
                        int buyValue = 0;
                        int buyUnit = 0;
                        int iMax = 0;
                        for (int i=0; i<dataL->codesCount[big][mid][small][dateIndex]; i++)
                        {
                            if (orderList[i] > orderMax)
                            {
                                if (codeIndexs[i]==0) {
                                    continue;
                                }
                                int tmpValue = data->dailys[codeIndexs[i]].end[dateIndex];
                                if (tmpValue==0) {
                                    continue;
                                }
                                long tmpCache = cache/(dataL->split - positionCount);
                                if (tmpCache > initialCache / dataL->split) {
                                    tmpCache = initialCache / dataL->split;
                                }
                                int tmpUnit = (int)(tmpCache / tmpValue);
                                int companyUnit = data->companys[codeIndexs[i]].unit;
                                if (companyUnit > 0 && tmpUnit < companyUnit)
                                {
                                    continue;
                                }
                                int tmpUnit2 = 0;
                                if (companyUnit > 0) {
                                    tmpUnit2 = (tmpUnit / companyUnit) * companyUnit;
                                } else {
                                    tmpUnit2 = tmpUnit;
                                }
                                if (tmpUnit2==0) {
                                    continue;
                                }
                                iMax = i;
                                orderMax = orderList[i];
                                codeIndexMax = codeIndexs[i];
                                buyValue = tmpValue;
                                buyUnit = tmpUnit2;
                            }
                        }
                
                        if (codeIndexMax==-1) {
                            break;
                        }
                        orderList[iMax] = -9999999999999.9;
                    
                
                        //発注
                        long money = buyValue * buyUnit;
                        cache -= money;
                        positionCount++;
                        positionSignal[positionCount-1].strategy = dataL->strategy[big][mid][small];
                        positionSignal[positionCount-1].codeIndex = codeIndexMax;
                        int tmpValue = 0;
                        if (dateIndex+1 < endDateIndex) {
                            tmpValue = data->dailys[codeIndexMax].start[dateIndex+1];
                        }
                        if (tmpValue==0) {
                            tmpValue = buyValue;
                        }
                        positionSignal[positionCount-1].buyValue = tmpValue;
                        positionSignal[positionCount-1].buyUnit = buyUnit;
                        positionSignal[positionCount-1].buyDateIndex = dateIndex;
                        positionSignal[positionCount-1].isKarauri = dataL->isKarauri[big][mid][small];
                        positionSignal[positionCount-1].sellReason = -1;
                        positionSignal[positionCount-1].sellDateIndex = endDateIndex;
                        positionSignal[positionCount-1].sellValue = -1;
                        positionSignal[positionCount-1].slippage = -1;
                        positionSignal[positionCount-1].zei = -1;
                        positionSignal[positionCount-1].big = big;
                        positionSignal[positionCount-1].mid = mid;
                        positionSignal[positionCount-1].small = small;
                        
                        //過去の履歴に移動
                        positionSignal[positionCount-1].historyIndex = positionSignalListCounter;
                        if (positionSignalListCounter<positionSignalListMax) {
                            memcpy(&positionSignalList[positionSignalListCounter], &positionSignal[positionCount-1], sizeof(strMsignal));
                            positionSignalListCounter++;
                        } else {
                            printf("Error:strLbacktest_in():positionSignalList size over\n");
                        }
                        
                        //printf("buy:cache=%ld, code=%d, positionCount=%d\n", cache, data->codes[positionSignal[positionCount-1].codeIndex], positionCount);
                    }
                }
            }
        }
        
        
        //手仕舞い条件を計算する
        for (int i=positionCount-1; i>=0; i--)
        {
            if (dateIndex < endDateIndex-1 && positionSignal[i].buyDateIndex == dateIndex)
            {
                //本日の発注なので、まだ持っていない
                continue;
            }
            
            int ibig = positionSignal[i].big;
            int imid = positionSignal[i].mid;
            int ismall = positionSignal[i].small;
            
            float sellCalc = calc(dataL->sellWords[ibig][imid][ismall], 0, dataL->sellWordCount[ibig][imid][ismall]-1, data, &positionSignal[i], NULL, dateIndex, positionSignal[i].codeIndex);
            float lcCalc = 0.0;
            
            if ( sellCalc <= 0.0 ) {
                lcCalc = calc(dataL->lcWords[ibig][imid][ismall], 0, dataL->lcWordCount[ibig][imid][ismall]-1, data, &positionSignal[i], NULL, dateIndex, positionSignal[i].codeIndex) > 0.0;
            }
            
            if ( sellCalc > 0.0 ||
                lcCalc > 0.0 ||
                ( dateIndex < endDateIndex-2 && data->dailys[positionSignal[i].codeIndex].start[dateIndex+1]==0 && data->dailys[positionSignal[i].codeIndex].start[dateIndex+2]==0 ) ||
                dateIndex >= endDateIndex-1 )
            {
                //printf("sell:holdDays=%d code=%d buyValue=%d sellValue=%d ", dateIndex - positionSignal[i].buyDateIndex, data->codes[positionSignal[i].codeIndex], positionSignal[i].buyValue, data->dailys[positionSignal[i].codeIndex].start[dateIndex+1<datemax?dateIndex+1:dateIndex]);
                
                //手仕舞い
                tejimai(dateIndex, datemax, data, positionSignal, i, &cache, &yearProfit, sellCalc,
                 lcCalc, positionSignalList, positionSignalListMax, &positionCount);
                
                //printf(" cache=%ld\n", cache);
            }
        }
        
        //本日のポジションからはみ出すものは強制手仕舞い
        {
            int smallStrategyCount[10] = {0x00};
            for (int i=0; i<dataL->split && i<positionCount; i++)
            {
                int flag = 0;
                for (int small=0; small<10; small++)
                {
                    if(todayStrategy[small]==NULL) continue;
                    if(positionSignal[i].strategy==NULL) continue;
                    if (strcmp(todayStrategy[small], positionSignal[i].strategy)==0)
                    {
                        smallStrategyCount[small]++;
                        if (smallStrategyCount[small] > todayStrategyPosition[small])
                        {
                        
                        }
                        flag = 1;
                        break;
                    }
                }
                if (flag==0) {
                    //強制手仕舞い
                    float sellCalc = -1.0;
                    float lcCalc = -1.0;
                    tejimai(dateIndex, datemax, data, positionSignal, i, &cache, &yearProfit, sellCalc,
                     lcCalc, positionSignalList, positionSignalListMax, &positionCount);
                }
            }
        }
        
        //年が変わると、年間利益額をリセットする
        if (dateIndex+1 < datemax && data->date[dateIndex+1]>>16 > data->date[dateIndex]>>16)
        {
            yearProfit = 0;
        }
        
        //資産額計算のための履歴データ
        {
            cacheHistory[dateIndex] = cache;
            positionCountHistory[dateIndex] = positionCount;
            memcpy(positionSignalHistory[dateIndex], positionSignal, sizeof(positionSignal));
        }
        
        memcpy(strategyHistory[dateIndex], todayStrategy, sizeof(todayStrategy));
    }
    
    //資産額の計算
    long stockHistory[datemax];
    {
        for (int dateIndex=startDateIndex; dateIndex<endDateIndex; dateIndex++)
        {
            stockHistory[dateIndex] = 0;
            for (int i=0; i<positionCountHistory[dateIndex]; i++)
            {
                int unit = positionSignalHistory[dateIndex][i].buyUnit;
                int codeIndex = positionSignalHistory[dateIndex][i].codeIndex;
                
                int lastEnd = data->dailys[codeIndex].end[dateIndex];
                for (int j=1; lastEnd==0 && dateIndex-j > 0 && j<30; j++) {
                    lastEnd = data->dailys[codeIndex].end[dateIndex-j];
                }
                long stock;
                if (positionSignalHistory[dateIndex][i].isKarauri) {
                    //空売りで仮想的に減らしていたお金
                    long base = positionSignalHistory[dateIndex][i].buyValue * (long)unit;
                    //空売った時と買い戻した値段の差額が利益/損失となる
                    long profit = (positionSignalHistory[dateIndex][i].buyValue - lastEnd) * (long)unit;
                    stock = base + profit;
                } else {
                    //買ったお金を戻す
                    long base = positionSignalHistory[dateIndex][i].buyValue * (long)unit;
                    //利益/損失
                    long profit = (lastEnd - positionSignalHistory[dateIndex][i].buyValue) * (long)unit;
                    if (profit > base * 3) {
                        profit = base * 3;
                    }
                    stock = base + profit;
                }
                stockHistory[dateIndex] += stock;
            }
        }
    }
    
    //allshisan.xmapの保存
    {
        xmap_t* xmap = xmap_load(NULL);
        
        for (int dateIndex=startDateIndex; dateIndex<endDateIndex; dateIndex++)
        {
            char* key = apr_palloc(xmap->pool, 32);
            char* value = apr_palloc(xmap->pool, 32);
            snprintf(key, 32-1, "date%d", dateIndex);
            snprintf(value, 32-1, "%d", data->date[dateIndex]);
            xmap_set(xmap, key, value);
        }
        
        for (int dateIndex=startDateIndex; dateIndex<endDateIndex; dateIndex++)
        {
            char* key = apr_palloc(xmap->pool, 32);
            char* value = apr_palloc(xmap->pool, 32);
            snprintf(key, 32-1, "cache%d", dateIndex);
            snprintf(value, 32-1, "%ld", cacheHistory[dateIndex]);
            xmap_set(xmap, key, value);
        }
        
        for (int dateIndex=startDateIndex; dateIndex<endDateIndex; dateIndex++)
        {
            char* key = apr_palloc(xmap->pool, 32);
            char* value = apr_palloc(xmap->pool, 32);
            snprintf(key, 32-1, "stock%d", dateIndex);
            snprintf(value, 32-1, "%ld", stockHistory[dateIndex]);
            xmap_set(xmap, key, value);
        }
        
        for (int dateIndex=startDateIndex; dateIndex<endDateIndex; dateIndex++)
        {
            char* key = apr_palloc(xmap->pool, 32);
            char* value = apr_palloc(xmap->pool, 2048);
            value[0] = '\0';
            snprintf(key, 32-1, "strategy%d", dateIndex);
            for(int i=0; i<10; i++){
                if (strategyHistory[dateIndex][i]==NULL) break;
                strcat(value, strategyHistory[dateIndex][i]);
                strcat(value, "\n");
            }
            xmap_set(xmap, key, value);
        }
        
        /*for (int dateIndex=0; dateIndex<datemax; dateIndex++)
        {
            char* key = apr_palloc(xmap->pool, 32);
            char* value = apr_palloc(xmap->pool, 32);
            snprintf(key, 32-1, "positionCountHistory%d", dateIndex);
            snprintf(value, 32-1, "%d", positionCountHistory[dateIndex]);
            xmap_set(xmap, key, value);
        }*/
        
        char filePath[256];
        snprintf(filePath, sizeof(filePath)-1, "%s/allshisan.xmap", path);
        xmap_saveWithFilename(xmap, filePath);
        xmap_release(xmap);
    }
    
    //allSignals.xmapの保存
    {
        xmap_t* xmap = xmap_load(NULL);
        
        char* key = apr_palloc(xmap->pool, 32);
        char* value = apr_palloc(xmap->pool, 32);
        snprintf(key, 32-1, "date");
        snprintf(value, 32-1, "%d", data->date[datemax]);
        xmap_set(xmap, key, value);
        
        
        key = apr_palloc(xmap->pool, 32);
        value = apr_palloc(xmap->pool, 32);
        snprintf(key, 32-1, "signalCount");
        snprintf(value, 32-1, "%d", positionSignalListCounter);
        xmap_set(xmap, key, value);
        
        char strategyName[256] = "";
        for (int c=(int)strlen(path)-1; c>=0; c--)
        {
            if (path[c]=='/')
            {
                strcpy(strategyName, &path[c+1]);
                if (strategyName[strlen(strategyName)-1]=='/') {
                    strategyName[strlen(strategyName)-1]='\0';
                }
            }
        }
        
        key = apr_palloc(xmap->pool, 32);
        value = apr_palloc(xmap->pool, 128);
        snprintf(key, 32-1, "strategyName");
        snprintf(value, 128-1, "%s", strategyName);
        xmap_set(xmap, key, value);
        
        for (int positionSignalListIndex=0; positionSignalListIndex < positionSignalListCounter; positionSignalListIndex++)
        {
            strMsignal* signal = &positionSignalList[positionSignalListIndex];
            
            char* key = apr_palloc(xmap->pool, 32);
            char* value = apr_palloc(xmap->pool, 32);
            snprintf(key, 32-1, "buyDateIndex%d", positionSignalListIndex);
            snprintf(value, 32-1, "%d", signal->buyDateIndex);
            xmap_set(xmap, key, value);
            
            key = apr_palloc(xmap->pool, 32);
            value = apr_palloc(xmap->pool, 32);
            snprintf(key, 32-1, "buyDate__%d", positionSignalListIndex);
            snprintf(value, 32-1, "%d", data->date[signal->buyDateIndex]);
            xmap_set(xmap, key, value);
            
            key = apr_palloc(xmap->pool, 32);
            value = apr_palloc(xmap->pool, 32);
            snprintf(key, 32-1, "buyValue%d", positionSignalListIndex);
            snprintf(value, 32-1, "%d", signal->buyValue);
            xmap_set(xmap, key, value);
            
            key = apr_palloc(xmap->pool, 32);
            value = apr_palloc(xmap->pool, 32);
            snprintf(key, 32-1, "code%d", positionSignalListIndex);
            snprintf(value, 32-1, "%d", data->codes[signal->codeIndex]);
            xmap_set(xmap, key, value);
            
            key = apr_palloc(xmap->pool, 32);
            value = apr_palloc(xmap->pool, 32);
            snprintf(key, 32-1, "rule%d", positionSignalListIndex);
            snprintf(value, 32-1, "%s", signal->isKarauri?"空売り":"買い");
            xmap_set(xmap, key, value);
            
            key = apr_palloc(xmap->pool, 32);
            value = apr_palloc(xmap->pool, 32);
            snprintf(key, 32-1, "sellDateIndex%d", positionSignalListIndex);
            snprintf(value, 32-1, "%d", signal->sellDateIndex);
            xmap_set(xmap, key, value);
            
            key = apr_palloc(xmap->pool, 32);
            value = apr_palloc(xmap->pool, 32);
            snprintf(key, 32-1, "sellDate__%d", positionSignalListIndex);
            snprintf(value, 32-1, "%d", data->date[signal->sellDateIndex]);
            xmap_set(xmap, key, value);
            
            key = apr_palloc(xmap->pool, 32);
            value = apr_palloc(xmap->pool, 32);
            snprintf(key, 32-1, "sellReason%d", positionSignalListIndex);
            snprintf(value, 32-1, "%d", signal->sellReason);
            xmap_set(xmap, key, value);
            
            key = apr_palloc(xmap->pool, 32);
            value = apr_palloc(xmap->pool, 32);
            snprintf(key, 32-1, "sellValue%d", positionSignalListIndex);
            snprintf(value, 32-1, "%d", signal->sellValue);
            xmap_set(xmap, key, value);
            
            key = apr_palloc(xmap->pool, 32);
            value = apr_palloc(xmap->pool, 32);
            snprintf(key, 32-1, "slippage%d", positionSignalListIndex);
            snprintf(value, 32-1, "%d", signal->slippage);
            xmap_set(xmap, key, value);
            
            key = apr_palloc(xmap->pool, 32);
            value = apr_palloc(xmap->pool, 32);
            snprintf(key, 32-1, "unit%d", positionSignalListIndex);
            snprintf(value, 32-1, "%d", signal->buyUnit);
            xmap_set(xmap, key, value);
            
            key = apr_palloc(xmap->pool, 32);
            value = apr_palloc(xmap->pool, 32);
            snprintf(key, 32-1, "zei%d", positionSignalListIndex);
            snprintf(value, 32-1, "%d", signal->zei);
            xmap_set(xmap, key, value);
            
            key = apr_palloc(xmap->pool, 32);
            value = apr_palloc(xmap->pool, 128);
            snprintf(key, 32-1, "strategyName%d", positionSignalListIndex);
            snprintf(value, 128-1, "%s", signal->strategy);
            xmap_set(xmap, key, value);
            
            key = apr_palloc(xmap->pool, 32);
            value = apr_palloc(xmap->pool, 32);
            snprintf(key, 32-1, "numABC%d", positionSignalListIndex);
            snprintf(value, 32-1, "%d %d %d", signal->big+1, signal->mid+1, signal->small+1);
            xmap_set(xmap, key, value);
        }
        
        xmap_set(xmap, "HoldDate", "0");
        
        char filePath[256];
        snprintf(filePath, sizeof(filePath)-1, "%s/allSignals.xmap", path);
        xmap_saveWithFilename(xmap, filePath);
        xmap_release(xmap);
    }
}

static void tejimai(int dateIndex, int datemax, strMdata* data, strMsignal* positionSignal, int i, long* pCache, long* pYearProfit, float sellCalc, float lcCalc, strMsignal* positionSignalList, int positionSignalListMax, int* pPositionCount)
{
    int zei = 0;
    long slippage = 0;
    int nextStart = 0;
    long nextVolume = 0;
    if (dateIndex+1<datemax) {
        nextStart = data->dailys[positionSignal[i].codeIndex].start[dateIndex+1];
        nextVolume = data->dailys[positionSignal[i].codeIndex].volume[dateIndex+1];
    } else {
        nextStart= data->dailys[positionSignal[i].codeIndex].end[dateIndex];
    }
    for (int j=2; nextStart==0 && dateIndex+j < datemax && j<5; j++) {
        nextStart = data->dailys[positionSignal[i].codeIndex].start[dateIndex+j];
        nextVolume = data->dailys[positionSignal[i].codeIndex].volume[dateIndex+j];
    }
    if (nextStart==0) {
        //閑散としすぎ。上場廃止？
        nextStart = data->dailys[positionSignal[i].codeIndex].end[dateIndex];
        nextVolume = 0;
    }
    if (positionSignal[i].isKarauri) {
        //空売りで仮想的に減らしていたお金を元に戻す
        long money = positionSignal[i].buyValue * (long)positionSignal[i].buyUnit;
        (*pCache) += money;
        //空売った時と買い戻した値段の差額が利益/損失となる
        long profit = (positionSignal[i].buyValue - nextStart) * (long)positionSignal[i].buyUnit;
        (*pCache) += profit;
        if (profit>0 && *pYearProfit+profit >= 0) {
            zei = profit * 0.2;
        }
        if (profit<0 && *pYearProfit > 0) {
            zei = profit * 0.2;
        }
        (*pYearProfit) += profit;
    } else {
        //買ったお金を元に戻す
        long money = positionSignal[i].buyValue * (long)positionSignal[i].buyUnit;
        (*pCache) += money;
        //利益/損失
        long profit = (nextStart - positionSignal[i].buyValue) * (long)positionSignal[i].buyUnit;
        if ( profit > money * 3 ) {
            profit = money * 3; //極端な銘柄の排除
        }
        (*pCache) += profit;
        if (profit>0 && *pYearProfit+profit >= 0) {
            zei = profit * 0.2;
        }
        if (profit<0 && *pYearProfit > 0) {
            zei = profit * 0.2;
        }
        (*pYearProfit) += profit;
    }
    (*pCache) -= zei;
    
    //スリッページ計算（発注時には計算せず、手仕舞い時のみ計算している）
    if (nextVolume > 1000 && positionSignal[i].buyUnit >= nextVolume) {
        slippage = (nextStart * positionSignal[i].buyUnit) / 20;
    }
    else if (nextVolume > 1000 && positionSignal[i].buyUnit >= nextVolume/2) {
        slippage = (nextStart * positionSignal[i].buyUnit) / 30;
    }
    else if (nextVolume > 1000 && positionSignal[i].buyUnit >= nextVolume/5) {
        slippage = (nextStart * positionSignal[i].buyUnit) / 40;
    }
    else if (positionSignal[i].buyUnit >= nextVolume/10) {
        slippage = (nextStart * positionSignal[i].buyUnit) / 50;
    }
    else if (positionSignal[i].buyUnit >= nextVolume/20) {
        slippage = (nextStart * positionSignal[i].buyUnit) / 75;
    }
    else if (positionSignal[i].buyUnit >= nextVolume/50) {
        slippage = (nextStart * positionSignal[i].buyUnit) / 100;
    }
    else if (positionSignal[i].buyUnit >= nextVolume/100) {
        slippage = (nextStart * positionSignal[i].buyUnit) / 200;
    }
    
    (*pCache) -= slippage;
    (*pYearProfit) -= slippage;
    
    positionSignal[i].sellDateIndex = dateIndex;
    positionSignal[i].sellValue = nextStart;
    positionSignal[i].slippage = (int)slippage;
    positionSignal[i].zei = zei;
    if (sellCalc == -1.0 && lcCalc == -1.0) {
        positionSignal[i].sellReason = 3;
    } else if (sellCalc > 0.0) {
        positionSignal[i].sellReason = 1;
    } else if (lcCalc > 0.0 ) {
        positionSignal[i].sellReason = 2;
    } else if (dateIndex >= datemax-1) {
        positionSignal[i].sellReason = 4;
    } else {
        positionSignal[i].sellReason = 0;
    }
    
    //過去の履歴に決済情報をコピー
    if (positionSignal[i].historyIndex < positionSignalListMax) {
        memcpy(&positionSignalList[positionSignal[i].historyIndex], &positionSignal[i], sizeof(strMsignal));
    }
    
    //現在のポジションから削除
    memcpy(&positionSignal[i], &positionSignal[(*pPositionCount)-1], sizeof(strMsignal));
    (*pPositionCount)--;
}

static int codeIndexFromCode(strMdata* data, int code)
{
    int i = 2000;
    int lastI = i;
    while(i>100 && i<data->count-100)
    {
        if (code==data->codes[i])
        {
            return i;
        }
        else if (code < data->codes[i])
        {
            if (i > lastI) break;
            lastI = i;
            i-=100;
        }
        else if (code > data->codes[i])
        {
            if (i < lastI) break;
            lastI = i;
            i+=100;
        }
    }
    
    int j=0;
    lastI = i;
    while(i>10 && i<data->count-10 && j<10)
    {
        if (code==data->codes[i])
        {
            return i;
        }
        else if (code < data->codes[i])
        {
            if (i > lastI) break;
            lastI = i;
            i-=10;
        }
        else if (code > data->codes[i])
        {
            if (i < lastI) break;
            lastI = i;
            i+=10;
        }
        j++;
    }
    
    j=0;
    while(i>1 && i<data->count && j<10)
    {
        if (code==data->codes[i])
        {
            return i;
        }
        else if (code < data->codes[i])
        {
            i-=1;
        }
        else if (code > data->codes[i])
        {
            i+=1;
        }
        j++;
    }
    
    return 0;
}
