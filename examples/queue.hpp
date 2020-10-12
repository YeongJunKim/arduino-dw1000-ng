
#pragma once

#include <stdlib.h>
#include <string.h>
#include <Arduino.h>
#define TRUE 1
#define FALSE 0


#define LEN_DATA 24

#define QUE_LEN 100 // 큐의 크기를 100으로 잡는다. 이때 원형 큐 MAX는 99이다.

typedef struct _frame
{
    uint8_t data[LEN_DATA];
}Data;
 
typedef struct _cQueue
{
    int front; // 큐의 시작점 F 
    int rear; // 큐의 끝점 R 
    Data queArr[QUE_LEN];
} CQueue;
 
typedef CQueue Queue; // Cqueue 구조체를 Queue로 선언한다.
 
void QueueInit(Queue *pq); // Queue를 pq로 본다(이 함수 안에서는)
int QIsEmpty(Queue *pq);
 
void Enqueue(Queue *pq, Data data);
Data Dequeue(Queue *pq);
Data QPeek(Queue *pq);
