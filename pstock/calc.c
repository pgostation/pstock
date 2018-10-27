//
//  calc.c
//  pstock
//
//  Created by takayoshi on 2016/01/25.
//  Copyright © 2016年 pgostation. All rights reserved.
//

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "strMsignal.h"
#include "calc.h"

#define ABS(a) (((a) < 0.0f) ? -(a) : (a))
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define MIN(a, b) ((a) < (b) ? (a) : (b))

#define MALLOC(a) (void*)my_malloc((a))
#define FREE(a) my_free((a))

#define XFALSE 0
#define XTRUE 0

static float calc1(char** words, int start, int end, strMdata* data, strMsignal* signalData, strMsignalSum* signalSum, int dateIndex, int codeIndex, float* bracketData, int* bracketIndex);
static float calc_value(const char* str, strMdata* data, strMsignal* signalData, strMsignalSum* signalSum, int dateIndex, int codeIndex);
static float this_value(const char* str, strMdata* data, int offset, strMsignal* signalData, strMsignalSum* signalSum, int dateIndex, int codeIndex, int boolx);
static char** clone(const char** words, int start, int end);
static void cloneFree(char **words, int start, int end);
static void* my_malloc(size_t size);
static void my_free(void *ptr);
static const char* substring(const char* str, int start);
static int indexOf(const char* str, char c);

float calc(const char** in_words, int start, int end, strMdata* data, strMsignal* signalData, strMsignalSum* signalSum, int dateIndex, int codeIndex)
{
    if(end-start < 0){
        return 0.0;
    }

    if(end-start <= 1){
        return calc_value(in_words[start], data, signalData, signalSum, dateIndex, codeIndex);
    }
    
    if(codeIndex==0){
        //printf("dateIndex:%d\n",dateIndex);
    }
    
    char** words = clone(in_words, start, end);
    
    float bracketValue[256];
    int bracketEnd[256];
    
    // ( ) 内を計算し、配列に持っておく
    int isBracket = 0;
    for(int i=end; i>=start; i--){
        if(strcmp(words[i],"(")==0){
            if(!isBracket){
                isBracket = 1;
            }
            int nest=0;
            for(int j=i+1; j<=end; j++)
            {
                if(words[j]==NULL){
                    continue;
                }
                if(strcmp(words[j],"(")==0){
                    nest++;
                }
                if(strcmp(words[j],")")==0){
                    if(nest==0){
                        float ret1 = calc1(words, i+1, j-1, data, signalData, signalSum, dateIndex, codeIndex, bracketValue, bracketEnd);
                        bracketValue[i] = ret1;
                        bracketEnd[i] = j;
                        for(int k=i+1; k<=j-1; k++){
                            //FREE(words[k]);
                            words[k] = NULL;
                        }
                        break;
                    }else{
                        nest--;
                    }
                }
            }
        }
    }

    float value = calc1(words, start, end, data, signalData, signalSum, dateIndex, codeIndex, bracketValue, bracketEnd);
    
    cloneFree(words, start, end);
    
    return value;
}

