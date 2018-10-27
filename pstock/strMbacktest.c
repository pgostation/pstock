//
//  strMbacktest.c
//  pstock
//
//  Created by takayoshi on 2016/01/24.
//  Copyright © 2016年 pgostation. All rights reserved.
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <libiomp/omp.h>
#include "strMbacktest.h"
#include "xmapString.h"
#include "calc.h"

static void strMdbacktest_in(const char* path, strMdata* data, strMsignalSum* signals,
  const char** sellWords, int sellWordCount, const char** lcWords, int lcWordCount, const char** sig2Words, int sig2WordCount, const char** orderWords, int orderWordCount, int isKarauri, int split, int startDateIndex, int endDateIndex);
static int signal2filter(strMdata* data, strMsignalSum* signals, int dateIndex, const char** sig2Words, int sig2WordCount);

void strMbacktest_start(const char* path, strMdata* data, strMsignalSum* signals, int startDateIndex, int endDateIndex)
{
    const char* sellWords[64] = {0x00};
    int sellWordCount = 0;
    const char* lcWords[64] = {0x00};
    int lcWordCount = 0;
    const char* sig2Words[64] = {0x00};
    int sig2WordCount = 0;
    const char* orderWords[64] = {0x00};
    int orderWordCount = 0;
    int isKarauri = 0;
    int split = 10; //分割数
    xmap_t* xmap;
    
    //xmap取得
    {
        char filePath[256];
        snprintf(filePath, sizeof(filePath)-1, "%s/conf.xmap", path);
        xmap = xmap_load(filePath);
        
        if(xmap==NULL)
        {
            printf("Error: strMdbacktest_start(): conf.xmap is NULL\n");
            return;
        }
        
        //期間指定の場合、結果保存ディレクトリを変える
        if (startDateIndex!=0 || endDateIndex < data->dailys[MAX_DATA_COUNT-1].count)
        {
            for (int i=(int)strlen(path)-2; i>0; i--)
            {
                if (path[i] == '/')
                {
                    char strategyName[256] = {0};
                    char parentPath[512] = {0};
                    
                    strcpy(strategyName, &path[i+1]);
                    strncpy(parentPath, path, i);
                    strcat(parentPath, "-date/");
                    strcat(parentPath, strategyName);
                    path = parentPath;
                    //ディレクトリ作成
                    {
                        printf("mkdir %s\n", parentPath);
                        mkdir(parentPath, 0777);
                    }
                    break;
                }
            }
        }
        
        snprintf(filePath, sizeof(filePath)-1, "%s/saveconf.xmap", path);
        xmap_saveWithFilename(xmap, filePath);
    }
    
    //スペースで区切って単語配列取得
    {
        const char *sellRule = xmap_get(xmap, "sell");
        printf("sellRule:%s\n", sellRule);
        
        int quoteFlag = 0;
        int start = 0;
        const int sellRuleLen = (int)strlen(sellRule);
        for(int i=0; i<=sellRuleLen; i++)
        {
            if(!quoteFlag && ( sellRule[i]==' ' || sellRule[i]=='\n' || i==sellRuleLen) )
            {
                if(i-start==0)
                {
                    start++;
                    continue;
                }
                char *buf = malloc(i-start+1);
                strncpy(buf, &sellRule[start], i-start);
                buf[i-start] = '\0';
                sellWords[sellWordCount] = buf;
                sellWordCount++;
                start = i+1;
                continue;
            }
            if(sellRule[i]=='"')
            {
                quoteFlag = !quoteFlag;
            }
        }
    }
    
    {
        const char *lcRule = xmap_get(xmap, "lc");
        printf("lcRule:%s\n", lcRule);
        
        int quoteFlag = 0;
        int start = 0;
        const int lcRuleLen = (int)strlen(lcRule);
        for(int i=0; i<=lcRuleLen; i++)
        {
            if(!quoteFlag && ( lcRule[i]==' ' || lcRule[i]=='\n' || i==lcRuleLen) )
            {
                if(i-start==0)
                {
                    start++;
                    continue;
                }
                char *buf = malloc(i-start+1);
                strncpy(buf, &lcRule[start], i-start);
                buf[i-start] = '\0';
                lcWords[lcWordCount] = buf;
                lcWordCount++;
                start = i+1;
                continue;
            }
            if(lcRule[i]=='"')
            {
                quoteFlag = !quoteFlag;
            }
        }
    }
    
    {
        const char *sig2Rule = xmap_get(xmap, "filter2");
        printf("filter2:%s\n", sig2Rule);
        
        int quoteFlag = 0;
        int start = 0;
        const int sig2RuleLen = (int)strlen(sig2Rule);
        for(int i=0; i<=sig2RuleLen; i++)
        {
            if(!quoteFlag && ( sig2Rule[i]==' ' || sig2Rule[i]=='\n' || i==sig2RuleLen) )
            {
                if(i-start==0)
                {
                    start++;
                    continue;
                }
                char *buf = malloc(i-start+1);
                strncpy(buf, &sig2Rule[start], i-start);
                buf[i-start] = '\0';
                sig2Words[sig2WordCount] = buf;
                sig2WordCount++;
                start = i+1;
                continue;
            }
            if(sig2Rule[i]=='"')
            {
                quoteFlag = !quoteFlag;
            }
        }
    }
    
    {
        const char *orderRule = xmap_get(xmap, "order");
        printf("order:%s\n", orderRule);
        
        int quoteFlag = 0;
        int start = 0;
        const int orderRuleLen = (int)strlen(orderRule);
        for(int i=0; i<=orderRuleLen; i++)
        {
            if(!quoteFlag && ( orderRule[i]==' ' || orderRule[i]=='\n' || i==orderRuleLen) )
            {
                if(i-start==0)
                {
                    start++;
                    continue;
                }
                char *buf = malloc(i-start+1);
                strncpy(buf, &orderRule[start], i-start);
                buf[i-start] = '\0';
                orderWords[orderWordCount] = buf;
                orderWordCount++;
                start = i+1;
                continue;
            }
            if(orderRule[i]=='"')
            {
                quoteFlag = !quoteFlag;
            }
        }
    }
    
    {
        const char *xRule = xmap_get(xmap, "rule");
        printf("rule:%s\n", xRule);
        
        if(strcmp(xRule,"空売り")==0)
        {
            isKarauri = 1;
        }
    }
    
    {
        const char *splitStr = xmap_get(xmap, "split");
        if(splitStr!=NULL)
        {
            sscanf(splitStr, "%d", &split);
            if(split>SPLIT_MAX){
                split = SPLIT_MAX;
            }
        }
    }
    
    strMdbacktest_in(path, data, signals, sellWords, sellWordCount, lcWords, lcWordCount, sig2Words, sig2WordCount, orderWords, orderWordCount, isKarauri, split, startDateIndex, endDateIndex);
    
    xmap_release(xmap);
}

