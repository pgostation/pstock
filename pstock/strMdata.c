//
//  strMdata.c
//  pstock
//
//  Created by takayoshi on 2016/01/24.
//  Copyright © 2016年 pgostation. All rights reserved.
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <libiomp/omp.h>
#include "main.h"
#include "strMdata.h"
#include "strMdataBinary.h"
#include "xmapString.h"

strMdata* strMdata_initData(const char *path)
{
    //スレッド数チェック
    #pragma omp parallel
    {
        if(omp_get_thread_num()==1)
        {
            printf("omp_get_num_threads:%d\n", omp_get_num_threads());
            if(omp_get_num_threads()>MAX_THREAD_COUNT)
            {
                printf("ERROR:omp_get_num_threads(%d) > MAX_THREAD_COUNT(%d)\n", omp_get_num_threads(), MAX_THREAD_COUNT);
            }
        }
    }
    
    strMdata* data = malloc(sizeof(strMdata));
    
    //メモリ確保
    {
        data->codes = malloc(MAX_DATA_COUNT * sizeof(int));
        data->dailys = malloc(MAX_DATA_COUNT * sizeof(strMdaily));
        data->companys = malloc(MAX_DATA_COUNT * sizeof(strMcompany));
        data->kessans = malloc(MAX_DATA_COUNT * sizeof(strMkessan));
    }
    
    char dailyDir[256];
    strncpy(dailyDir, path, sizeof(dailyDir)-1);
    strncat(dailyDir, "_dailydataXmap/", sizeof(dailyDir)-1);
    
    //ディレクトリを走査して企業コードを入れる
    {
        FILE* file;
        if ((file = fopen(dailyDir, "r")) == NULL)
        {
            printf("cantopen1\n");
        }
        
        DIR* dir = opendir(dailyDir);
        struct dirent *dp;
        int i = 0;
        for(dp=readdir(dir); dp!=NULL; dp=readdir(dir))
        {
            sscanf(dp->d_name, "%d.xmap", &data->codes[i]);
            i++;
        }
        data->count = i;
        closedir(dir);
    }
    
    return data;
}
    
