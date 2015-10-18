#include <stdio.h>
#include <mpi.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <stdlib.h>
#include "tools.h"

int main(int argc, char** argv)
{
    MPI_Init(&argc, &argv);
    int commSize;
    int myRank;
    MPI_Comm_size(MPI_COMM_WORLD, &commSize);
    MPI_Comm_rank(MPI_COMM_WORLD, &myRank);

    Statistics finalStatistics[26];
    initStatistics(&finalStatistics);
    Statistics step1statitics[26];
    initStatistics(&step1statitics);
    Statistics step2statitics[26];
    initStatistics(&step2statitics);

    if(commSize < 1 || commSize > 5){
        if(myRank == 0){
            printf("Process size should be in the range of [1, 5].\n");
        }
    }else{
        ShareResourceOfReadAndMainThread shareResource1;
        shareResource1.finalStatisticFinishCount = commSize - 1;
        shareResource1.statisticFinishCount = commSize - 1;
        shareResource1.textFinishCount = commSize - 1;
        shareResource1.myRank = myRank;
        shareResource1.isReading = 1;
        initMessageList(&shareResource1.msgList);

        ShareResourceOfWriteAndMainThread shareResource2;
        shareResource2.comm_size = commSize;
        shareResource2.myRank = myRank;
        shareResource2.keepWriting = 1;
        shareResource2.isWriting = 1;
        initMessageList(&shareResource2.msgList);

        char fileName[12];
        sprintf(fileName, "input%d.txt", myRank);
        char *pDocText = readFile(fileName);
        if(pDocText){
            if (pthread_mutex_init(&(shareResource1.mutex), NULL)
                    || pthread_mutex_init(&(shareResource2.mutex), NULL))
            {
                printf("mutex init failed\n");

            }else{
                pthread_t readThreadId;
                pthread_t writeThreadId;
                if (pthread_create(&readThreadId, NULL, &readThreadHandler, (void*)&shareResource1)){
                    printf("\ncan't create read thread.\n");
                }else{
                    if (pthread_create(&writeThreadId, NULL, &writeThreadHandler, (void*)&shareResource2)){
                        printf("\ncan't create write thread.\n");
                    }else{
                        struct timespec waitInterval;
                        int defaultBlockSize = strlen(pDocText) / commSize;

                        for(int rank = 0; rank < commSize; rank++){
                            int blockSize = defaultBlockSize;
                            if(rank == commSize - 1){
                                blockSize = strlen(pDocText) - defaultBlockSize * rank;
                            }

                            Message* pMsg1 = (Message*) calloc(1, sizeof(Message));
                            pMsg1->type = TEXT;
                            pMsg1->data = (void*) calloc (blockSize + 1, sizeof(char));
                            memcpy(pMsg1->data, pDocText + rank * defaultBlockSize, blockSize);
                            pMsg1->size = blockSize + 1;
                            pMsg1->receiver = rank;
                            pMsg1->sender = myRank;

//                            Message* pMsg2 = (Message*) calloc(1, sizeof(Message));
//                            pMsg2->type = TEXTFINISH;
//                            pMsg2->data = NULL;
//                            pMsg2->size = 0;
//                            pMsg2->receiver = rank;
//                            pMsg2->sender = myRank;

                            clock_gettime(CLOCK_REALTIME, &waitInterval);
                            waitInterval.tv_sec += 2;

                            if(rank == myRank){
                                if(pthread_mutex_timedlock(&(shareResource1.mutex),&waitInterval)){
                                    printf("timed out. Will terminat session.\n");
                                }else{
                                    pushBackMessage(&(shareResource1.msgList), pMsg1);
//                                    pushBackMessage(&(shareResource1.msgList), pMsg2);
                                    pthread_mutex_unlock(&(shareResource1.mutex));
                                }
                            }else{
                                if(pthread_mutex_timedlock(&(shareResource2.mutex),&waitInterval)){
                                    printf("timed out. Will terminat session.\n");
                                }else{
                                    pushBackMessage(&(shareResource2.msgList), pMsg1);
//                                    pushBackMessage(&(shareResource2.msgList), pMsg2);
                                    pthread_mutex_unlock(&(shareResource2.mutex));
                                }
                            }
                        }
                        free(pDocText);

                        Message* pMsg = NULL;
                        int isStatisticDispatched = 0;
                        while(1){
                            clock_gettime(CLOCK_REALTIME, &waitInterval);
                            waitInterval.tv_sec += 2;

                            if(pthread_mutex_timedlock(&(shareResource1.mutex),&waitInterval)){
                                printf("timed out. Will terminat session.\n");
                            }else{
                                pMsg = popupMessage(&(shareResource1.msgList));
                                pthread_mutex_unlock(&(shareResource1.mutex));
                            }

                            if(pMsg){
                                if(pMsg->type == TEXT){
                                    char *p = pMsg->data;
                                    while(*p){
                                        if(*p >= 'A' && *p <= 'Z'){
                                            step1statitics[*p - 'A'].count++;
                                        }else if(*p >= 'a' && *p <= 'z'){
                                            step1statitics[*p - 'a'].count++;
                                        }
                                        ++p;
                                    }
                                }else if(pMsg->type == STEP1_STATISTICS){
                                    Statistics* p = (Statistics *) pMsg->data;
                                    for(int i = 0; i < pMsg->size; i++){
                                        step2statitics[p[i].letter - 'A'].count += p[i].count;
                                    }
                                }else if(pMsg->type == STEP2_STATISTICS){
                                    Statistics* p = (Statistics*) pMsg->data;
                                    for(int i = 0; i < pMsg->size; i++){
                                        finalStatistics[p[i].letter - 'A'].count += p[i].count;
                                    }
                                }
                                free(pMsg->data);
                                free(pMsg);
                            }

                            if(shareResource1.textFinishCount == 0
                                    && !isStatisticDispatched){
                                //send statics to all other process

                                printf("MyRank:%d main thread will dispatch step1 statistics\n", myRank);
                                int defaultBlockSize = 26 / commSize;

                                for(int rank = 0; rank < commSize; rank++){
                                    int blockSize = defaultBlockSize;
                                    if(rank == commSize - 1){
                                        blockSize = 26 - rank * defaultBlockSize;
                                    }
                                    Message* pMsg = (Message*) calloc(1, sizeof(Message));
                                    pMsg->type = STEP1_STATISTICS;
                                    pMsg->data = (void*) calloc (blockSize, sizeof(Statistics));
                                    memcpy(pMsg->data, step1statitics + rank * defaultBlockSize, blockSize * sizeof(Statistics));
                                    pMsg->size = blockSize;
                                    pMsg->receiver = rank;
                                    pMsg->sender = myRank;

                                    clock_gettime(CLOCK_REALTIME, &waitInterval);
                                    waitInterval.tv_sec += 2;
                                    if(rank == myRank){
                                        if(pthread_mutex_timedlock(&(shareResource1.mutex),&waitInterval)){
                                            printf("timed out. Will terminat session.\n");
                                        }else{
                                            printf("MyRank:%d push a message to shareResource1111111.msgList\n", myRank);
                                            pushBackMessage(&(shareResource1.msgList), pMsg);
                                            pthread_mutex_unlock(&(shareResource1.mutex));
                                        }
                                    }else{

                                        if(pthread_mutex_timedlock(&(shareResource2.mutex),&waitInterval)){
                                            printf("timed out. Will terminat session.\n");
                                        }else{
                                            printf("MyRank:%d push a message to shareResource2222222.msgList\n", myRank);
                                            pushBackMessage(&(shareResource2.msgList), pMsg);
                                            pthread_mutex_unlock(&(shareResource2.mutex));
                                        }
                                    }

//                                    printMessageList(&(shareResource1.msgList));
                                }

                                isStatisticDispatched = 1;
                            }

                            if(myRank == 0){
                                if(shareResource1.finalStatisticFinishCount == 0){
                                    for(unsigned i = 0; i < sizeof(step2statitics); i++){
                                        finalStatistics[step2statitics[i].letter - 'A'].count += step2statitics[i].count;
                                    }

                                    //printStatistics(&finalStatistics);

                                    printf("Root process received final statistics from all other process. will finishe\n");
                                    shareResource2.keepWriting = 0;
                                    while(shareResource1.isReading || shareResource2.isWriting){
                                        if(shareResource1.isReading)
                                            printf("MyRank:%d waiting for read thread to finish\n", myRank);
                                        else
                                            printf("MyRank:%d waiting for write thread to finish\n", myRank);
                                        sleep(1);
                                    }
                                    break;
                                }
                            }else{
                                if(shareResource1.statisticFinishCount == 0){
                                    //send the step2Statistics to root process
                                    Message* pMsg = (Message*) calloc(1, sizeof(Message));
                                    pMsg->type = STEP2_STATISTICS;
                                    pMsg->data = (void*) calloc (26, sizeof(Statistics));
                                    memcpy(pMsg->data, step2statitics, 26 * sizeof(Statistics));
                                    pMsg->size = 26;
                                    pMsg->receiver = 0;
                                    pMsg->sender = myRank;

                                    clock_gettime(CLOCK_REALTIME, &waitInterval);
                                    waitInterval.tv_sec += 2;
                                    if(pthread_mutex_timedlock(&(shareResource2.mutex),&waitInterval)){
                                        printf("timed out. Will terminat session.\n");
                                    }else{
                                        pushBackMessage(&(shareResource2.msgList), pMsg);
                                        pthread_mutex_unlock(&(shareResource2.mutex));
                                    }

                                    printf("MyRank:%d child process send the final statistics to root. will terminate\n", myRank);
                                    shareResource2.keepWriting = 0;
                                    while(shareResource1.isReading || shareResource2.isWriting){
                                        if(shareResource1.isReading)
                                            printf("MyRank:%d waiting for read thread to finish\n", myRank);
                                        else
                                            printf("MyRank:%d waiting for write thread to finish\n", myRank);
                                        sleep(1);
                                    }
                                    break;
                                }
                            }
                        }
                        pthread_join(&writeThreadId, NULL);
                    }
                    pthread_join(&readThreadId, NULL);
                }
                pthread_mutex_destroy(&(shareResource1.mutex));
                pthread_mutex_destroy(&(shareResource2.mutex));
            }
        }else{
            printf("Failed to open file:%s\n", fileName);
        }

    }
    MPI_Finalize();
    return 0;
}
