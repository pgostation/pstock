//
//  xmapString.c
//  pstock
//
//  Created by takayoshi on 2016/01/23.
//  Copyright © 2016年 pgostation. All rights reserved.
//

#include <stdlib.h>
#include <apr-1/apr_file_io.h>
#include "xmapString.h"

static xmap_t* xmap_alloc()
{
    apr_pool_t* pool;
    apr_initialize();
    apr_pool_create(&pool, NULL);
    xmap_t* xmap = apr_palloc(pool, sizeof(xmap_t));
    xmap->pool = pool;
    xmap->hash = apr_hash_make(xmap->pool);
    xmap->fileName[0] = '\0';
    
    return xmap;
}

xmap_t* xmap_load(char* fileName)
{
    xmap_t* xmap = xmap_alloc();
    if(fileName==NULL) {
        return xmap;
    }
    
    strncpy(xmap->fileName, fileName, sizeof(xmap->fileName)-1);
    
    apr_status_t status;
    apr_file_t* file = NULL;
    status = apr_file_open(&file, xmap->fileName, APR_READ | APR_BUFFERED, APR_OS_DEFAULT, xmap->pool);
    if (status != APR_SUCCESS) {
        //printf("can't load xmap(%s)\n", fileName);
        return NULL;
    }
    
    apr_finfo_t finfo;
    apr_file_info_get(&finfo, APR_FINFO_SIZE, file);
    apr_off_t fileLen = finfo.size;
    
    char* buf = apr_palloc(xmap->pool, fileLen+1);
    apr_size_t len = fileLen;
    if (APR_SUCCESS != apr_file_read(file, buf, &len))
    {
        apr_file_close(file);
        return NULL;
    }
    apr_file_close(file);
    
    char* keyStart = &buf[0];
    char* valueStart = NULL;
    int escaped = 0;
    for(int i=0; i<=fileLen; i++)
    {
        if(i==fileLen || buf[i]=='\n')
        {
            buf[i]='\0';
            if(escaped){
                char* buf2 = apr_palloc(xmap->pool, strlen(valueStart)+1);
                int dmax = (int)strlen(valueStart);
                for(int c=0,d=0; c<16384 && d<dmax; c++,d++)
                {
                    if(valueStart[d]=='%'){
                        d++;
                        if(valueStart[d]=='n'){
                            valueStart[d] = '\n';
                        }
                    }
                    buf2[c] = valueStart[d];
                }
                xmap_set(xmap, keyStart, buf2);
                escaped = 0;
            } else {
                xmap_set(xmap, keyStart, valueStart);
            }
            keyStart = &buf[i+1];
        }
        else if(buf[i]=='%')
        {
            escaped = 1;
        }
        else if(buf[i]==':')
        {
            buf[i]='\0';
            valueStart = &buf[i+1];
        }
    }
    
    return xmap;
}

char *xmap_get(xmap_t* xmap, char* key)
{
    return apr_hash_get(xmap->hash, key, strlen(key));
}

void xmap_set(xmap_t* xmap, char* key, char* value)
{
    //printf("%s  %s\n", key, value);
    apr_hash_set(xmap->hash, key, APR_HASH_KEY_STRING, value);
}

void xmap_saveWithFilename(xmap_t* xmap, char* fileName)
{
    strncpy(xmap->fileName, fileName, sizeof(xmap->fileName)-1);
    xmap_save(xmap);
}

void xmap_save(xmap_t* xmap)
{
    apr_hash_index_t *hash_index;
    
    int maxLen = 1024 * apr_hash_count(xmap->hash) + 4096;
    char* buf = apr_palloc(xmap->pool, maxLen+1);
    memset(buf, 0x00, maxLen+1);
    apr_size_t len = 0;
    char tmpVal[4096];
    
    for (hash_index = apr_hash_first(NULL, xmap->hash); hash_index; hash_index = apr_hash_next(hash_index))
    {
        const char *key;
        const char *value;
        apr_hash_this(hash_index, (const void**)&key, NULL, (void**)&value);
        
        unsigned long keyLen = strlen(key);
        memcpy(&buf[len], key, keyLen);
        len += keyLen;
        buf[len] = ':';
        len += 1;
        
        unsigned long valueLen = strlen(value);
        int j=0;
        for(int i=0; i<valueLen && j<sizeof(tmpVal)-1; i++,j++)
        {
            if(value[i]=='\n')
            {
                tmpVal[j] = '%';
                j++;
                tmpVal[j] = 'n';
                //i++;
                //j++;
                continue;
            }
            else if(value[i]=='%')
            {
                tmpVal[j] = '%';
                j++;
            }
            tmpVal[j] = value[i];
        }
        tmpVal[j] = '\0';
        
        unsigned long tmpValLen = j;
        memcpy(&buf[len], tmpVal, tmpValLen);
        len += tmpValLen;
        buf[len] = '\n';
        buf[len+1] = '\0';
        len += 1;
    }
    
    apr_status_t status;
    apr_file_t* file = NULL;
    status = apr_file_open(&file, xmap->fileName, APR_FOPEN_CREATE | APR_FOPEN_TRUNCATE | APR_WRITE | APR_BUFFERED, APR_OS_DEFAULT, xmap->pool);
    if (status != APR_SUCCESS) {
        return;
    }
    apr_size_t write_len = len;
    write_len = apr_file_write(file, buf, &write_len);
    apr_file_close(file);
}

void xmap_release(xmap_t* xmap)
{
    if(xmap==NULL) return;
    apr_pool_t* pool = xmap->pool;
    xmap->hash = NULL;
    xmap->pool = NULL;
    xmap->fileName[0] = '\0';
    apr_pool_clear(pool);
}
