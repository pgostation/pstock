//
//  strMdataBinary.c
//  pstock
//
//  Created by takayoshi on 2016/01/26.
//  Copyright © 2016年 pgostation. All rights reserved.
//

#include <stdio.h>
#include <sys/stat.h>
#include <apr-1/apr_file_io.h>
#include <libiomp/omp.h>
#include "strMdataBinary.h"

extern void strMdataBinary_loadData(const char* path, strMdata* data)
{
    char filePathBinary[MAX_THREAD_COUNT][256];
    
    apr_pool_t* pool[MAX_THREAD_COUNT];
    apr_initialize();
    for(int i=0; i<MAX_THREAD_COUNT; i++)
    {
        apr_pool_create(&pool[i], NULL);
    }
    
    apr_status_t status;
    apr_file_t* file = NULL;
    apr_finfo_t finfo;
    apr_off_t fileLen;
    int* buf;
    int i;
    #pragma omp parallel for private(status,file,finfo,fileLen,buf,i)
    for(int codeIndex=0; codeIndex<data->count; codeIndex++)
    {
        snprintf(filePathBinary[omp_get_thread_num()], 256-1, "%s_dailydataBinary/%d", path, data->codes[codeIndex]);
        
        //ファイルを開いて、データをバッファに読み込む
        {
            status = apr_file_open(&file, filePathBinary[omp_get_thread_num()], APR_READ | APR_BUFFERED, APR_OS_DEFAULT, pool[omp_get_thread_num()]);
            if (status != APR_SUCCESS) {
                continue;
            }
            
            apr_file_info_get(&finfo, APR_FINFO_SIZE, file);
            fileLen = finfo.size;
            
            buf = apr_palloc(pool[omp_get_thread_num()], fileLen+1);
            apr_size_t len = fileLen;
            if (APR_SUCCESS != apr_file_read(file, buf, &len))
            {
                apr_file_close(file);
                continue;
            }
            apr_file_close(file);
        }
        
        data->dailys[codeIndex].count = (int)(fileLen/6/sizeof(int));
        
        //メモリ確保
        {
            //data->dailys[codeIndex].date = malloc(fileLen/6);
            data->dailys[codeIndex].start = malloc(fileLen/6);
            data->dailys[codeIndex].end = malloc(fileLen/6);
            data->dailys[codeIndex].max = malloc(fileLen/6);
            data->dailys[codeIndex].min = malloc(fileLen/6);
            data->dailys[codeIndex].volume = malloc(fileLen/6);
        }
        
        //配列に代入する
        {
            for(i=0; i<fileLen/sizeof(int); i+=6)
            {
                //data->dailys[codeIndex].date[i/6] = buf[i+0];
                data->dailys[codeIndex].start[i/6] = buf[i+1];
                data->dailys[codeIndex].end[i/6] = buf[i+2];
                data->dailys[codeIndex].max[i/6] = buf[i+3];
                data->dailys[codeIndex].min[i/6] = buf[i+4];
                data->dailys[codeIndex].volume[i/6] = buf[i+5];
            }
        }
        
        if(data->codes[codeIndex]==9984)
        {
            data->dailys[MAX_DATA_COUNT-1].count = (int)(fileLen/6/sizeof(int));
            
            //メモリ確保
            {
                data->date = malloc(fileLen/6);
                //data->dailys[MAX_DATA_COUNT-1].date = malloc(fileLen/6);
                data->dailys[MAX_DATA_COUNT-1].start = malloc(fileLen/6);
                data->dailys[MAX_DATA_COUNT-1].end = malloc(fileLen/6);
                data->dailys[MAX_DATA_COUNT-1].max = malloc(fileLen/6);
                data->dailys[MAX_DATA_COUNT-1].min = malloc(fileLen/6);
                data->dailys[MAX_DATA_COUNT-1].volume = malloc(fileLen/6);
            }
            
            //配列に代入する
            {
                for(i=0; i<fileLen/sizeof(int); i+=6)
                {
                    data->date[i/6] = buf[i+0];
                    data->dailys[MAX_DATA_COUNT-1].start[i/6] = buf[i+1];
                    data->dailys[MAX_DATA_COUNT-1].end[i/6] = buf[i+2];
                    data->dailys[MAX_DATA_COUNT-1].max[i/6] = buf[i+3];
                    data->dailys[MAX_DATA_COUNT-1].min[i/6] = buf[i+4];
                    data->dailys[MAX_DATA_COUNT-1].volume[i/6] = buf[i+5];
                }
            }
        }
        if(data->codes[codeIndex]==1321)
        {
            data->dailys[MAX_DATA_COUNT-2].count = (int)(fileLen/6/sizeof(int));
            
            //メモリ確保
            {
                //data->dailys[MAX_DATA_COUNT-2].date = malloc(fileLen/6);
                data->dailys[MAX_DATA_COUNT-2].start = malloc(fileLen/6);
                data->dailys[MAX_DATA_COUNT-2].end = malloc(fileLen/6);
                data->dailys[MAX_DATA_COUNT-2].max = malloc(fileLen/6);
                data->dailys[MAX_DATA_COUNT-2].min = malloc(fileLen/6);
                data->dailys[MAX_DATA_COUNT-2].volume = malloc(fileLen/6);
            }
            
            //配列に代入する
            {
                for(i=0; i<fileLen/sizeof(int); i+=6)
                {
                    //data->dailys[MAX_DATA_COUNT-2].date[i/6] = buf[i+0];
                    data->dailys[MAX_DATA_COUNT-2].start[i/6] = buf[i+1];
                    data->dailys[MAX_DATA_COUNT-2].end[i/6] = buf[i+2];
                    data->dailys[MAX_DATA_COUNT-2].max[i/6] = buf[i+3];
                    data->dailys[MAX_DATA_COUNT-2].min[i/6] = buf[i+4];
                    data->dailys[MAX_DATA_COUNT-2].volume[i/6] = buf[i+5];
                }
            }
        }
    }
    
    for(int i=0; i<MAX_THREAD_COUNT; i++)
    {
        apr_pool_destroy(pool[i]);
    }
}

