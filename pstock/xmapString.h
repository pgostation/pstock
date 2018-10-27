//
//  xmapString.h
//  pstock
//
//  Created by takayoshi on 2016/01/23.
//  Copyright © 2016年 pgostation. All rights reserved.
//

#ifndef xmapString_h
#define xmapString_h

//#include <stdio.h>
#include <apr-1/apr.h>
#include <apr-1/apr_hash.h>

typedef struct xmapString
{
    char fileName[256];
    apr_pool_t* pool;
    apr_hash_t* hash;
} xmap_t;

extern xmap_t* xmap_load(char* fileName);
extern char *xmap_get(xmap_t* xmap, char* key);
extern void xmap_set(xmap_t* xmap, char* key, char* value);
extern void xmap_save(xmap_t* xmap);
extern void xmap_saveWithFilename(xmap_t* xmap, char* fileName);
extern void xmap_release(xmap_t* xmap);

#endif /* xmapString_h */