static float calc1(char** words, int start, int end, strMdata* data, strMsignal* signalData, strMsignalSum* signalSum, int dateIndex, int codeIndex, float* bracketValue, int* bracketEnd)
{
    if(end<start) return 0;

    if(strcmp(words[start],"(")==0 && strcmp(words[end],")")==0){
        if(bracketEnd[start]==end){
            return bracketValue[start];
        }
    }
    if(strcmp(words[start],"(")==0){
    }
    if( (strcmp(words[start],"max[")==0 || strcmp(words[start],"min[")==0 || strcmp(words[start],"abs[")==0 )
        && strcmp(words[end],"]")==0){
        if(bracketEnd[start]==end){
            return bracketValue[start];
        }
    }
    
    for(int i=end; i>=start+1; i--){
        if(strcmp(words[i],"")==0){
            end--;
        }
        else break;
    }
    
    if(end>start+1)
    {
        // max[],min[],abs[]関数
        for(int i=start; i<=end-1; i++){
            if(bracketEnd[i]==0 && words[i]!=NULL && (strcmp(words[i],"max[")==0 || strcmp(words[i],"min[")==0 || strcmp(words[i],"abs[")==0)){
                float retAry[99];
                int count = 0;
                int i2 = i;
                for(int j=i+1; j<=end; j++){
                    if(strcmp(words[j],",")==0){
                        retAry[count] = calc1(words, i2+1, j-1, data, signalData, signalSum, dateIndex, codeIndex, bracketValue, bracketEnd);
                        count++;
                        i2 = j;
                    }
                    if(strcmp(words[j],"]")==0){
                        retAry[count] = calc1(words, i2+1, j-1, data, signalData, signalSum, dateIndex, codeIndex, bracketValue, bracketEnd);
                        count++;
                        float ret = 0;
                        if(strcmp(words[i],"max[")==0){
                            ret = -1000000000000.0;
                            for(int k=0; k<count; k++){
                                ret = MAX(ret,retAry[k]);
                            }
                        }
                        else if(strcmp(words[i],"min[")==0){
                            ret = 1000000000000.0;
                            for(int k=0; k<count; k++){
                                ret = MIN(ret,retAry[k]);
                            }
                        }
                        else if(strcmp(words[i],"abs[")==0){
                            ret = ABS(retAry[0]);
                        }
                        if(i==start && j==end){
                            return ret;
                        }
                        bracketValue[i] = ret;
                        bracketEnd[i] = j;
                        for(int k=i+1; k<=j-1; k++){
                            strcpy(words[k],"");
                        }
                        break;
                    }
                }
            }
        }
        
        // ||で分割
        for(int i=start+1; i<=end-1; i++){
            if(words[i]!=NULL && strcmp(words[i],"||")==0){
                float ret1 = calc1(words, start, i-1, data, signalData, signalSum, dateIndex, codeIndex, bracketValue, bracketEnd);
                if(ret1>0){
                    return 1;
                }
                float ret2 = calc1(words, i+1, end, data, signalData, signalSum, dateIndex, codeIndex, bracketValue, bracketEnd);
                return (ret1>0 || ret2>0)?1:0;
            }
        }
        
        // &&で分割
        for(int i=start+1; i<=end-1; i++){
            if(words[i]!=NULL && strcmp(words[i],"&&")==0){
                float ret1 = calc1(words, start, i-1, data, signalData, signalSum, dateIndex, codeIndex, bracketValue, bracketEnd);
                if(!(ret1>0)){
                    return 0;
                }
                float ret2 = calc1(words, i+1, end, data, signalData, signalSum, dateIndex, codeIndex, bracketValue, bracketEnd);
                return (ret1>0 && ret2>0)?1:0;
            }
        }
        
        // 比較演算子で分割
        for(int i=start+1; i<=end-1; i++){
            if(words[i]==NULL) continue;
            if(strcmp(words[i],">=")==0){
                float ret1 = calc1(words, start, i-1, data, signalData, signalSum, dateIndex, codeIndex, bracketValue, bracketEnd);
                float ret2 = calc1(words, i+1, end, data, signalData, signalSum, dateIndex, codeIndex, bracketValue, bracketEnd);
                return (ret1 >= ret2)?1:0;
            }
            else if(strcmp(words[i],"<=")==0){
                float ret1 = calc1(words, start, i-1, data, signalData, signalSum, dateIndex, codeIndex, bracketValue, bracketEnd);
                float ret2 = calc1(words, i+1, end, data, signalData, signalSum, dateIndex, codeIndex, bracketValue, bracketEnd);
                return (ret1 <= ret2)?1:0;
            }
            else if(strcmp(words[i],">")==0){
                float ret1 = calc1(words, start, i-1, data, signalData, signalSum, dateIndex, codeIndex, bracketValue, bracketEnd);
                float ret2 = calc1(words, i+1, end, data, signalData, signalSum, dateIndex, codeIndex, bracketValue, bracketEnd);
                return (ret1 > ret2)?1:0;
            }
            else if(strcmp(words[i],"<")==0){
                float ret1 = calc1(words, start, i-1, data, signalData, signalSum, dateIndex, codeIndex, bracketValue, bracketEnd);
                float ret2 = calc1(words, i+1, end, data, signalData, signalSum, dateIndex, codeIndex, bracketValue, bracketEnd);
                return (ret1 < ret2)?1:0;
            }
            else if(strcmp(words[i],"==")==0 || strcmp(words[i],"=")==0){
                float ret1 = calc1(words, start, i-1, data, signalData, signalSum, dateIndex, codeIndex, bracketValue, bracketEnd);
                float ret2 = calc1(words, i+1, end, data, signalData, signalSum, dateIndex, codeIndex, bracketValue, bracketEnd);
                return (ret1 == ret2)?1:0;
            }
            else if(strcmp(words[i],"!=")==0 || strcmp(words[i],"<>")==0){
                float ret1 = calc1(words, start, i-1, data, signalData, signalSum, dateIndex, codeIndex, bracketValue, bracketEnd);
                float ret2 = calc1(words, i+1, end, data, signalData, signalSum, dateIndex, codeIndex, bracketValue, bracketEnd);
                return (ret1 != ret2)?1:0;
            }
        }
        
        // 加算、減算で分割
        for(int i=end-1; i>=start+1; i--){
            if(words[i]==NULL) continue;
            if(strcmp(words[i],"+")==0){
                float ret1 = calc1(words, start, i-1, data, signalData, signalSum, dateIndex, codeIndex, bracketValue, bracketEnd);
                float ret2 = calc1(words, i+1, end, data, signalData, signalSum, dateIndex, codeIndex, bracketValue, bracketEnd);
                return (ret1 + ret2);
            }
            else if(strcmp(words[i],"-")==0){
                float ret1 = calc1(words, start, i-1, data, signalData, signalSum, dateIndex, codeIndex, bracketValue, bracketEnd);
                float ret2 = calc1(words, i+1, end, data, signalData, signalSum, dateIndex, codeIndex, bracketValue, bracketEnd);
                return (ret1 - ret2);
            }
        }
        
        // 乗算で分割
        for(int i=start+1; i<=end-1; i++){
            if(words[i]==NULL) continue;
            if(strcmp(words[i],"*")==0){
                float ret1 = calc1(words, start, i-1, data, signalData, signalSum, dateIndex, codeIndex, bracketValue, bracketEnd);
                float ret2 = calc1(words, i+1, end, data, signalData, signalSum, dateIndex, codeIndex, bracketValue, bracketEnd);
                return (ret1 * ret2);
            }
            else if(strcmp(words[i],"/")==0){
                float ret1 = calc1(words, start, i-1, data, signalData, signalSum, dateIndex, codeIndex, bracketValue, bracketEnd);
                float ret2 = calc1(words, i+1, end, data, signalData, signalSum, dateIndex, codeIndex, bracketValue, bracketEnd);
                if(ret2==0) {
                    return ret1;
                }
                return (ret1 / ret2);
            }
        }
        
        if(strstr(words[start],"\"")!=NULL && strstr(words[start+1],"\"")!=NULL){
            //途中で分割されちゃってる
            char tmpWord[512] = "";
            snprintf(tmpWord, sizeof(tmpWord)-1, "%s %s", words[start], words[start+1]);
            return calc((const char**)&tmpWord, 0, 0, data, signalData, signalSum, dateIndex, codeIndex);
        }

        char errStr[512] = "";
        for(int i=start; i<=end; i++){
            if(words[i]==NULL) continue;
            strcat(errStr,words[i]);
            strcat(errStr," ");
        }
        printf("Script Error:%s\n",errStr);
    }
    
    // 負数
    if(words[start][0]=='-'){
        char str[256] = "";
        strcpy(str, &words[start][1]);
        float result = calc_value(str, data, signalData, signalSum, dateIndex, codeIndex);
        return -1.0*result;
    }
    
    // 関数、直値
    float result = calc_value(words[start], data, signalData, signalSum, dateIndex, codeIndex);
    return result;
}

