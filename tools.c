#include "tools.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include <unistd.h>
#include <time.h>
#include <mpi.h>

void initMessageList(MessageList* pMsgList){
    pMsgList->header = calloc(1, sizeof(Message));
    pMsgList->tail = calloc(1, sizeof(Message));
    pMsgList->header->next = pMsgList->tail;
    pMsgList->tail->prev = pMsgList->header;
}

void pushBackMessage(MessageList* pMsgList, Message* pMsg){
    pMsg->next = pMsgList->tail;
    pMsg->prev = pMsgList->tail->prev;
    pMsg->prev->next = pMsg;
    pMsgList->tail->prev = pMsg;
}

Message* popupMessage(MessageList* pMsgList){
    if(pMsgList->header->next == pMsgList->tail){
        return NULL;
    }else{
        Message* pRetMsg = pMsgList->header->next;
        pMsgList->header->next = pRetMsg->next;
        pRetMsg->next->prev = pMsgList->header;
        return pRetMsg;
    }
}

void clearMessageList(MessageList* pMsgList){
    Message* pMsg = pMsgList->header;

    while(pMsgList->header){
        pMsgList->header = pMsgList->header->next;
        if(pMsg->data){
            //printf("data %ld of message %ld will be deleted.\n", pMsg->data, pMsg);
            free(pMsg->data);
        }
        //printf("message %ld will be deleted.\n", pMsg);
        free(pMsg);
        pMsg = pMsgList->header;
    }
}

void printMessage(Message* pMsg){
    switch(pMsg->type){
    case TEXT:
    case ERR_MSG:
        printf("msg.type:%u, msg.size:%d, data:%s\n", pMsg->type, pMsg->size, (char*)pMsg->data);
        break;
    case STEP1_STATISTICS:
    case STEP2_STATISTICS:
    {
        Statistics* pStatisticData = (Statistics*)(pMsg->data);
        printf("msg.type:%d, msg.size:%d\n", pMsg->type, pMsg->size);
        for(int i = 0; i < pMsg->size; i++){
            printf("\tletter:%c, count:%d\n", (pStatisticData + i)->letter, (pStatisticData + i)->count);
        }
    }
        break;
    case TEXTFINISH:
        printf("finished sending text.\n");
        break;
    }
}

void printMessageList(MessageList* pMsgList){
    Message* pMsg = pMsgList->header->next;
    while(pMsg != pMsgList->tail){
        printMessage(pMsg);
        pMsg = pMsg->next;
    }
}

