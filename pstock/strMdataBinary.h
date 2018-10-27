//
//  strMdataBinary.h
//  pstock
//
//  Created by takayoshi on 2016/01/26.
//  Copyright © 2016年 pgostation. All rights reserved.
//

#ifndef strMdataBinary_h
#define strMdataBinary_h

#include "strMdata.h"

extern void strMdataBinary_loadData(const char* path, strMdata* data);
extern void strMdataBinary_saveData(const char* path, strMdata* data);

#endif /* strMdataBinary_h */