static float calc_value(const char* str, strMdata* data, strMsignal* signalData, strMsignalSum* signalSum, int dateIndex, int codeIndex)
{
    if(strncmp(str,"this.",5)==0)
    {
        /*if(codeIndex==-1 && !strncmp(str,"this.signal",11)==0){
            printf("Don't use 'this' in Filter Script:%s\n",str);
            return 0.0;
        }*/
        const char* str2 = &str[5];
        
        return this_value(str2, data, 0, signalData, signalSum, dateIndex, codeIndex, XFALSE);
    }
    
    //if(str.matches("[0-9]+") || str.matches("[0-9]+\\.[0-9]+"))
    if(str[0]<='9' && str[0]>='0')
    {
        float value;
        sscanf(str, "%f", &value);
        return value;
    }
    
    if(strncmp(str,"this(",5)==0)
    {
        /*if(codeIndex==-1){
            printf("Don't use 'this' in Filter Script:%s\n",str);
            return 0.0;
        }*/
        //char* daystr = str.substring(5,str.indexOf(")"));
        const char* str2 = substring(str,indexOf(str,')')+2);
        int day;
        sscanf(str,"this(%d",&day);
        day = -day;
        if(day<0){
            printf("Future Value:'%s'\n",str);
            return 0.0;
        }
        return this_value(str2, data, day, signalData, signalSum, dateIndex, codeIndex, XFALSE);
    }
    
    if(strcmp(str,"")==0){
        return 0.0;
    }
    
    if(strcmp(str,"next.start")==0) //次の日の始値を使う
    {
        const char* str2 = substring(str,5);
        
        return this_value(str2, data, -1, signalData, signalSum, dateIndex, codeIndex, XTRUE);//明日の株価を使える
    }
    
    if(strncmp(str,"nk225.",6)==0)
    {
        //if(nk225data==null){
        //	nk225data = kabuData.loadCacheKabuData(homeDir, 1321);
        //}
        
        const char* str2 = substring(str,6);
        
        return this_value(str2, data, 0, signalData, signalSum, dateIndex, MAX_DATA_COUNT-2, XFALSE);
    }
    if(strncmp(str,"nk225(",6)==0)
    {
        //String daystr = str.substring(6,str.indexOf(")"));
        const char* str2 = substring(str,indexOf(str,')')+2);
        int day;
        sscanf(str,"nk225(%d",&day);
        day = -day;
        if(day<0){
            printf("Future Value:'%s'\n",str);
            return 0.0;
        }
        return this_value(str2, data, day, signalData, signalSum, dateIndex, MAX_DATA_COUNT-2, XFALSE);
    }
    if(strncmp(str,"nk225future(",12)==0)
    {
        const char* str2 = substring(str,indexOf(str,')')+2);
        int day;
        sscanf(str,"nk225future(%d",&day);
        dateIndex += day;
        if (dateIndex >= data->dailys[MAX_DATA_COUNT-2].count)
        {
            dateIndex = data->dailys[MAX_DATA_COUNT-2].count-1;
        }
        return this_value(str2, data, 0, signalData, signalSum, dateIndex, MAX_DATA_COUNT-2, XFALSE);
    }

    if(strncmp(str,"signal(",7)==0){
        return 0.0;
    }
    /*if(str.startsWith("signal(-"))
    {
        String daystr = str.substring(7,str.indexOf(","));
        int dayOffset = Integer.valueOf(daystr);
        String str2 = str.substring(str.indexOf(",")+1);
        String idStr;
        if(str2.indexOf("\"")!=-1){
            idStr = str2.substring(1,str2.lastIndexOf("\""));
        }else{
            idStr = str2.substring(1,str2.length()-1);
        }
        if(signalCache.containsKey(idStr)){
            String signalStr = signalCache.get(idStr).get(""+date);
            if(signalStr!=null){
                return float.valueOf(signalStr);
            }
        }
        File dataFile = new File(homeDir+"/lhuser/"+userId+"/strategyS/"+idStr+"/data.xmap");
        if(dataFile.exists()==false){
            System.out.println("No signal (not exist):"+str);
        }
        xmapString dataXmap = new xmapString(dataFile);
        if(code!=-1 && !signalCache.containsKey(idStr)){
            signalCache.put(idStr, dataXmap);
        }
        int date2 = date;
        for(int j=0; j<date9984.length; j++){
            if(date9984[j]==date) {
                if(date9984[j]==date && j+dayOffset>=0){
                    date2 = date9984[j+dayOffset];
                }else{
                    System.out.println("date offset error");
                }
                break;
            }
        }
        String signalStr = dataXmap.get(""+date2);
        if(signalStr==null) {
            String dateStr = (((date/65536)&255)+1900)+"/"+((date/256)&255)+"/"+(date&255);
            System.out.println("No signal:"+str+" date:"+dateStr);
            return 0.0;
        }
        return float.valueOf(signalStr);
    }
    if(str.startsWith("signal("))
    {
        String str2 = str.substring(7);
        String idStr;
        if(str2.indexOf("\"")!=-1){
            idStr = str2.substring(1,str2.lastIndexOf("\""));
        }else{
            idStr = str2.substring(1,str2.length()-1);
        }
        if(signalCache.containsKey(idStr)){
            String signalStr = signalCache.get(idStr).get(""+date);
            if(signalStr!=null){
                return float.valueOf(signalStr);
            }
        }
        File dataFile = new File(homeDir+"/lhuser/"+userId+"/strategyS/"+idStr+"/data.xmap");
        if(dataFile.exists()==false){
            System.out.println("No signal (not exist):"+str);
        }
        xmapString dataXmap = new xmapString(dataFile);
        if(code!=-1 && !signalCache.containsKey(idStr)){
            signalCache.put(idStr, dataXmap);
        }
        String signalStr = dataXmap.get(""+date);
        if(signalStr==null) {
            String dateStr = (((date/65536)&255)+1900)+"/"+((date/256)&255)+"/"+(date&255);
            System.out.println("No signal:"+str+" date:"+dateStr);
            return 0.0;
        }
        return float.valueOf(signalStr);
    }*/

    if(strcmp(str,"buyvalue")==0)
    {
        if(signalData==NULL){
            printf("You can use 'buyvalue' in Sell/LC Script:%s\n",str);
            return 0.0;
        }
        return signalData->buyValue;
    }
    if(strcmp(str,"dayscount")==0)
    {
        if(signalData==NULL){
            printf("You can use 'buycount' in Sell/LC Script:%s\n",str);
            return 0.0;
        }
        int daysCount = dateIndex - signalData->buyDateIndex;
        return daysCount;
    }
    if(strcmp(str,"year")==0)
    {
        return (data->date[dateIndex]>>16)+1900;
    }
    if(strcmp(str,"month")==0)
    {
        return (data->date[dateIndex]>>8)&0xFF;
    }
    if(strcmp(str,"day")==0)
    {
        return (data->date[dateIndex]&0xFF);
    }
    if(strcmp(str,"code")==0)
    {
        return data->codes[codeIndex];
    }
    if(strcmp(str,"date")==0)
    {
        float x = (data->date[dateIndex]>>16)+1900 + ((data->date[dateIndex]>>8)&0xFF)/100.0 + (data->date[dateIndex]&0xFF)/10000.0;
        return x;
    }
    
    if(strcmp(str," ")==0){
        return 0.0;
    }
    
    // 負数
    if(str[0]=='-'){
        float result = calc_value(&str[1], data, signalData, signalSum, dateIndex, codeIndex);
        return -1.0*result;
    }

    printf("Unknown Value:'%s'\n",str);
    
    return 0.0;

}

