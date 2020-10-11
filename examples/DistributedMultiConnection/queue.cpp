#include <stdio.h>
#include <stdlib.h>
#include <queue.hpp>
 
void QueueInit(Queue *pq) // 텅 빈 경우 front와  rear은 동일위치를 가리킨다.
{
    pq->front = 0;  // 초기화 과정
    pq->rear = 0;
}
 
int QIsEmpty(Queue *pq) 
{
    if (pq->front == pq->rear) // 큐가 텅 비었다면,
        return TRUE;
 
    else
        return FALSE;
}
 
int NextPosIdx(int pos) // 큐의 다음 위치에 해당하는 인덱스 값 반환
{
    if (pos == QUE_LEN - 1) //  만약 현재위치가 큐길이 - 1이라면 // 즉 배열의 마지막 요소의 인덱스 값이라면
        return 0; // 0을 반환(큐의 끝에 도달했으므로 회전을 돕는 함수이다)
 
    else
        return pos + 1; // 그외에는 다음 큐를 가리키도록
}
 
void Enqueue(Queue *pq, Data data)
{
    if (NextPosIdx(pq->rear) == pq->front) // 큐가 꽉 찼다면,
    {
        printf("Queue Memory Eroor!"); // 여기 접근을 할 수 없는 코드인데 접근한다면 에러구문 표시 후 종료
        exit(-1); // 즉, 원형 큐에서는 전체크기 - 1을 기준으로 F,R이 만나지 않게되는 알고리즘인데 F==Q가 될 수 없다.
    }
 
    pq->rear = NextPosIdx(pq->rear); // rear을 한 칸 이동
    pq->queArr[pq->rear] = data; // rear이 가리키는 곳에 데이터 저장
}
 
Data Dequeue(Queue *pq)
{
    if (QIsEmpty(pq)) // 아무것도 없는 상태에서 큐에 있는 데이터를 뺀다는 것은 오류.
    {
        printf("Queue Memory Error!");
        exit(-1);
    }
 
    pq->front = NextPosIdx(pq->front); // front를 한 칸 이동한다
    return pq->queArr[pq->front];  // front가 가리키는 데이터를 반환한다.
}
 
Data QPeek(Queue *pq)           
{
    if (QIsEmpty(pq))
    {
        printf("Queue Memory Error!");
        exit(-1);
    }
 
    return pq->queArr[NextPosIdx(pq->front)];
}