extern void strMdataBinary_saveData(const char* path, strMdata* data)
{
    char filePathBinary[MAX_THREAD_COUNT][256];
    
    //ディレクトリ作る
    snprintf(filePathBinary[0], 256-1, "%s_dailydataBinary", path);
    mkdir(filePathBinary[0], 0777);
    
    apr_pool_t* pool[MAX_THREAD_COUNT];
    apr_initialize();
    for(int i=0; i<MAX_THREAD_COUNT; i++)
    {
        apr_pool_create(&pool[i], NULL);
    }
    
    apr_size_t len;
    int* buf;
    int count_max;
    int i;
    apr_status_t status;
    apr_file_t* file;
    #pragma omp parallel for private(len,buf,count_max,i,status,file)
    for(int codeIndex=0; codeIndex<data->count; codeIndex++)
    {
        if(data->codes[codeIndex]==0){
            continue;
        }
        snprintf(filePathBinary[omp_get_thread_num()], 256-1, "%s_dailydataBinary/%d", path, data->codes[codeIndex]);
        
        len = sizeof(int) * 6 * data->dailys[codeIndex].count;
        buf = malloc(len);
        
        //データ用のバッファにフォーマット
        {
            count_max = 6 * data->dailys[codeIndex].count;
            for(i=0; i<count_max; i+=6)
            {
                buf[i+0] = data->date[i/6];
                buf[i+1] = data->dailys[codeIndex].start[i/6];
                buf[i+2] = data->dailys[codeIndex].end[i/6];
                buf[i+3] = data->dailys[codeIndex].max[i/6];
                buf[i+4] = data->dailys[codeIndex].min[i/6];
                buf[i+5] = data->dailys[codeIndex].volume[i/6];
            }
        }
        
        //ファイルに書き込み
        {
            status = apr_file_open(&file, filePathBinary[omp_get_thread_num()], APR_FOPEN_CREATE | APR_WRITE | APR_BUFFERED, APR_OS_DEFAULT, pool[omp_get_thread_num()]);
            if (status != APR_SUCCESS) {
                continue;
            }
            apr_file_write(file, buf, &len);
            apr_file_close(file);
        }
        
        free(buf);
    }
}