static float this_value(const char* str, strMdata* data, int dayMinus, strMsignal* signalData, strMsignalSum* signalSum, int dateIndex, int codeIndex, int canNext)
{
    // this.signal()
    if(strncmp(str,"signal(",7)==0){
        int days;
        sscanf(str,"signal(%d",&days);
        int sum = 0;
        int count = 0;
        for(int i=0; i<days; i++){
            if(dateIndex-dayMinus>=0){
                sum += signalSum->dailyCount[dateIndex-dayMinus-i];
            }
            count++;
        }
        return ((float)sum)/count;
    }
    // this.signal
    if(strcmp(str,"signal")==0){
        if(dateIndex-dayMinus>=0){
            return signalSum->dailyCount[dateIndex-dayMinus];
        }
        else return 0.0;
    }
    
    // this.end
    if(strcmp(str,"end")==0){
        int dayIndex = dateIndex-dayMinus;
        if(dayIndex<0) dayIndex=0;
        if(dayIndex>=data->dailys[codeIndex].count) return 0.0;
        return data->dailys[codeIndex].end[dayIndex];
    }
    // this.start
    if(strcmp(str,"start")==0){
        int dayIndex = dateIndex-dayMinus;
        if(canNext==XFALSE && dayIndex<0) dayIndex=0;
        if(dayIndex>=data->dailys[codeIndex].count) return 0.0;
        return data->dailys[codeIndex].start[dayIndex];
    }
    // this.max
    if(strcmp(str,"max")==0){
        int dayIndex = dateIndex-dayMinus;
        if(dayIndex<0) dayIndex=0;
        if(dayIndex>=data->dailys[codeIndex].count) return 0.0;
        return data->dailys[codeIndex].max[dayIndex];
    }
    // this.min
    if(strcmp(str,"min")==0){
        int dayIndex = dateIndex-dayMinus;
        if(dayIndex<0) dayIndex=0;
        if(dayIndex>=data->dailys[codeIndex].count) return 0.0;
        return data->dailys[codeIndex].min[dayIndex];
    }
    // this.min()
    if(strncmp(str,"min(",4)==0){
        int days;
        sscanf(str,"min(%d",&days);
        int dayIndex = dateIndex-dayMinus;
        if(dayIndex<0) dayIndex=0;
        if(dayIndex>=data->dailys[codeIndex].count) return 0.0;
        if(dayIndex+1<days) days=dayIndex+1;
        int min = 2147483647;
        for(int i=dayIndex; i>dayIndex-days; i--){
            if(data->dailys[codeIndex].min[i] < min && data->dailys[codeIndex].min[i] > 0){
                min = data->dailys[codeIndex].min[i];
            }
        }
        return min;
    }
    // this.max()
    if(strncmp(str,"max(",4)==0){
        int days;
        sscanf(str,"max(%d",&days);
        int dayIndex = dateIndex-dayMinus;
        if(dayIndex<0) dayIndex=0;
        if(dayIndex>=data->dailys[codeIndex].count) return 0.0;
        if(dayIndex+1<days) days=dayIndex+1;
        int max = 0;
        for(int i=dayIndex; i>dayIndex-days; i--){
            if(data->dailys[codeIndex].max[i] > max){
                max = data->dailys[codeIndex].max[i];
            }
        }
        return max;
    }

    // this.kairi()
    if(strncmp(str,"kairi(",6)==0){
        int days;
        sscanf(str,"kairi(%d",&days);
        int dayIndex = dateIndex-dayMinus;
        if(dayIndex<0) dayIndex=0;
        if(dayIndex>=data->dailys[codeIndex].count) return 0.0;
        if(dayIndex+1<days) days=dayIndex+1;
        float sum = 0;
        for(int i=dayIndex; i>dayIndex-days; i--){
            int end = data->dailys[codeIndex].end[i];
            if(end==0){
                sum += data->dailys[codeIndex].end[dayIndex];
            }
            sum += end;
        }
        float mav = sum/days;
        float kairi = (data->dailys[codeIndex].end[dayIndex]-mav)/mav;
        return kairi;
    }
    // this.mav()
    if(strncmp(str,"mav(",4)==0){
        int days;
        sscanf(str,"mav(%d",&days);
        int dayIndex = dateIndex-dayMinus;
        if(dayIndex>=data->dailys[codeIndex].count) return 0.0;
        if(dayIndex+1<days) days=dayIndex+1;
        float sum = 0;
        for(int i=dayIndex; i>dayIndex-days; i--){
            int end = data->dailys[codeIndex].end[i];
            if(end==0){
                sum += data->dailys[codeIndex].end[dayIndex];
            }
            sum += end;
        }
        float mav = sum/days;
        return mav;
    }
    // this.rsi()
    if(strncmp(str,"rsi(",4)==0){
        int days;
        sscanf(str,"rsi(%d",&days);
        int dayIndex = dateIndex-dayMinus;
        if(dayIndex>=data->dailys[codeIndex].count) return 0.0;
        if(dayIndex+1<days) days=dayIndex+1;
        float up = 0;
        float down = 0;
        for(int i=dayIndex; i>dayIndex-days && i>=1; i--){
            int dif = data->dailys[codeIndex].end[i]-data->dailys[codeIndex].end[i-1];
            if(dif>0){
                up += dif;
            }else{
                down += (-dif);
            }
        }
        float rsi = up/(up+down)*100;
        return rsi;
    }
    /*
    // this.atr()
    if(str.startsWith("atr(")){
        String daysStr = str.substring(4,str.indexOf(")"));
        int days = valueOf2(daysStr, sigdata);
        int dayIndex = dateIndex-dayMinus;
        if(dayIndex>=data->dailys[codeIndex].count) return 0.0;
        if(dayIndex+1<days) days=dayIndex+1;
        float atr = 0;
        int count = 0;
        for(int i=dayIndex; i>dayIndex-days && i>=1; i--){
            int a1 = maxArray[i]-minArray[i];
            int a2 = Math.abs(maxArray[i]-endArray[i-1]);
            int a3 = Math.abs(minArray[i]-endArray[i-1]);
            int tr = Math.max(Math.max(a1,a2),a3);
            count++;
            float alpha = 2/(count+1);
            atr = (1-alpha)*atr + alpha * (tr-atr);
        }
        return atr;
    }*/
    
    // this.volume()
    if(strncmp(str,"volume(",7)==0){
        int days;
        sscanf(str,"volume(%d",&days);
        int dayIndex = dateIndex-dayMinus;
        if(dayIndex>=data->dailys[codeIndex].count) return 0.0;
        if(dayIndex+1<days) days=dayIndex+1;
        float sum = 0;
        for(int i=dayIndex; i>dayIndex-days && i>=0; i--){
            float vol = data->dailys[codeIndex].volume[i];
            vol *= data->dailys[codeIndex].end[i];
            sum += vol;
        }
        float volume = sum/days;
        return volume;
    }
    // this.dekidaka
    if(strcmp(str,"dekidaka")==0){
        int dayIndex = dateIndex-dayMinus;
        if(dayIndex<0) dayIndex=0;
        if(dayIndex>=data->dailys[codeIndex].count) return 0.0;
        return data->dailys[codeIndex].volume[dayIndex];
    }
    // this.dekidaka()
    /*if(str.startsWith("dekidaka(")){
        String daysStr = str.substring(9,str.indexOf(")"));
        int days;
        if(daysStr.equals("")){
            days = 0;
        }
        else{
            days = valueOf2(daysStr, sigdata);
        }
        int dayIndex = dateIndex-dayMinus;
        if(dayIndex>=dekidakaArray.length) return 0.0;
        if(dayIndex+1<days) days=dayIndex+1;
        if(days==0){
            return dekidakaArray[dayIndex];
        }
        float sum = 0;
        for(int i=dayIndex; i>dayIndex-days && i>=0; i--){
            float dekidaka = dekidakaArray[i];
            sum += dekidaka;
        }
        return sum / days;
    }
    // this.maxdekidaka()
    if(str.startsWith("maxdekidaka(")){
        String daysStr = str.substring(12,str.indexOf(")"));
        int days = valueOf2(daysStr, sigdata);
        int dayIndex = dateIndex-dayMinus;
        if(dayIndex>=dekidakaArray.length) return 0.0;
        if(dayIndex+1<days) days=dayIndex+1;
        float maxdekidaka = 0;
        for(int i=dayIndex; i>dayIndex-days && i>=0; i--){
            float dekidaka = dekidakaArray[i];
            if(dekidaka > maxdekidaka){
                maxdekidaka = dekidaka;
            }
        }
        return maxdekidaka;
    }*/
    // this.vr()
    if(strncmp(str,"vr(",3)==0){
        int days;
        sscanf(str,"vr(%d",&days);
        int dayIndex = dateIndex-dayMinus;
        float up = 0;
        float down = 0;
        float neut = 0;
        if(data->dailys[codeIndex].volume==NULL){
            return 0;
        }
        for(int i=dayIndex; i>dayIndex-days && i>=1; i--){
            float vol = data->dailys[codeIndex].volume[i];
            if(data->dailys[codeIndex].end[i]-data->dailys[codeIndex].end[i-1]>0){
                up += vol;
            }
            else if(data->dailys[codeIndex].end[i]-data->dailys[codeIndex].end[i-1]<0){
                down += vol;
            }
            else{
                neut += vol;
            }
        }
        float A = up + neut/2;
        float B = down + neut/2;
        if(B==0) B=0.1;
        float vr1 = A/B*100.0;
        return vr1;
    }
    // this.bigsell(*day,*%,*volume)
    if(strncmp(str,"bigsell(",8)==0){
        int days;
        float down;
        float volume;
        sscanf(str,"bigsell(%d,%f%%%fvolume",&days,&down,&volume);
        down /= 100;
        int dayIndex = dateIndex-dayMinus;
        if(dayIndex>=data->dailys[codeIndex].count) return 0.0;
        if(dayIndex+1<days) days=dayIndex+1;
        int count = 0;
        for(int i=dayIndex; i>dayIndex-days && i>=10; i--){
            if(((float)(data->dailys[codeIndex].start[i]-data->dailys[codeIndex].end[i])) / data->dailys[codeIndex].end[i] >= down){
                if(data->dailys[codeIndex].volume[i] >= MIN(data->dailys[codeIndex].volume[i-10],data->dailys[codeIndex].volume[i-5])*volume){
                    count++;
                }
            }
        }
        return count;
    }
    /*
    // this.bigbuy(*day,*%,*volume)
    if(str.startsWith("bigbuy(")){
        String daysStr = str.substring(7,str.indexOf("day"));
        String upStr = str.substring(str.indexOf("day")+4,str.indexOf("%"));
        String volumeStr = str.substring(str.indexOf("%")+2,str.indexOf("volume"));
        int days = valueOf2(daysStr, sigdata);
        float up = float.valueOf(upStr)/100.0;
        float volume = float.valueOf(volumeStr);
        int[] startArray = speedKabu.getStart2Array(code);
        int[] endArray = speedKabu.getEnd2Array(code);
        int[] dekidakaArray = speedKabu.getDekidaka2Array(code);
        int dayIndex = dateIndex-dayMinus;
        if(dayIndex>=data->dailys[codeIndex].count) return 0.0;
        if(dayIndex+1<days) days=dayIndex+1;
        int count = 0;
        for(int i=dayIndex; i>dayIndex-days && i>=10; i--){
            if(((float)(endArray[i]-startArray[i])) / endArray[i] >= up){
                if(dekidakaArray[i] >= Math.min(dekidakaArray[i-10],dekidakaArray[i-5])*volume){
                    count++;
                }
            }
        }
        return count;
    }
    */
    // this.maxkairi([day])
    if(strncmp(str,"maxkairi(",9)==0){
        int days;
        sscanf(str,"maxkairi(%d",&days);
        int dayIndex = dateIndex-dayMinus;
        if(dayIndex>=data->dailys[codeIndex].count) return 0.0;
        if(dayIndex+1<days) days=dayIndex+1;
        float maxkairi = -1.0;
        for(int j=dayIndex; j>dayIndex-200 && j>=0; j--){
            float sum = 0;
            for(int i=dayIndex; i>dayIndex-days; i--){
                sum += data->dailys[codeIndex].end[i];
            }
            float mav = sum/days;
            float kairi = (data->dailys[codeIndex].end[dayIndex]-mav)/mav;
            if(kairi>maxkairi){
                maxkairi = kairi;
            }
        }
        return maxkairi;
    }
    // this.minkairi([day])
    if(strncmp(str,"minkairi(",9)==0){
        int days;
        sscanf(str,"minkairi(%d",&days);
        int dayIndex = dateIndex-dayMinus;
        if(dayIndex>=data->dailys[codeIndex].count) return 0.0;
        if(dayIndex+1<days) days=dayIndex+1;
        float minkairi = 1.0;
        for(int j=dayIndex; j>dayIndex-200 && j>=0; j--){
            float sum = 0;
            for(int i=dayIndex; i>dayIndex-days; i--){
                sum += data->dailys[codeIndex].end[i];
            }
            float mav = sum/days;
            float kairi = (data->dailys[codeIndex].end[dayIndex]-mav)/mav;
            if(kairi<minkairi){
                minkairi = kairi;
            }
        }
        return minkairi;
    }
    // this.ichi([day])
    if(strncmp(str,"ichi(",5)==0){
        int days;
        sscanf(str,"ichi(%d",&days);
        int dayIndex = dateIndex-dayMinus;
        if(dayIndex>=data->dailys[codeIndex].count) return 0.0;
        if(dayIndex+1<days) days=dayIndex+1;
        float max = 0.0;
        float min = 2147000000;
        for(int i=dayIndex; i>dayIndex-days; i--){
            if(data->dailys[codeIndex].max[i] > max ){
                max = data->dailys[codeIndex].max[i];
            }
            if(data->dailys[codeIndex].min[i] < min ){
                min = data->dailys[codeIndex].min[i];
            }
        }
        float ichi = (data->dailys[codeIndex].end[dayIndex] - min ) / ( max - min );
        return ichi;
    }
    /*
    // this.daysfromhigh(days) 最高値からの日数
    if(str.startsWith("daysfromhigh(")){
        String daysStr = str.substring(13,str.indexOf(")"));
        int days = valueOf2(daysStr, sigdata);
        int[] maxArray = speedKabu.getMax2Array(code);
        int dayIndex = dateIndex-dayMinus;
        if(dayIndex>=maxArray.length) return 0.0;
        if(dayIndex+1<days) days=dayIndex+1;
        float max = 0.0;
        int maxI = dayIndex;
        for(int i=dayIndex; i>dayIndex-days; i--){
            if(maxArray[i] > max ){
                max = maxArray[i];
                maxI = i;
            }
        }
        return dayIndex - maxI;
    }
    // this.triangle(days) 直近のある期間に三角形ができているか
    // daysを10分割して、だんだんmaxとminの差が少なくなっているか
    if(str.startsWith("triangle(")){
        String daysStr = str.substring(9,str.indexOf(")"));
        int days = valueOf2(daysStr, sigdata);
        if(days<10) return 0.0;
        int[] maxArray = speedKabu.getMax2Array(code);
        int[] minArray = speedKabu.getMin2Array(code);
        int dayIndex = dateIndex-dayMinus;
        if(dayIndex>=maxArray.length) return 0.0;
        if(dayIndex+1<days) days=dayIndex+1;
        float maxs[] = new float[10];
        float mins[] = new float[10];
        for(int i=0;i<10; i++){
            mins[i] = float.MAX_VALUE;
        }
        for(int i=dayIndex; i>dayIndex-days; i--){
            int j = (dayIndex - i)*10/days;
            if(maxArray[i] > maxs[j] ){
                maxs[j] = maxArray[i];
            }
            if(minArray[i] < mins[j] ){
                mins[j] = minArray[i];
            }
        }
        float sa[] = new float[10];
        float avrsa = 0.0;
        for(int i=0; i<10; i++){
            sa[i] = maxs[i] - mins[i];
            avrsa += sa[i];
        }
        avrsa /= 10;
        float result = 0.0;
        for(int i=0; i<9; i++){
            if(i+1<10){
                if(sa[i+1] > sa[i]){
                    result += 0.1;
                }
            }
            if(i+2<10){
                if(sa[i+2] > sa[i]){
                    result += 0.05;
                }
            }
        }
        if(sa[0] / avrsa < 0.2){
            result += 0.4;
        }
        else if(sa[0] / avrsa < 0.3){
            result += 0.3;
        }
        else if(sa[0] / avrsa < 0.4){
            result += 0.2;
        }
        if(sa[1] / avrsa < 0.6){
            result += 0.1;
        }
        return result;
    }*/
    
    //==============================
    // this.asset
    if(strcmp(str,"asset")==0){
        int dayIndex = dateIndex-dayMinus;
        if(dayIndex<0) dayIndex=0;
        if(data->kessans[codeIndex].asset==NULL){
            return 0;
        }
        float asset = data->kessans[codeIndex].asset[dayIndex];
        return asset;
    }
    // this.netasset
    if(strcmp(str,"netasset")==0){
        int dayIndex = dateIndex-dayMinus;
        if(dayIndex<0) dayIndex=0;
        if(data->kessans[codeIndex].netasset==NULL){
            return 0;
        }
        float asset = data->kessans[codeIndex].netasset[dayIndex];
        return asset;
    }
    // this.eigyoucf
    if(strcmp(str,"eigyoucf")==0){
        int dayIndex = dateIndex-dayMinus;
        if(dayIndex<0) dayIndex=0;
        if(data->kessans[codeIndex].eigyouCf==NULL){
            return 0;
        }
        float asset = data->kessans[codeIndex].eigyouCf[dayIndex];
        return asset;
    }
    // this.profit 純利益
    if(strcmp(str,"profit")==0){
        int dayIndex = dateIndex-dayMinus;
        if(dayIndex<0) dayIndex=0;
        if(data->kessans[codeIndex].pureProfit==NULL){
            return 0;
        }
        float asset = data->kessans[codeIndex].pureProfit[dayIndex];
        return asset;
    }
    // this.keijourieki
    if(strcmp(str,"keijourieki")==0){
        int dayIndex = dateIndex-dayMinus;
        if(dayIndex<0) dayIndex=0;
        if(data->kessans[codeIndex].keijouProfit==NULL){
            return 0;
        }
        float asset = data->kessans[codeIndex].keijouProfit[dayIndex];
        return asset;
    }
    // this.sales
    if(strcmp(str,"sales")==0){
        int dayIndex = dateIndex-dayMinus;
        if(dayIndex<0) dayIndex=0;
        if(data->kessans[codeIndex].sales==NULL){
            return 0;
        }
        float asset = data->kessans[codeIndex].sales[dayIndex];
        return asset;
    }
    // this.per
    if(strcmp(str,"per")==0){
        int dayIndex = dateIndex-dayMinus;
        if(dayIndex<0) dayIndex=0;
        if(data->kessans[codeIndex].eps==NULL){
            return 9999.0;
        }
        float eps = data->kessans[codeIndex].eps[dayIndex];
        if(eps==0) return 9999.0;
        return data->dailys[codeIndex].end[dayIndex]/eps;
    }
    // this.per2
    if(strcmp(str,"per2")==0){
        int dayIndex = dateIndex-dayMinus;
        if(dayIndex<0) dayIndex=0;
        if(data->kessans[codeIndex].eps==NULL){
            return 9999.0;
        }
        float eps = data->kessans[codeIndex].eps[dayIndex];
        float keijou = data->kessans[codeIndex].keijouProfit[dayIndex];
        float jun = data->kessans[codeIndex].pureProfit[dayIndex];
        if(eps==0) return 9999.0;
        return data->dailys[codeIndex].end[dayIndex]/eps * (keijou/jun);
    }
    // this.pbr
    if(strcmp(str,"pbr")==0){
        int dayIndex = dateIndex-dayMinus;
        if(dayIndex<0) dayIndex=0;
        if(data->kessans[codeIndex].bps==NULL){
            return 9999.0;
        }
        float bps = data->kessans[codeIndex].bps[dayIndex];
        if(bps==0) return 9999.0;
        return data->dailys[codeIndex].end[dayIndex]/bps;
    }
    // this.psr
    if(strcmp(str,"psr")==0){
        int dayIndex = dateIndex-dayMinus;
        if(dayIndex<0) dayIndex=0;
        if(data->kessans[codeIndex].bps==NULL){
            return 9999.0;
        }
        float bps = data->kessans[codeIndex].bps[dayIndex];
        float asset = data->kessans[codeIndex].asset[dayIndex];
        float sales = data->kessans[codeIndex].sales[dayIndex];
        if(bps==0 || asset==0) return 9999.0;
        return data->dailys[codeIndex].end[dayIndex]/bps*sales/asset;
    }
    // this.capitalRatio
    if(strcmp(str,"capitalRatio")==0){
        int dayIndex = dateIndex-dayMinus;
        if(dayIndex<0) dayIndex=0;
        if(data->kessans[codeIndex].asset==NULL){
            return 9999.0;
        }
        float asset = data->kessans[codeIndex].asset[dayIndex];
        float netasset = data->kessans[codeIndex].netasset[dayIndex];
        if(netasset==0) return -9999.0;
        return asset/netasset;
    }
    // this.shijou
    if(strcmp(str,"shijou")==0)
    {
        return data->companys[codeIndex].shijou;
    }
    // this.code
    if(strcmp(str,"code")==0)
    {
        return data->codes[codeIndex];
    }
    // this.year
    if(strcmp(str,"year")==0){
        int dayIndex = dateIndex-dayMinus;
        if(dayIndex<0) dayIndex=0;
        return (data->date[dayIndex]>>16)+1900;
    }
    // this.month
    if(strcmp(str,"month")==0){
        int dayIndex = dateIndex-dayMinus;
        if(dayIndex<0) dayIndex=0;
        return (data->date[dayIndex]>>8)&0xFF;
    }
    // this.day
    if(strcmp(str,"day")==0){
        int dayIndex = dateIndex-dayMinus;
        if(dayIndex<0) dayIndex=0;
        return (data->date[dayIndex]&0xFF);
    }
    
    printf("Unknown this-Value:%s\n",str);
    return 0.0;
}

