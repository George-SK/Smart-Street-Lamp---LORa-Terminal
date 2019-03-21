#ifndef _DATA_PROCESSING_H_
#define _DATA_PROCESSING_H_

#include "stdbool.h"
#include "stdint.h"
#include "macro-definition.h"

#if defined (FOTA_ENABLE)
#define USER_DATA_MAX_SIZE      154  
#define MESSAGE_MAX_SIZE        16
#define QUEUE_DATA_MAX_SIZE     174 
#else
#define USER_DATA_MAX_SIZE      64  
#define MESSAGE_MAX_SIZE        16
#define QUEUE_DATA_MAX_SIZE     84  
#endif

typedef struct 
{
  unsigned char data[QUEUE_DATA_MAX_SIZE];
  unsigned char DataSize;
}pBase_t;

typedef struct
{
  uint8_t port;
  uint8_t IsTxConfirmed;
  uint8_t nbRetries;
  uint8_t AppDataSize;  
  uint8_t AppData[USER_DATA_MAX_SIZE];
}frame_t;

typedef struct queue   
{  
    pBase_t pBase[MESSAGE_MAX_SIZE];  
    int front;    
    int rear;    
    int maxsize; 
}QUEUE,*PQUEUE;  

extern QUEUE Qmsg;
  
void QueueInit(void);
char CreateQueue(PQUEUE Q,int maxsize);  
void TraverseQueue(PQUEUE Q);  
bool FullQueue(PQUEUE Q);  
bool EmptyQueue(PQUEUE Q);  
bool Enqueue(PQUEUE Q, unsigned char *message, unsigned short size);  
bool Dequeue(PQUEUE Q, unsigned char *message, unsigned char *size); 
int SendQueueFrames( void );
void QueueMessageTest(void);
void InsertOneFrame( frame_t *frame );

#endif