static void strMdbacktest_in(const char* path,strMdata* data, strMsignalSum* signals,
  const char** sellWords, int sellWordCount, const char** lcWords, int lcWordCount, const char** sig2Words, int sig2WordCount, const char** orderWords, int orderWordCount, int isKarauri, int split, int startDateIndex, int endDateIndex)
{
    const long initialCache = 10000000;
    long cache = initialCache;
    int positionCount = 0;
    strMsignal positionSignal[split];
    const int positionSignalListMax = 20000;
    strMsignal positionSignalList[positionSignalListMax] = {0x00};
    int positionSignalListCounter = 0;
    
    long cacheHistory[signals->count];
    int positionCountHistory[signals->count];
    strMsignal positionSignalHistory[signals->count][split];
    int filter2History[signals->count];
    
    long yearProfit = 0;
    
    memset(positionSignal, 0x00, sizeof(strMsignal) * split);
    
    //1日ずつずらしながらバックテスト
    for(int dateIndex=startDateIndex; dateIndex<endDateIndex; dateIndex++)
    {
        cacheHistory[dateIndex] = -1;
        
        //発注シグナルがあるので、発注する
        while(signals->dailyCount[dateIndex] > 0 && positionCount < split && cache>0)
        {
            //シグナル数フィルター
            int signal2filterResult = signal2filter(data, signals, dateIndex, sig2Words, sig2WordCount);
            filter2History[dateIndex] = signal2filterResult;
            if(!signal2filterResult)
            {
                break;
            }
            
            {
                double orderList[signals->dailyCount[dateIndex]];
            
                //最も優先度の高いものを発注する
                #pragma omp parallel
                #pragma omp sections
                {
                    #pragma omp section
                    for(int i=0; i<signals->dailyCount[dateIndex]/2; i++)
                    {
                        orderList[i] = -9999999999999.9;
                        
                        //すでに保有していれば発注しない
                        int flag = 0;
                        for(int j=0; j<positionCount; j++)
                        {
                            if(positionSignal[j].codeIndex == signals->dailySignal[dateIndex][i].codeIndex)
                            {
                                flag = 1;
                                break;
                            }
                        }
                        if(flag){
                            orderList[i] = -9999999999999.9;
                            continue;
                        }
                        
                        //変な銘柄は発注しない
                        int code = data->codes[signals->dailySignal[dateIndex][i].codeIndex];
                        if(code==1773){ //株価データが壊れている
                            continue;
                        }
                        
                        //貸借銘柄じゃないので空売りできない
                        if (isKarauri)
                        {
                            int karauriOk = data->companys[signals->dailySignal[dateIndex][i].codeIndex].isKarauriOk;
                            if(!karauriOk){
                                continue;
                            }
                        }
                        
                        //優先度を計算
                        orderList[i] = calc(orderWords, 0, orderWordCount-1, data, NULL, NULL, dateIndex, signals->dailySignal[dateIndex][i].codeIndex);
                        
                    }
                    
                    #pragma omp section
                    for(int i=signals->dailyCount[dateIndex]/2; i<signals->dailyCount[dateIndex]; i++)
                    {
                        orderList[i] = -9999999999999.9;
                        
                        //すでに保有していれば発注しない
                        int flag = 0;
                        for(int j=0; j<positionCount; j++)
                        {
                            if(positionSignal[j].codeIndex == signals->dailySignal[dateIndex][i].codeIndex)
                            {
                                flag = 1;
                                break;
                            }
                        }
                        if(flag) {
                            orderList[i] = -9999999999999.9;
                            continue;
                        }
                        
                        //変な銘柄は発注しない
                        int code = data->codes[signals->dailySignal[dateIndex][i].codeIndex];
                        if(code==1773){ //株価データが壊れている
                            continue;
                        }
                        
                        //貸借銘柄じゃないので空売りできない
                        if (isKarauri)
                        {
                            int karauriOk = data->companys[signals->dailySignal[dateIndex][i].codeIndex].isKarauriOk;
                            if(!karauriOk){
                                continue;
                            }
                        }
                        
                        //優先度を計算
                        orderList[i] = calc(orderWords, 0, orderWordCount-1, data, NULL, NULL, dateIndex, signals->dailySignal[dateIndex][i].codeIndex);
                    }
                }
                
                double orderMax = -9999999999999.9;
                strMsignal* signalMax = NULL;
                int buyValue = 0;
                int buyUnit = 0;
                for(int i=0; i<signals->dailyCount[dateIndex]; i++)
                {
                    if(orderList[i] > orderMax)
                    {
                        int tmpValue = signals->dailySignal[dateIndex][i].buyValue;
                        if(tmpValue==-1){
                            tmpValue = data->dailys[signals->dailySignal[dateIndex][i].codeIndex].end[dateIndex];
                        }
                        long tmpCache = cache/(split - positionCount);
                        if (tmpCache > initialCache / split) {
                            tmpCache = initialCache / split;
                        }
                        int tmpUnit = (int)(tmpCache / tmpValue);
                        int companyUnit = data->companys[signals->dailySignal[dateIndex][i].codeIndex].unit;
                        if(companyUnit > 0 && tmpUnit < companyUnit)
                        {
                            continue;
                        }
                        int tmpUnit2 = 0;
                        if(companyUnit > 0){
                            tmpUnit2 = (tmpUnit / companyUnit) * companyUnit;
                        } else {
                            tmpUnit2 = tmpUnit;
                        }
                        if(tmpUnit2==0) {
                            continue;
                        }
                        orderMax = orderList[i];
                        signalMax = &signals->dailySignal[dateIndex][i];
                        buyValue = tmpValue;
                        buyUnit = tmpUnit2;
                    }
                }
                if(signalMax==NULL){
                    break;
                }
                
                //発注
                long money = buyValue * buyUnit;
                cache -= money;
                positionCount++;
                memcpy(&positionSignal[positionCount-1], signalMax, sizeof(strMsignal));
                positionSignal[positionCount-1].buyUnit = buyUnit;
                positionSignal[positionCount-1].buyDateIndex = dateIndex;
                positionSignal[positionCount-1].isKarauri = isKarauri;
                positionSignal[positionCount-1].sellReason = -1;
                positionSignal[positionCount-1].sellDateIndex = endDateIndex;
                positionSignal[positionCount-1].sellValue = -1;
                positionSignal[positionCount-1].slippage = -1;
                positionSignal[positionCount-1].zei = -1;
                
                //過去の履歴に移動
                positionSignal[positionCount-1].historyIndex = positionSignalListCounter;
                if(positionSignalListCounter<positionSignalListMax){
                    memcpy(&positionSignalList[positionSignalListCounter], &positionSignal[positionCount-1], sizeof(strMsignal));
                    positionSignalListCounter++;
                } else {
                    printf("Error:strMdbacktest_in():positionSignalList size over\n");
                }
                
                //printf("buy:cache=%ld, code=%d, positionCount=%d\n", cache, data->codes[positionSignal[positionCount-1].codeIndex], positionCount);
            }
        }
        
        //手仕舞い条件を計算する
        for(int i=positionCount-1; i>=0; i--)
        {
            if(dateIndex < endDateIndex-1 && positionSignal[i].buyDateIndex == dateIndex)
            {
                //本日の発注なので、まだ持っていない
                continue;
            }
            
            float sellCalc = calc(sellWords, 0, sellWordCount-1, data, &positionSignal[i], NULL, dateIndex, positionSignal[i].codeIndex);
            float lcCalc = 0.0;
            
            if( sellCalc <= 0.0 ){
                lcCalc = calc(lcWords, 0, lcWordCount-1, data, &positionSignal[i], NULL, dateIndex, positionSignal[i].codeIndex) > 0.0;
            }
            
            if( sellCalc > 0.0 ||
                lcCalc > 0.0 ||
                ( dateIndex < endDateIndex-2 && data->dailys[positionSignal[i].codeIndex].start[dateIndex+1]==0 && data->dailys[positionSignal[i].codeIndex].start[dateIndex+2]==0 ) ||
                dateIndex >= endDateIndex-1 )
            {
                //printf("sell:holdDays=%d code=%d buyValue=%d sellValue=%d ", dateIndex - positionSignal[i].buyDateIndex, data->codes[positionSignal[i].codeIndex], positionSignal[i].buyValue, data->dailys[positionSignal[i].codeIndex].start[dateIndex+1<signals->count?dateIndex+1:dateIndex]);
                //手仕舞い
                int zei = 0;
                long slippage = 0;
                int nextStart = 0;
                long nextVolume = 0;
                if(dateIndex+1<signals->count){
                    nextStart = data->dailys[positionSignal[i].codeIndex].start[dateIndex+1];
                    nextVolume = data->dailys[positionSignal[i].codeIndex].volume[dateIndex+1];
                } else {
                    nextStart= data->dailys[positionSignal[i].codeIndex].end[dateIndex];
                }
                for(int j=2; nextStart==0 && dateIndex+j < signals->count && j<5; j++){
                    nextStart = data->dailys[positionSignal[i].codeIndex].start[dateIndex+j];
                    nextVolume = data->dailys[positionSignal[i].codeIndex].volume[dateIndex+j];
                }
                if(nextStart==0){
                    //閑散としすぎ。上場廃止？
                    nextStart = data->dailys[positionSignal[i].codeIndex].end[dateIndex];
                    nextVolume = 0;
                }
                if ( positionSignal[i].buyValue == -1 ) {
                    positionSignal[i].buyValue = nextStart;
                }
                if(isKarauri){
                    //空売りで仮想的に減らしていたお金を元に戻す
                    long money = positionSignal[i].buyValue * (long)positionSignal[i].buyUnit;
                    cache += money;
                    //空売った時と買い戻した値段の差額が利益/損失となる
                    long profit = (positionSignal[i].buyValue - nextStart) * (long)positionSignal[i].buyUnit;
                    cache += profit;
                    if(profit>0 && yearProfit+profit >= 0){
                        zei = profit * 0.2;
                    }
                    if(profit<0 && yearProfit > 0){
                        zei = profit * 0.2;
                    }
                    yearProfit += profit;
                } else {
                    //買ったお金を元に戻す
                    long money = positionSignal[i].buyValue * (long)positionSignal[i].buyUnit;
                    cache += money;
                    //利益/損失
                    long profit = (nextStart - positionSignal[i].buyValue) * (long)positionSignal[i].buyUnit;
                    if ( profit > money * 3 ) {
                        profit = money * 3; //極端な銘柄の排除
                    }
                    cache += profit;
                    if(profit>0 && yearProfit+profit >= 0){
                        zei = profit * 0.2;
                    }
                    if(profit<0 && yearProfit > 0){
                        zei = profit * 0.2;
                    }
                    yearProfit += profit;
                }
                cache -= zei;
                
                //スリッページ計算（発注時には計算せず、手仕舞い時のみ計算している）
                if(nextVolume > 1000 && positionSignal[i].buyUnit >= nextVolume){
                    slippage = (nextStart * positionSignal[i].buyUnit) / 40;
                }
                else if(nextVolume > 1000 && positionSignal[i].buyUnit >= nextVolume/2){
                    slippage = (nextStart * positionSignal[i].buyUnit) / 60;
                }
                else if(nextVolume > 1000 && positionSignal[i].buyUnit >= nextVolume/5){
                    slippage = (nextStart * positionSignal[i].buyUnit) / 80;
                }
                else if(positionSignal[i].buyUnit >= nextVolume/10){
                    slippage = (nextStart * positionSignal[i].buyUnit) / 100;
                }
                else if(positionSignal[i].buyUnit >= nextVolume/20){
                    slippage = (nextStart * positionSignal[i].buyUnit) / 150;
                }
                else if(positionSignal[i].buyUnit >= nextVolume/50){
                    slippage = (nextStart * positionSignal[i].buyUnit) / 200;
                }
                else if(positionSignal[i].buyUnit >= nextVolume/100){
                    slippage = (nextStart * positionSignal[i].buyUnit) / 400;
                }
                
                cache -= slippage;
                yearProfit -= slippage;
                
                positionSignal[i].sellDateIndex = dateIndex;
                positionSignal[i].sellValue = nextStart;
                positionSignal[i].slippage = (int)slippage;
                positionSignal[i].zei = zei;
                if(sellCalc > 0.0) {
                    positionSignal[i].sellReason = 1;
                } else if(lcCalc > 0.0 ) {
                    positionSignal[i].sellReason = 2;
                } else if(dateIndex >= signals->count-1){
                    positionSignal[i].sellReason = 3;
                } else {
                    positionSignal[i].sellReason = 0;
                }
                
                //過去の履歴に決済情報をコピー
                if(positionSignal[i].historyIndex < positionSignalListMax){
                    memcpy(&positionSignalList[positionSignal[i].historyIndex], &positionSignal[i], sizeof(strMsignal));
                }
                
                //現在のポジションから削除
                memcpy(&positionSignal[i], &positionSignal[positionCount-1], sizeof(strMsignal));
                positionCount--;
                
                //printf(" cache=%ld\n", cache);
            }
        }
        
        //年が変わると、年間利益額をリセットする
        if(dateIndex+1 < signals->count && data->date[dateIndex+1]>>16 > data->date[dateIndex]>>16)
        {
            yearProfit = 0;
        }
        
        //資産額計算のための履歴データ
        {
            cacheHistory[dateIndex] = cache;
            positionCountHistory[dateIndex] = positionCount;
            memcpy(positionSignalHistory[dateIndex], positionSignal, sizeof(positionSignal));
        }
    }
    
    //資産額の計算
    long stockHistory[signals->count];
    memset(stockHistory, 0x00, sizeof(stockHistory));
    {
        for(int dateIndex=0; dateIndex<endDateIndex; dateIndex++)
        {
            for(int i=0; i<positionCountHistory[dateIndex]; i++)
            {
                int unit = positionSignalHistory[dateIndex][i].buyUnit;
                int codeIndex = positionSignalHistory[dateIndex][i].codeIndex;
                
                int lastEnd = data->dailys[codeIndex].end[dateIndex];
                for(int j=1; lastEnd==0 && dateIndex-j > 0 && j<30; j++){
                    lastEnd = data->dailys[codeIndex].end[dateIndex-j];
                }
                long stock;
                if(isKarauri){
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
        
        for(int dateIndex=startDateIndex; dateIndex<endDateIndex; dateIndex++)
        {
            char* key = apr_palloc(xmap->pool, 32);
            char* value = apr_palloc(xmap->pool, 32);
            snprintf(key, 32-1, "date%d", dateIndex-startDateIndex);
            snprintf(value, 32-1, "%d", data->date[dateIndex]);
            xmap_set(xmap, key, value);
        }
        
        for(int dateIndex=startDateIndex; dateIndex<endDateIndex; dateIndex++)
        {
            char* key = apr_palloc(xmap->pool, 32);
            char* value = apr_palloc(xmap->pool, 32);
            snprintf(key, 32-1, "cache%d", dateIndex-startDateIndex);
            snprintf(value, 32-1, "%ld", cacheHistory[dateIndex]);
            xmap_set(xmap, key, value);
        }
        
        for(int dateIndex=startDateIndex; dateIndex<endDateIndex; dateIndex++)
        {
            char* key = apr_palloc(xmap->pool, 32);
            char* value = apr_palloc(xmap->pool, 32);
            snprintf(key, 32-1, "stock%d", dateIndex-startDateIndex);
            snprintf(value, 32-1, "%ld", stockHistory[dateIndex]);
            xmap_set(xmap, key, value);
        }
        
        for(int dateIndex=startDateIndex; dateIndex<endDateIndex; dateIndex++)
        {
            char* key = apr_palloc(xmap->pool, 32);
            char* value = apr_palloc(xmap->pool, 32);
            snprintf(key, 32-1, "positionCountHistory%d", dateIndex-startDateIndex);
            snprintf(value, 32-1, "%d", positionCountHistory[dateIndex]);
            xmap_set(xmap, key, value);
        }
        
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
        snprintf(value, 32-1, "%d", data->date[signals->count]);
        xmap_set(xmap, key, value);
        
        
        key = apr_palloc(xmap->pool, 32);
        value = apr_palloc(xmap->pool, 32);
        snprintf(key, 32-1, "signalCount");
        snprintf(value, 32-1, "%d", positionSignalListCounter);
        xmap_set(xmap, key, value);
        
        char strategyName[256] = "";
        for(int c=(int)strlen(path)-1; c>=0; c--)
        {
            if(path[c]=='/')
            {
                strcpy(strategyName, &path[c+1]);
                if(strategyName[strlen(strategyName)-1]=='/'){
                    strategyName[strlen(strategyName)-1]='\0';
                }
            }
        }
        
        key = apr_palloc(xmap->pool, 32);
        value = apr_palloc(xmap->pool, 128);
        snprintf(key, 32-1, "strategyName");
        snprintf(value, 128-1, "%s", strategyName);
        xmap_set(xmap, key, value);
        
        for(int positionSignalListIndex=0; positionSignalListIndex < positionSignalListCounter; positionSignalListIndex++)
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
        }
        
        xmap_set(xmap, "HoldDate", "0");
        
        char filePath[256];
        snprintf(filePath, sizeof(filePath)-1, "%s/allSignals.xmap", path);
        xmap_saveWithFilename(xmap, filePath);
        xmap_release(xmap);
    }
    
    //extraResult.xmapの保存
    {
        xmap_t* xmap = xmap_load(NULL);
        
        for(int dateIndex=startDateIndex; dateIndex<endDateIndex; dateIndex++)
        {
            char* key = apr_palloc(xmap->pool, 32);
            char* value = apr_palloc(xmap->pool, 32);
            snprintf(key, 32-1, "date%d", dateIndex-startDateIndex);
            snprintf(value, 32-1, "%d", data->date[dateIndex]);
            xmap_set(xmap, key, value);
        }
        
        for(int dateIndex=startDateIndex; dateIndex<endDateIndex; dateIndex++)
        {
            char* key = apr_palloc(xmap->pool, 32);
            char* value = apr_palloc(xmap->pool, 32);
            snprintf(key, 32-1, "signalCount%d", dateIndex-startDateIndex);
            snprintf(value, 32-1, "%d", signals->dailyCount[dateIndex]);
            xmap_set(xmap, key, value);
        }
        
        for(int dateIndex=startDateIndex; dateIndex<endDateIndex; dateIndex++)
        {
            char* key = apr_palloc(xmap->pool, 32);
            snprintf(key, 32-1, "codes%d", dateIndex-startDateIndex);
            char* value = apr_palloc(xmap->pool, 6*signals->dailyCount[dateIndex]);
            memset(value, 0x00, 6*signals->dailyCount[dateIndex]);
            
            for(int i=0; i<signals->dailyCount[dateIndex]; i++)
            {
                char codeStr[16];
                snprintf(codeStr, sizeof(codeStr)-1, "%d,", data->codes[signals->dailySignal[dateIndex][i].codeIndex]);
                strcat(value, codeStr);
            }
            xmap_set(xmap, key, value);
        }
        
        char filePath[256];
        snprintf(filePath, sizeof(filePath)-1, "%s/extraResult.xmap", path);
        xmap_saveWithFilename(xmap, filePath);
        xmap_release(xmap);
    }
    
    //extraSignals.xmapの保存
    {
        xmap_t* xmap = xmap_load(NULL);
        
        for(int dateIndex=startDateIndex; dateIndex<endDateIndex; dateIndex++)
        {
            if(signals->dailyCount[dateIndex]==0){
                continue;
            }
            
            char* key = apr_palloc(xmap->pool, 32);
            snprintf(key, 32-1, "codes%d", data->date[dateIndex]);
            char* value = apr_palloc(xmap->pool, 6*signals->dailyCount[dateIndex]);
            memset(value, 0x00, 6*signals->dailyCount[dateIndex]);
            
            for(int i=0; i<signals->dailyCount[dateIndex]; i++)
            {
                char codeStr[16];
                snprintf(codeStr, sizeof(codeStr)-1, "%d ", data->codes[signals->dailySignal[dateIndex][i].codeIndex]);
                strcat(value, codeStr);
            }
            xmap_set(xmap, key, value);
        }
        
        for(int dateIndex=startDateIndex; dateIndex<endDateIndex; dateIndex++)
        {
            char* key = apr_palloc(xmap->pool, 32);
            char* value = apr_palloc(xmap->pool, 32);
            snprintf(key, 32-1, "filter2%d", data->date[dateIndex]);
            snprintf(value, 32-1, "%s", filter2History[dateIndex]?"true":"false");
            xmap_set(xmap, key, value);
        }
        
        char filePath[256];
        snprintf(filePath, sizeof(filePath)-1, "%s/extraSignals.xmap", path);
        xmap_saveWithFilename(xmap, filePath);
        xmap_release(xmap);
    }
    
    //filter2signal.xmapの保存
    {
        xmap_t* xmap = xmap_load(NULL);
        
        for(int dateIndex=startDateIndex; dateIndex<endDateIndex; dateIndex++)
        {
            if(signals->dailyCount[dateIndex]==0){
                continue;
            }
            
            char* key = apr_palloc(xmap->pool, 32);
            char* value = apr_palloc(xmap->pool, 32);
            snprintf(key, 32-1, "signalCount%d", data->date[dateIndex]);
            snprintf(value, 32-1, "%d ", signals->dailyCount[dateIndex]);
            xmap_set(xmap, key, value);
        }
        
        for(int dateIndex=startDateIndex; dateIndex<endDateIndex; dateIndex++)
        {
            if(filter2History[dateIndex]){
            char* key = apr_palloc(xmap->pool, 32);
            char* value = apr_palloc(xmap->pool, 32);
                snprintf(key, 32-1, "filter2_%d", data->date[dateIndex]);
                snprintf(value, 32-1, "-");
                xmap_set(xmap, key, value);
            }
        }
        
        char filePath[256];
        snprintf(filePath, sizeof(filePath)-1, "%s/filter2signal.xmap", path);
        xmap_saveWithFilename(xmap, filePath);
        xmap_release(xmap);
    }
}

static int signal2filter(strMdata* data, strMsignalSum* signalSum, int dateIndex, const char** sig2Words, int sig2WordCount)
{
    if(sig2WordCount==0) return 1;

    int dummyCodeIndex = -1;
    
    return calc(sig2Words, 0, sig2WordCount-1, data, NULL, signalSum, dateIndex, dummyCodeIndex) > 0.0;
}