static char** clone(const char** words, int start, int end)
{
    char** logicWords = MALLOC(256 * sizeof(char*) + 256*32);
    //memset(logicWords, 0x00, 256 * sizeof(char*) + 256*32);
    
    for(int i=start; i<=end; i++)
    {
        if(words[i]==NULL){
            logicWords[i] = NULL;
            continue;
        }
        logicWords[i] = (char *)&(logicWords[256+32/4*i]);
        strcpy(logicWords[i],words[i]);
    }
    
    return logicWords;
}

static void cloneFree(char **words, int start, int end)
{
    FREE(words);
}

/* 細切れのメモリだと遅い。もっと早くするには、あらかじめプールに確保したメモリを割り当てるようにすると良さげ。

static char** clone(const char** words, int start, int end)
{
    char** logicWords = MALLOC(256 * sizeof(char*));
    memset(logicWords, 0x00, 256 * sizeof(char*));
    
    for(int i=start; i<=end; i++)
    {
        if(words[i]==NULL){
            continue;
        }
        logicWords[i] = MALLOC(strlen(words[i])+1);
        strcpy(logicWords[i],words[i]);
    }
    
    return logicWords;
}

static void cloneFree(char **words, int start, int end)
{
    for(int i=start; i<=end; i++)
    {
        if(words[i]==NULL){
            continue;
        }
        FREE(words[i]);
        words[i] = NULL;
    }
    FREE(words);
    words = NULL;
} */

static void* my_malloc(size_t size)
{
    return malloc(size);
}

static void my_free(void *ptr)
{
    if(ptr==NULL){
        return;
    }
    
    free(ptr);
}

static const char* substring(const char* str, int start)
{
    return str+start;
}

static int indexOf(const char* str, char c)
{
    for(int i=0; i<strlen(str); i++)
    {
        if(str[i]==c){
            return i;
        }
    }
    
    return -1;
}
