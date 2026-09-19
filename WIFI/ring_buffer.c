#include "errno.h"
//#include "libs.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "ring_buffer.h"
//#include "printf.h"

static int RingBufferWrite(struct RingBuffer *ptbuf, const unsigned char *src, unsigned int length);
static int RingBufferRead(struct RingBuffer *ptbuf, unsigned char *dst, unsigned int length);
static int RingBufferClear(struct RingBuffer *ptbuf);
static int RingBufferFree(struct RingBuffer *ptbuf);

/**
 * @brief Create and initialize a ring buffer.
 * @param length Buffer length in bytes.
 * @retval Ring buffer object, or NULL if allocation fails.
 */
struct RingBuffer *RingBufferNew(unsigned int length)
{
    struct RingBuffer *ptbuf;
    if(0 == length)     return NULL;
    
    ptbuf = (struct RingBuffer*)malloc(sizeof(struct RingBuffer));
    if(NULL == ptbuf)   
    {
        return NULL;
    }
    memset(ptbuf, 0, sizeof(struct RingBuffer));
//    if(NULL != ptbuf->info.pHead)
//    {
//        free(ptbuf->info.pHead);
//    }
    ptbuf->info.pHead = (unsigned char*)malloc(length);
    if(NULL == ptbuf->info.pHead) 
    {
        // debugprintf("Error. Malloc %d bytes failed.\r\n", length);
        return NULL;
    }
    ptbuf->info.pValid = ptbuf->info.pValidEnd = ptbuf->info.pHead;
    ptbuf->info.pEnd = ptbuf->info.pHead + length;
    ptbuf->info.nValidLength = 0;
    ptbuf->info.nBufferLength = length;
    
    ptbuf->Write = RingBufferWrite;
    ptbuf->Read = RingBufferRead;
    ptbuf->Clear = RingBufferClear;
    ptbuf->Free = RingBufferFree;
    
    return ptbuf;
}

static int RingBufferFree(struct RingBuffer *ptbuf)
{
    if(ptbuf == NULL)           return -EINVAL;
    if(ptbuf->info.pHead==NULL) return -EINVAL;
    
    free((unsigned char*)ptbuf->info.pHead);
    
    ptbuf->info.pHead = NULL;
    ptbuf->info.pValid = NULL;
    ptbuf->info.pValidEnd = NULL;
    ptbuf->info.pEnd = NULL;
    ptbuf->info.nValidLength = 0;
    
    free((struct RingBuffer *)ptbuf);
    return ESUCCESS;
}

static int RingBufferWrite(struct RingBuffer *ptbuf, const unsigned char *src, unsigned int length)
{
    unsigned int len1 = 0, len2 = 0;
    unsigned int move_len = 0;
    
    if(length > ptbuf->info.nBufferLength)
    {
        return -EINVAL;
    }
    if(ptbuf->info.pHead==NULL)
    {
        return -EINVAL;
    }
    
    // copy buffer to pValidEnd
    if( (ptbuf->info.pValidEnd + length) > ptbuf->info.pEnd )  // 超过了Buffer范围需要分为两段
    {
        len1 = (unsigned)(ptbuf->info.pEnd - ptbuf->info.pValidEnd);
        len2 = length - len1;
        
        memcpy((unsigned char*)ptbuf->info.pValidEnd, src, len1);
        memcpy((unsigned char*)ptbuf->info.pHead, src + len1, len2);
        
        ptbuf->info.pValidEnd = ptbuf->info.pHead + len2;   // 更新有效数据区尾地址
    }
    else
    {
        memcpy((unsigned char*)ptbuf->info.pValidEnd, src, length);
        ptbuf->info.pValidEnd = ptbuf->info.pValidEnd + length;
    }
    
    // 重新计算已使用区的起始位置
    if( (ptbuf->info.nValidLength + length) > ptbuf->info.nBufferLength )     // 要写入的数据超过了缓冲区总长度，分为两段写
    {
        move_len = ptbuf->info.nValidLength + length - ptbuf->info.nBufferLength;
        if( (ptbuf->info.pValid + move_len) > ptbuf->info.pEnd )
        {
            len1 = (unsigned)(ptbuf->info.pEnd - ptbuf->info.pValid);
            len2 = move_len - len1;
            
            ptbuf->info.pValid = ptbuf->info.pHead + len2;
        }
        else
        {
            ptbuf->info.pValid = ptbuf->info.pValid + move_len;
        }
        
        ptbuf->info.nValidLength = ptbuf->info.nBufferLength;
    }
    else
    {
        ptbuf->info.nValidLength = ptbuf->info.nValidLength + length;
    }
    
    return (int)length;
}

static int RingBufferRead(struct RingBuffer *ptbuf, unsigned char *dst, unsigned int length)
{
    unsigned int len1 = 0, len2 = 0;
    if(ptbuf->info.pHead==NULL)     return -EINVAL;
    if(ptbuf->info.nValidLength==0) return -ENOMEM;
    
    if(length > ptbuf->info.nValidLength)
    {
        length = ptbuf->info.nValidLength;
    }
    
    if( (ptbuf->info.pValid + length) > ptbuf->info.pEnd )
    {
        len1 = (unsigned int)(ptbuf->info.pEnd - ptbuf->info.pValid);
        len2 = length - len1;
        
        memcpy(dst, (unsigned char*)ptbuf->info.pValid, len1);
        memcpy(dst + len1, (unsigned char*)ptbuf->info.pHead, len2);
        
        ptbuf->info.pValid = ptbuf->info.pHead + len2;
    }
    else
    {
        memcpy(dst, (unsigned char*)ptbuf->info.pValid, length);
        ptbuf->info.pValid = ptbuf->info.pValid + length;
    }
    
    ptbuf->info.nValidLength -= length;
    
    return (int)length;
}

static int RingBufferClear(struct RingBuffer *ptbuf)
{
    if(ptbuf == NULL)           return -EINVAL;
    if(ptbuf->info.pHead==NULL) return -EINVAL;
    if(ptbuf->info.pHead != NULL)
    {
        memset(ptbuf->info.pHead, 0, ptbuf->info.nBufferLength);
    }
    
    ptbuf->info.pValid = ptbuf->info.pValidEnd = ptbuf->info.pHead;
    ptbuf->info.nValidLength = 0;
    return ESUCCESS;
}