void strMdata_loadData(const char *path, strMdata* data)
{
    char dailyDir[256];
    strncpy(dailyDir, path, sizeof(dailyDir)-1);
    strncat(dailyDir, "_dailydataXmap/", sizeof(dailyDir)-1);
    
    //バイナリファイルの方が新しければ、そちらを使う
    {
        char filePath9984[256];
        snprintf(filePath9984, sizeof(filePath9984)-1, "%s/%d.xmap", dailyDir, 9984);
        struct stat statXmap;
        stat(filePath9984, &statXmap);
        
        char filePathBinary9984[256];
        snprintf(filePathBinary9984, sizeof(filePathBinary9984)-1, "%s_dailydataBinary/%d", path, 9984);
        struct stat statBinary;
        if(0==stat(filePathBinary9984, &statBinary))
        {
            if(statXmap.st_mtime < statBinary.st_mtime)
            {
                strMdataBinary_loadData(path, data);
                return;
            }
        }
    }
    
    //基準株価データを取得する
    int dateMax = 0;
    {
        char filePath[256];
        snprintf(filePath, sizeof(filePath)-1, "%s/%d.xmap", dailyDir, 9984);
        xmap_t* xmap = xmap_load(filePath);
        
        data->date = malloc(4000 * sizeof(int));
        data->dailys[MAX_DATA_COUNT-1].count = 4000;
        //data->dailys[MAX_DATA_COUNT-1].date = malloc(4000 * sizeof(int));
        data->dailys[MAX_DATA_COUNT-1].start = malloc(4000 * sizeof(int));
        data->dailys[MAX_DATA_COUNT-1].end = malloc(4000 * sizeof(int));
        data->dailys[MAX_DATA_COUNT-1].max = malloc(4000 * sizeof(int));
        data->dailys[MAX_DATA_COUNT-1].min = malloc(4000 * sizeof(int));
        data->dailys[MAX_DATA_COUNT-1].volume = malloc(4000 * sizeof(int));
        
        for(int i=0; i<4000; i++)
        {
            char key[16];
            char* value;
            int intValue;
            
            snprintf(key, sizeof(key)-1, "date%d", i);
            value = xmap_get(xmap, key);
            if(value==NULL){
                data->dailys[MAX_DATA_COUNT-1].count = i-1;
                dateMax = i-1;
                break;
            }
            sscanf(value, "%d", &intValue);
            data->date[i] = intValue;
        }
        
        xmap_release(xmap);
    }
    {
        data->dailys[MAX_DATA_COUNT-2].count = 4000;
        //data->dailys[MAX_DATA_COUNT-2].date = malloc(4000 * sizeof(int));
        data->dailys[MAX_DATA_COUNT-2].start = malloc(4000 * sizeof(int));
        data->dailys[MAX_DATA_COUNT-2].end = malloc(4000 * sizeof(int));
        data->dailys[MAX_DATA_COUNT-2].max = malloc(4000 * sizeof(int));
        data->dailys[MAX_DATA_COUNT-2].min = malloc(4000 * sizeof(int));
        data->dailys[MAX_DATA_COUNT-2].volume = malloc(4000 * sizeof(int));
    }
    
    //株価データを取得する
    char buf[MAX_THREAD_COUNT][256];
    char* filePath;
    xmap_t* xmap;
    char key[24];
    char* value;
    int intValue;
    #pragma omp parallel for private(filePath,xmap,key,value,intValue)
    for(int codeIndex=0; codeIndex<data->count; codeIndex++)
    {
        filePath = buf[omp_get_thread_num()];
        snprintf(filePath, 256-1, "%s/%d.xmap", dailyDir, data->codes[codeIndex]);
        xmap = xmap_load(filePath);
        if(xmap==NULL){
            continue;
        }
        
        data->dailys[codeIndex].count = dateMax+1;
        //data->dailys[codeIndex].date = malloc((dateMax+1) * sizeof(int));
        data->dailys[codeIndex].start = malloc((dateMax+1) * sizeof(int));
        data->dailys[codeIndex].end = malloc((dateMax+1) * sizeof(int));
        data->dailys[codeIndex].max = malloc((dateMax+1) * sizeof(int));
        data->dailys[codeIndex].min = malloc((dateMax+1) * sizeof(int));
        data->dailys[codeIndex].volume = malloc((dateMax+1) * sizeof(int));
        
        int offset = 0;
        for(int i=0; i<dateMax+1; i++)
        {
            snprintf(key, sizeof(key)-1, "date%d", i-offset);
            value = xmap_get(xmap, key);
            if(value==NULL){
                break;
            }
            sscanf(value, "%d", &intValue);
            while(intValue > data->date[i])
            {
                //data->dailys[codeIndex].date[i] = data->dailys[MAX_DATA_COUNT-1].date[i];
                data->dailys[codeIndex].start[i] = 0;
                data->dailys[codeIndex].end[i] = 0;
                data->dailys[codeIndex].max[i] = 0;
                data->dailys[codeIndex].min[i] = 0;
                data->dailys[codeIndex].volume[i] = 0;
                i++;
                offset++;
            }
            if(intValue < data->date[i]){
                printf("strMdata_loadData date error intValue=%d %d\n",intValue,data->date[i]);
            }
            //data->dailys[codeIndex].date[i] = intValue;
            
            snprintf(key, sizeof(key)-1, "start%d", i-offset);
            value = xmap_get(xmap, key);
            sscanf(value, "%d", &intValue);
            data->dailys[codeIndex].start[i] = intValue;
            
            snprintf(key, sizeof(key)-1, "end%d", i-offset);
            value = xmap_get(xmap, key);
            sscanf(value, "%d", &intValue);
            data->dailys[codeIndex].end[i] = intValue;
            
            snprintf(key, sizeof(key)-1, "min%d", i-offset);
            value = xmap_get(xmap, key);
            sscanf(value, "%d", &intValue);
            data->dailys[codeIndex].min[i] = intValue;
            
            snprintf(key, sizeof(key)-1, "max%d", i-offset);
            value = xmap_get(xmap, key);
            sscanf(value, "%d", &intValue);
            data->dailys[codeIndex].max[i] = intValue;
            
            snprintf(key, sizeof(key)-1, "volume%d", i-offset);
            value = xmap_get(xmap, key);
            sscanf(value, "%d", &intValue);
            data->dailys[codeIndex].volume[i] = intValue;
        }
        
        xmap_release(xmap);
    }
    
    //データがおかしくないか確認する
    for(int codeIndex=0; codeIndex<4000; codeIndex++)
    {
        if( data->codes[codeIndex] == 3370 )
        {
            for(int i=1; i<dateMax; i++)
            {
                if(data->dailys[codeIndex].end[i]==0 || data->dailys[codeIndex].end[i-1]==0){
                    continue;
                }
                double rate = data->dailys[codeIndex].end[i] / (double)data->dailys[codeIndex].end[i-1];
                if (rate < 0.6 || rate > 1.4 )
                {
                    printf("data broken. rate=%lf\n",rate);
                    system("pkill pstock");
                    while(1){
                    }
                    //exit(-1);
                }
            }
        }
    }
    
    //バイナリ形式で保存
    strMdataBinary_saveData(path, data);
    
    for(int codeIndex=0; codeIndex<data->count; codeIndex++)
    {
        free(data->dailys[codeIndex].start);
        free(data->dailys[codeIndex].end);
        free(data->dailys[codeIndex].max);
        free(data->dailys[codeIndex].min);
        free(data->dailys[codeIndex].volume);
    }
    
    strMdataBinary_loadData(path, data);
}

