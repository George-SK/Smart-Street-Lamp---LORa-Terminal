#include<stdio.h>  
#include<stdlib.h>
#include<string.h>
#include "board.h"
#include "data-processing.h"
#include "LoRaMac.h"
#include "Region.h"

#define DEFAULT_DATARATE	DR_6	//Modified by Steven in 2018/06/08.

static bool SendQueueFrame( uint8_t msgType, uint8_t fPort, uint8_t *fBuffer, uint16_t fBufferSize, uint8_t nbRetries );

QUEUE Qmsg;

void QueueInit(void)
{
	CreateQueue(&Qmsg, MESSAGE_MAX_SIZE);
}

/*! 
 * Create a empty Queue; 
 */  
char CreateQueue(PQUEUE Q,int maxsize)  
{  
    /*Q->pBase=(unsigned char *)malloc(sizeof(unsigned char)*maxsize);  
    if(NULL==Q->pBase)  
    {  
        Uart1.PrintfLog("Memory allocation failure");  
        return -1;         
    }*/  
    Q->front=0;         //parameter initialization.  
    Q->rear=0;  
    Q->maxsize=maxsize;
    
    return 0;
}  
/*! 
 * Print the Queue element; 
 */  
void TraverseQueue(PQUEUE Q)  
{  
    int i=Q->front, j; 
    
    Uart1.PrintfLog("************Traversing Queue:\r\n");  
    while(i%Q->maxsize!=Q->rear)  
    {  
      Uart1.PrintfLog("msg%d: ", i);
        for(j = 0; (Q->pBase[i].data[j]!='\0' && j<QUEUE_DATA_MAX_SIZE); j++)
          Uart1.PrintfLog("%x ", Q->pBase[i].data[j]);
        Uart1.PrintfLog("\r\n");
        i++;  
    }  
    Uart1.PrintfLog("************end************\r\n"); 
}

bool FullQueue(PQUEUE Q)  
{  
    if(Q->front==(Q->rear+1)%Q->maxsize)    //Reserve one element.  
        return true;  
    else  
        return false;  
}

bool EmptyQueue(PQUEUE Q)  
{  
    if(Q->front==Q->rear)      
        return true;  
    else  
        return false;  
}

bool Enqueue(PQUEUE Q, unsigned char *message, unsigned short size)  
{  
    if(FullQueue(Q))  
        return false;  
    else  
    {  
        if(size > QUEUE_DATA_MAX_SIZE)
          Q->pBase[Q->rear].DataSize = QUEUE_DATA_MAX_SIZE;
        else
          Q->pBase[Q->rear].DataSize = size;
        
        memcpy1(Q->pBase[Q->rear].data, message, Q->pBase[Q->rear].DataSize);
        Q->rear=(Q->rear+1)%Q->maxsize;  
        return true;  
    }  
}  
  
bool Dequeue(PQUEUE Q, unsigned char *message, unsigned char *size)  
{  
    if(EmptyQueue(Q))  
    {  
        return false;  
    }  
    else  
    {  
        memcpy1(message, Q->pBase[Q->front].data, Q->pBase[Q->front].DataSize);
        *size = Q->pBase[Q->front].DataSize;
        Q->front=(Q->front+1)%Q->maxsize;  
        return true;  
    }  
}

void QueueTest(void)
{
  uint8_t i, j, size;
  int error;
  unsigned char str[MESSAGE_MAX_SIZE][20];
  static unsigned char buf[128];
  
  error = CreateQueue(&Qmsg, MESSAGE_MAX_SIZE);
  if(error < 0)
  {
    Uart1.PrintfLog("Create Queue failed!\r\n");
    return;
  }
  else
    Uart1.PrintfLog("Create Queue OK\r\n");
  
    for(i = 0; i < MESSAGE_MAX_SIZE-1; i++)
    {
      for(j = 0; j < 20; j++)
        str[i][j] = i+1;
      
      if(Enqueue(&Qmsg, str[i], 20) == false)
        Uart1.PrintfLog("Enqueue failed!\r\n");
      else
        Uart1.PrintfLog("Enqueue OK\r\n");
    }
    
    TraverseQueue(&Qmsg);
    Uart1.PrintfLog("Dequeue\r\n");
    if(Dequeue(&Qmsg, buf, &size) == true)
    {
      Uart1.PrintfLog("Dequeue(%d):", size);
      for(i = 0; i < size; i++)
        Uart1.PrintfLog("%d ", buf[i]);
      Uart1.PrintfLog("\r\n");
    }
    
    if(Dequeue(&Qmsg, buf, &size) == true)
    {
      Uart1.PrintfLog("Dequeue(%d):", size);
      for(i = 0; i < size; i++)
        Uart1.PrintfLog("%d ", buf[i]);
      Uart1.PrintfLog("\r\n");
    }
    
    TraverseQueue(&Qmsg);
    /*if(Enqueue(&Qmsg, str[10], 20) == false)
        Uart1.PrintfLog("Enqueue failed!\r\n");
      else
        Uart1.PrintfLog("Enqueue OK\r\n");
     TraverseQueue(&Qmsg);*/
}

