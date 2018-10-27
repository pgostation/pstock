//
//  calc.h
//  pstock
//
//  Created by takayoshi on 2016/01/25.
//  Copyright © 2016年 pgostation. All rights reserved.
//

#ifndef calc_h
#define calc_h

#include "strMdata.h"
#include "strMsignal.h"

extern float calc(const char** words, int start, int end, strMdata* data, strMsignal* signalData, strMsignalSum* signalSum, int dateIndex, int codeIndex);

#endif /* calc_h */
