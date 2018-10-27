//
//  strMbacktest.h
//  pstock
//
//  Created by takayoshi on 2016/01/24.
//  Copyright © 2016年 pgostation. All rights reserved.
//

#ifndef strMbacktest_h
#define strMbacktest_h

#include "strMdata.h"
#include "strMsignal.h"

extern void strMbacktest_start(const char* path, strMdata* data, strMsignalSum* signals, int startDateIndex, int endDateIndex);

#endif /* strMbacktest_h */