/*#####################################################################################################*/


/*!
 * \brief   Insert one Frame to the Queue.
 */
void InsertOneFrame( frame_t *frame )
{  
  if(Enqueue(&Qmsg, (unsigned char *)frame, sizeof(frame_t)) == false)
    Uart1.PrintfLog("Insert frame failed!\r\n");
  else
    Uart1.PrintfLog("Insert frame OK\r\n");
}


int SendQueueFrames( void )
{
    uint8_t sendFrameStatus = 0, len;
    frame_t frame;
    
    if(Dequeue(&Qmsg, (unsigned char *)&frame, &len) == false)   
      return -1;    
    
    sendFrameStatus = SendQueueFrame( frame.IsTxConfirmed, frame.port, frame.AppData, frame.AppDataSize, frame.nbRetries );
    
    return sendFrameStatus;
}

void QueueMessageTest(void)
{
  frame_t frame;
  
  frame.port = 3;
  frame.IsTxConfirmed = 1;
  frame.nbRetries = 4;
  frame.AppDataSize = 4;
  frame.AppData[0] = 0;
  frame.AppData[1] = 1;
  frame.AppData[2] = 2;
  frame.AppData[3] = 3;  
  
  InsertOneFrame(&frame);
  
  frame.AppData[0] = 4;
  frame.AppData[1] = 5;
  frame.AppData[2] = 6;
  frame.AppData[3] = 7;
  
  InsertOneFrame(&frame);
}

static bool SendQueueFrame( uint8_t msgType, uint8_t fPort, uint8_t *fBuffer, uint16_t fBufferSize, uint8_t nbRetries )
{
    McpsReq_t mcpsReq;
    LoRaMacTxInfo_t txInfo;

    if( LoRaMacQueryTxPossible( fBufferSize, &txInfo ) != LORAMAC_STATUS_OK )
    {
        // Send empty frame in order to flush MAC commands
        mcpsReq.Type = MCPS_UNCONFIRMED;
        mcpsReq.Req.Unconfirmed.fBuffer = NULL;
        mcpsReq.Req.Unconfirmed.fBufferSize = 0;
        mcpsReq.Req.Unconfirmed.Datarate = DEFAULT_DATARATE;
    }
    else
    {
        if( msgType == 0 )
        {
            mcpsReq.Type = MCPS_UNCONFIRMED;
            mcpsReq.Req.Unconfirmed.fPort = fPort;
            mcpsReq.Req.Unconfirmed.fBuffer = fBuffer;
            mcpsReq.Req.Unconfirmed.fBufferSize = fBufferSize;
            mcpsReq.Req.Unconfirmed.Datarate = DEFAULT_DATARATE;
        }
        else
        {
            mcpsReq.Type = MCPS_CONFIRMED;
            mcpsReq.Req.Confirmed.fPort = fPort;
            mcpsReq.Req.Confirmed.fBuffer = fBuffer;
            mcpsReq.Req.Confirmed.fBufferSize = fBufferSize;
            mcpsReq.Req.Confirmed.NbTrials = nbRetries;
            mcpsReq.Req.Confirmed.Datarate = DEFAULT_DATARATE;
        }
    }

    if( LoRaMacMcpsRequest( &mcpsReq ) == LORAMAC_STATUS_OK )
    {
        return false;
    }
    return true;
}