void strMdata_loadKessan(const char *path, strMdata* data)
{
    char companyDir[256];
    strncpy(companyDir, path, sizeof(companyDir)-1);
    strncat(companyDir, "_companydataXmap/", sizeof(companyDir)-1);
    
    char buf[MAX_THREAD_COUNT][256];
    char* filePath;
    xmap_t* xmap;
    char key[24];
    char* value;
    int intValue;
    
    #pragma omp parallel for private(filePath,xmap,key,value,intValue)
    for(int codeIndex=0; codeIndex<data->count; codeIndex++)
    {
        filePath = buf[omp_get_thread_num()];
        snprintf(filePath, 256-1, "%s/%d.xmap", companyDir, data->codes[codeIndex]);
        xmap = xmap_load(filePath);
        if(xmap==NULL) {
            continue;
        }
    
        snprintf(key, sizeof(key)-1, "unit");
        value = xmap_get(xmap, key);
        sscanf(value, "%d", &intValue);
        data->companys[codeIndex].unit = intValue;
    
        snprintf(key, sizeof(key)-1, "shijou");
        value = xmap_get(xmap, key);
        if(value==NULL){
            data->companys[codeIndex].shijou = 0;
        }
        else if(strcmp(value,"市場第一部（内国株）")==0){
            data->companys[codeIndex].shijou = 1;
        }
        else if(strcmp(value,"市場第二部（内国株）")==0){
            data->companys[codeIndex].shijou = 2;
        }
        else {
            data->companys[codeIndex].shijou = 3;
        }
    
        snprintf(key, sizeof(key)-1, "taishaku");
        value = xmap_get(xmap, key);
        if(value==NULL || strcmp(value,"貸借銘柄")!=0){
            data->companys[codeIndex].shijou = 0;
        }
        else {
            data->companys[codeIndex].isKarauriOk = 1;
        }
        
        xmap_release(xmap);
    }
    
    char kessanDir[256];
    strncpy(kessanDir, path, sizeof(kessanDir)-1);
    strncat(kessanDir, "_kessandata/", sizeof(kessanDir)-1);
    
    int dateMax = data->dailys[MAX_DATA_COUNT-1].count;
    
    int i1,i2,j;
    long longValue;
    float floatValue;
    long asset,netasset,pureProfit,keijouProfit,sales,eigyouCf;
    float eps,bps;
    int date,hasRenketsu;
    #pragma omp parallel for private(filePath,xmap,i1,i2,j,key,value,longValue,floatValue,asset,netasset,pureProfit,keijouProfit,sales,eigyouCf,eps,bps,date,hasRenketsu)
    for(int codeIndex=0; codeIndex<data->count; codeIndex++)
    {
        filePath = buf[omp_get_thread_num()];
        snprintf(filePath, 256-1, "%s/%d.xmap", kessanDir, data->codes[codeIndex]);
        xmap = xmap_load(filePath);
        if(xmap==NULL){
            continue;
        }
        
        data->kessans[codeIndex].count = dateMax+1;
        //data->kessans[codeIndex].date = malloc((dateMax+1) * sizeof(int));
        data->kessans[codeIndex].asset = malloc((dateMax+1) * sizeof(float));
        data->kessans[codeIndex].netasset = malloc((dateMax+1) * sizeof(float));
        data->kessans[codeIndex].pureProfit = malloc((dateMax+1) * sizeof(float));
        data->kessans[codeIndex].keijouProfit = malloc((dateMax+1) * sizeof(float));
        data->kessans[codeIndex].sales = malloc((dateMax+1) * sizeof(float));
        data->kessans[codeIndex].eigyouCf = malloc((dateMax+1) * sizeof(float));
        data->kessans[codeIndex].eps = malloc((dateMax+1) * sizeof(float));
        data->kessans[codeIndex].bps = malloc((dateMax+1) * sizeof(float));
        
        asset = 0;
        netasset = 0;
        pureProfit = 0;
        keijouProfit = 0;
        sales = 0;
        eigyouCf = 0;
        eps = -999999;
        bps = -999999;
    
        j=0;
        date=0;
        hasRenketsu = 0;
        
        for(i1=0; i1<999; i1++)
        {
            snprintf(key, sizeof(key)-1, "renketsuKobetsu%d", i1);
            value = xmap_get(xmap, key);
            if(value==NULL){
                break;
            }
            if(0==strcmp(value, "連結"))
            {
                hasRenketsu = 1;
                break;
            }
        }
        
        for(i2=0; i2<999; i2++)
        {
            snprintf(key, sizeof(key)-1, "date%d", i2);
            value = xmap_get(xmap, key);
            if(value==NULL){
                if(j>=dateMax+1){
                    break;
                }
                date += 0x00010000;
            } else {
                sscanf(value, "%d", &date);
            }
            
            for(; data->date[j]<date && j<dateMax+1; j++)
            {
                data->kessans[codeIndex].asset[j] = asset;
                data->kessans[codeIndex].netasset[j] = netasset;
                data->kessans[codeIndex].pureProfit[j] = pureProfit;
                data->kessans[codeIndex].keijouProfit[j] = keijouProfit;
                data->kessans[codeIndex].sales[j] = sales;
                data->kessans[codeIndex].eigyouCf[j] = eigyouCf;
                data->kessans[codeIndex].eps[j] = eps;
                data->kessans[codeIndex].bps[j] = bps;
            }
            
            if(value==NULL){
                date += 0x00010000;
                asset = 0;
                netasset = 0;
                pureProfit = 0;
                keijouProfit = 0;
                eigyouCf = 0;
                eps = -999999;
                bps = -999999;
                continue;
            }
            
            if(hasRenketsu)
            {
                snprintf(key, sizeof(key)-1, "renketsuKobetsu%d", i2);
                value = xmap_get(xmap, key);
                if(0!=strcmp(value, "連結")){
                    continue;
                }
            }
            
            snprintf(key, sizeof(key)-1, "bps%d", i2);
            value = xmap_get(xmap, key);
            sscanf(value, "%f", &floatValue);
            if(floatValue==0.0){
                continue;
            }
            bps = floatValue;
            
            snprintf(key, sizeof(key)-1, "junShisan%d", i2);
            value = xmap_get(xmap, key);
            sscanf(value, "%ld", &longValue);
            asset = longValue;
            
            snprintf(key, sizeof(key)-1, "souShisan%d", i2);
            value = xmap_get(xmap, key);
            sscanf(value, "%ld", &longValue);
            netasset = longValue;
            
            snprintf(key, sizeof(key)-1, "junRieki%d", i2);
            value = xmap_get(xmap, key);
            sscanf(value, "%ld", &longValue);
            pureProfit = longValue;
            
            snprintf(key, sizeof(key)-1, "keijouRieki%d", i2);
            value = xmap_get(xmap, key);
            sscanf(value, "%ld", &longValue);
            keijouProfit = longValue;
            
            snprintf(key, sizeof(key)-1, "uriage%d", i2);
            value = xmap_get(xmap, key);
            sscanf(value, "%ld", &longValue);
            sales = longValue;
            
            snprintf(key, sizeof(key)-1, "eigyouCacheFlow%d", i2);
            value = xmap_get(xmap, key);
            sscanf(value, "%ld", &longValue);
            eigyouCf = longValue;
            
            snprintf(key, sizeof(key)-1, "eps%d", i2);
            value = xmap_get(xmap, key);
            sscanf(value, "%f", &floatValue);
            eps = floatValue;
        }
        
        xmap_release(xmap);
    }
}