void* readThreadHandler(void* p)
{
    char buffer[BUFFER_SIZE];
    bzero(buffer, BUFFER_SIZE);
    ShareResourceOfReadAndMainThread* pShareResource = (ShareResourceOfReadAndMainThread*) p;

    while(1){
        sleep(1);
        MPI_Status status;
        if(MPI_Recv(buffer, BUFFER_SIZE, MPI_CHAR, MPI_ANY_SOURCE, 0, MPI_COMM_WORLD, &status)){
            printf("MPI_Recv error.\n");
            break;
        }else{
            Packet* pPacket = (Packet* )buffer;

            if(pPacket->type == TEXT){
                Message* pMsg = calloc(1, sizeof(Message));
                pMsg->type = pPacket->type;
                pMsg->size = pPacket->size;
                pMsg->sender = status.MPI_SOURCE;

                pMsg->data = calloc(pPacket->size + 1, sizeof(char));
                memcpy(pMsg->data, ((void*)pPacket) + sizeof(Packet), pPacket->size);

                struct timespec waitInterval;
                clock_gettime(CLOCK_REALTIME, &waitInterval);
                waitInterval.tv_sec += 2;

                //printMessage(pMsg);
                if(pthread_mutex_timedlock(&(pShareResource->mutex),&waitInterval)){
                    //if(pShareResource->rank == 0){
                    printf("child thread wait timed out. Will terminat session.\n");
                }else{
                    pushBackMessage(&(pShareResource->msgList), pMsg);
                    pShareResource->textFinishCount--;
                    pthread_mutex_unlock(&(pShareResource->mutex));
                }
            }else if(pPacket->type == STEP1_STATISTICS){
                Message* pMsg = calloc(1, sizeof(Message));
                pMsg->type = pPacket->type;
                pMsg->size = pPacket->size;
                pMsg->sender = status.MPI_SOURCE;

                pMsg->data = calloc(pPacket->size, sizeof(Statistics));
                memcpy(pMsg->data, ((void*)pPacket) + sizeof(Packet), pPacket->size * sizeof(Statistics));

                struct timespec waitInterval;
                clock_gettime(CLOCK_REALTIME, &waitInterval);
                waitInterval.tv_sec += 2;

                if(pthread_mutex_timedlock(&(pShareResource->mutex),&waitInterval)){
                    //if(pShareResource->rank == 0){
                    printf("child thread wait timed out. Will terminat session.\n");
                }else{
                    pushBackMessage(&(pShareResource->msgList), pMsg);
                    pShareResource->statisticFinishCount--;
                    pthread_mutex_unlock(&(pShareResource->mutex));
                }
            }else if(pPacket->type == STEP2_STATISTICS){
                Message* pMsg = calloc(1, sizeof(Message));
                pMsg->type = pPacket->type;
                pMsg->size = pPacket->size;
                pMsg->sender = status.MPI_SOURCE;

                pMsg->data = calloc(pPacket->size, sizeof(Statistics));
                memcpy(pMsg->data, ((void*)pPacket) + sizeof(Packet), pPacket->size * sizeof(Statistics));

                struct timespec waitInterval;
                clock_gettime(CLOCK_REALTIME, &waitInterval);
                waitInterval.tv_sec += 2;

                if(pthread_mutex_timedlock(&(pShareResource->mutex),&waitInterval)){
                    //if(pShareResource->rank == 0){
                    printf("child thread wait timed out. Will terminat session.\n");
                }else{
                    pushBackMessage(&(pShareResource->msgList), pMsg);
                    pShareResource->finalStatisticFinishCount--;
                    pthread_mutex_unlock(&(pShareResource->mutex));
                }
            }else if(pPacket->type == TEXTFINISH){
//                Message* pMsg = calloc(1, sizeof(Message));
//                pMsg->type = pPacket->type;
//                pMsg->size = pPacket->size;
//                pMsg->sender = status.MPI_SOURCE;

//                pMsg->data = calloc(pPacket->size, sizeof(Statistics));
//                memcpy(pMsg->data, ((void*)pPacket) + sizeof(Packet), pPacket->size * sizeof(Statistics));

//                struct timespec waitInterval;
//                clock_gettime(CLOCK_REALTIME, &waitInterval);
//                waitInterval.tv_sec += 2;

//                if(pthread_mutex_timedlock(&(pShareResource->mutex),&waitInterval)){
//                    //if(pShareResource->rank == 0){
//                    printf("child thread wait timed out. Will terminat session.\n");
//                }else{
//                    pushBackMessage(&(pShareResource->msgList), pMsg);
//                    pShareResource->finalStatisticFinishCount--;
//                    pthread_mutex_unlock(&(pShareResource->mutex));
//                }
            }else if(pPacket->type == ERR_MSG){
                if(pShareResource->myRank == 0){
                    printf("ERROR:%s\n", buffer + sizeof(pPacket));
                    printf("Will terminat session.\n");
                }else{
                    //TODO Try to send error message to root process.
                }
            }else{
                if(pShareResource->myRank == 0){
                    printf("Unrecognized message. Will terminat session.\n");
                }else{
                    //TODO Try to send error message to root process.
                }
            }

            printf("++++++MyRank:%d,Read thread get a msg:%d, finalStatisticFinishCount:%d, statisticFinishCount:%d, textFinishCount:%d\n"
                   ,pShareResource->myRank, pPacket->type, pShareResource->finalStatisticFinishCount, pShareResource->statisticFinishCount, pShareResource->textFinishCount);
            if(pShareResource->myRank == 0){
                printf("**********\n");

                //IS THIS A GCCXX BUG???????????????????????????????????
                if((pShareResource->finalStatisticFinishCount == 0)
                        /*&& (pShareResource->statisticFinishCount == 0)
                        && (pShareResource->textFinishCount == 0)*/){
                    printf("####################\n");
                    printf("####################\n");
                    break;
                }
            }else{
                if(pShareResource->statisticFinishCount == 0
                        && pShareResource->textFinishCount == 0){
                    break;
                }
            }
        }
    }
    printf("++++++MyRank:%d read thread finished\n", pShareResource->myRank);
    pShareResource->isReading = 0;
    return NULL;
}

