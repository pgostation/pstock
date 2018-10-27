//
//  strMsignal.h
//  pstock
//
//  Created by takayoshi on 2016/01/24.
//  Copyright © 2016年 pgostation. All rights reserved.
//

#ifndef strMsignal_h
#define strMsignal_h

#include "strMdata.h"

typedef struct {
    int buyValue;
    int buyUnit;
    int codeIndex;
    int buyDateIndex;
    int sellDateIndex;
    int sellValue;
    int slippage;
    int zei;
    int historyIndex;
    unsigned char sellReason;
    char* strategy; //startegyL用
    unsigned char isKarauri; //startegyL用
    unsigned char big; //startegyL用
    unsigned char mid; //startegyL用
    unsigned char small; //startegyL用
} strMsignal;

typedef struct {
    int count;
    int *dailyCount;
    strMsignal** dailySignal;
} strMsignalSum;

extern strMsignalSum* strMsignal_start(const char *path, strMdata* data, int startDateIndex, int endDateIndex);

#endif /* strMsignal_h */
