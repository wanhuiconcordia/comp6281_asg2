#ifndef TOOLS_H
#define TOOLS_H
#include <pthread.h>

#ifndef BUFFER_SIZE
#define BUFFER_SIZE (5 * 1024 * 1024)
#endif

enum MessageType{
    TEXT = 0,
    STEP1_STATISTICS = 1,
    STEP2_STATISTICS = 2,
    ERR_MSG = 3,
    TEXTFINISH = 4
};

typedef struct {
    char letter;
    int count;
}Statistics;

typedef struct{
    enum MessageType type;
    int size;
} Packet;

struct _Message{
    enum MessageType type;
    int sender;
    int receiver;
    int size;  //for text, the size is the text length; for statistics, it is the size of statistics array
    void* data;
    struct _Message* next;
    struct _Message* prev;
};

typedef struct _Message Message;


typedef struct{
    Message* header;
    Message* tail;
}MessageList;

typedef struct{
    int myRank;
    pthread_mutex_t mutex;
    int textFinishCount;
    int statisticFinishCount;
    int finalStatisticFinishCount;
    int isReading;
    MessageList msgList;
}ShareResourceOfReadAndMainThread;

typedef struct{
    int myRank;
    int comm_size;
    pthread_mutex_t mutex;
    MessageList msgList;
    int keepWriting;
    int isWriting;
}ShareResourceOfWriteAndMainThread;

void initMessageList(MessageList* pMsgList);

void pushBackMessage(MessageList* pMsgList, Message* pMsg);

Message* popupMessage(MessageList* pMsgList);

void clearMessageList(MessageList* pMsgList);

void printMessage(Message* pMsg);

void printMessageList(MessageList* pMsgList);

int isSessionFinished(int *textFinishedCount, int * staticFinishedCount);

void* readThreadHandler(void* pShareResource);

void* writeThreadHandler(void* pShareResource);

char* readFile(char* fileName);

void initStatistics(Statistics (*pStatisticsArr)[26]);
void printStatistics(Statistics (*pStatisticsArr)[26]);

#endif // TOOLS_H