void* writeThreadHandler(void* p)
{
    ShareResourceOfWriteAndMainThread* pShareResource = (ShareResourceOfWriteAndMainThread*) p;
    struct timespec waitInterval;
    Message* pMsg;
    while(1){
        pMsg = NULL;
        clock_gettime(CLOCK_REALTIME, &waitInterval);
        waitInterval.tv_sec += 2;
        if(pthread_mutex_timedlock(&(pShareResource->mutex),&waitInterval)){
            //if(pShareResource->rank == 0){
            printf("child thread wait timed out. Will terminat session.\n");
        }else{
            pMsg = popupMessage(&(pShareResource->msgList));
            pthread_mutex_unlock(&(pShareResource->mutex));
        }

        if(pMsg){
            printf("------MyRank:%d, writeThread message:%d\n", pShareResource->myRank, pMsg->type);
//            printMessage(pMsg);

//            sleep(1);
            Packet packet = {pMsg->type, pMsg->size};
            int dataBytes = pMsg->size;
            if(pMsg->type == STEP1_STATISTICS
                    || pMsg->type == STEP2_STATISTICS){
                dataBytes = sizeof(Statistics) * pMsg->size;
            }
            void* buffer = (void*) calloc (sizeof(Packet) + dataBytes, sizeof(char));
            memcpy(buffer, (void*)&packet, sizeof(Packet));
            if(pMsg->data){
                memcpy(buffer + sizeof(Packet), pMsg->data, dataBytes);
                free(pMsg->data);
            }
            free(pMsg);
            MPI_Send(buffer, sizeof(Packet) + dataBytes, MPI_CHAR, pMsg->receiver, 0, MPI_COMM_WORLD);
            free(buffer);
        }else{
            if(pShareResource->keepWriting){
                usleep(1000);
            }else{
                break;
            }
        }
    }
    printf("------MyRank:%d write thread finished\n", pShareResource->myRank);
    pShareResource->isWriting = 0;
    return NULL;
}


int isSessionFinished(int *textFinishedCount, int * staticFinishedCount){
    return (*textFinishedCount == 0 && staticFinishedCount == 0);
}

char* readFile(char* fileName){
    char* buffer = NULL;
    FILE * pFile = fopen (fileName , "r" );

    if (pFile == NULL) {
        perror ("");
    }else{
        // obtain file size:
        fseek (pFile , 0 , SEEK_END);
        size_t lSize = ftell (pFile);
        rewind (pFile);
        // allocate memory to contain the whole file:
        buffer = (char*) calloc (lSize + 1, sizeof(char));
        if (buffer == NULL) {
            perror ("");
        }else{
            // copy the file into the buffer:
            size_t result = fread (buffer, 1, lSize, pFile);

            if (result != lSize) {
                free (buffer);
                buffer = NULL;
                perror ("");
            }
            buffer[lSize] = 0;
            /*else the whole file is now loaded in the memory buffer. */
        }
        fclose (pFile);
    }
    return buffer;
}


void initStatistics(Statistics (*pStatisticsArr)[26]){
    for(int i = 0; i < 26; i++){
        pStatisticsArr[0][i].count = 0;
            pStatisticsArr[0][i].letter = 'A' + i;
    }
}

void printStatistics(Statistics (*pStatisticsArr)[26]){
    for(int i = 0; i < 26; i++){
        printf("%c\t%d\n", pStatisticsArr[0][i].letter, pStatisticsArr[0][i].count);
    }
}
