/*
*********************************************************************************************************
*                                                uC/OS-II
*                                          The Real-Time Kernel
*
*                          (c) Copyright 1992-2002, Jean J. Labrosse, Weston, FL
*                                           All Rights Reserved
*
*                                               EXAMPLE #2
*********************************************************************************************************
*/

#include "includes.h"

/*
*********************************************************************************************************
*                                              CONSTANTS
*********************************************************************************************************
*/

#define          TASK_STK_SIZE     512                /* Size of each task's stacks (# of WORDs)       */
#define N_TASK 4
#define MSG_QUEUE_SIZE 3 //size of message queue

/*
*********************************************************************************************************
*                                              VARIABLES
*********************************************************************************************************
*/

OS_STK TaskStartStk[TASK_STK_SIZE];
OS_STK TaskStk[N_TASK][TASK_STK_SIZE];
char TaskData[N_TASK];
OS_EVENT* majority_sem;   // semaphore 변수
OS_EVENT* master_task;   //mailbox
OS_EVENT* queue_to_vote;   //message queue
void* MsgQueueTbl[3];   //message queue storage
OS_FLAG_GRP* vote_grp;   //Event flag
INT8U cnt_O;   //O의 개수를 count하는 공유변수
INT8U err;   // err

  
INT8U select=1;

/*
*********************************************************************************************************
*                                         FUNCTION PROTOTYPES
*********************************************************************************************************
*/

void Task(void * data);
static  void  TaskStart(void *data);                  /* Function prototypes of tasks                  */
static  void  TaskStartCreateTasks(void);
static  void  TaskStartDispInit(void);
static  void  TaskStartDisp(void);
   

/*$PAGE*/
/*
*********************************************************************************************************
*                                                  MAIN
*********************************************************************************************************
*/

void main (void)
{
    OS_STK *ptos;
    OS_STK *pbos;
    INT32U  size;

	PC_DispClrScr(DISP_FGND_WHITE + DISP_BGND_BLACK);                        /* Clear the screen                         */

    OSInit();                                              /* Initialize uC/OS-II                      */

    PC_DOSSaveReturn();                                    /* Save environment to return to DOS        */
    PC_VectSet(uCOS, OSCtxSw);                             /* Install uC/OS-II's context switch vector */

	OSTaskCreate(TaskStart, (void*)0, &TaskStartStk[TASK_STK_SIZE - 1], 0);
   
    OSStart();                                             /* Start multitasking                       */
}
/*$PAGE*/
/*
*********************************************************************************************************
*                                               STARTUP TASK
*********************************************************************************************************
*/

static void  TaskStart (void *pdata)
{
#if OS_CRITICAL_METHOD == 3                                /* Allocate storage for CPU status register */
    OS_CPU_SR  cpu_sr;
#endif
    INT16S     key;
	INT8U i;

    pdata = pdata;                                         /* Prevent compiler warning                 */

    TaskStartDispInit();                                   /* Setup the display                        */

    OS_ENTER_CRITICAL();                                   /* Install uC/OS-II's clock tick ISR        */
    PC_VectSet(0x08, OSTickISR);
    PC_SetTickRate(OS_TICKS_PER_SEC);                      /* Reprogram tick rate                      */
    OS_EXIT_CRITICAL();

    OSStatInit();                                          /* Initialize uC/OS-II's statistics         */

	if (select == 1) {
		majority_sem = OSSemCreate(1);   //create semaphore initialize 1
		vote_grp = OSFlagCreate(0x00, &err);   //create event flag initalize 0x00
		queue_to_vote = OSQCreate(&MsgQueueTbl[0], MSG_QUEUE_SIZE);   //create message queue
		master_task = OSMboxCreate((void*)0);   //create mailbox
	}
	else if (select == 2) {
		
		
	}
  
    TaskStartCreateTasks();                                /* Create all other tasks                   */

    for (;;) {
        TaskStartDisp();                                   /* Update the display                       */

        if (PC_GetKey(&key)) {                             /* See if key has been pressed              */
            if (key == 0x1B) {                             /* Yes, see if it's the ESCAPE key          */
                PC_DOSReturn();                            /* Yes, return to DOS                       */
            }
        }

        OSCtxSwCtr = 0;                                    /* Clear context switch counter             */
        OSTimeDly(OS_TICKS_PER_SEC);                       /* Wait one second                          */
    }
}
/*$PAGE*/
/*
*********************************************************************************************************
*                                        INITIALIZE THE DISPLAY
*********************************************************************************************************
*/

static  void  TaskStartDispInit (void)
{
/*                                1111111111222222222233333333334444444444555555555566666666667777777777 */
/*                      01234567890123456789012345678901234567890123456789012345678901234567890123456789 */
    PC_DispStr( 0,  0, "                         uC/OS-II, The Real-Time Kernel                         ", DISP_FGND_WHITE + DISP_BGND_RED + DISP_BLINK);
    PC_DispStr( 0,  1, "                                Jean J. Labrosse                                ", DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
    PC_DispStr( 0,  2, "                                                                                ", DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
    PC_DispStr( 0,  3, "                                                                                ", DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
    PC_DispStr( 0,  4, "                                                                                ", DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
    PC_DispStr( 0,  5, "                                                                                ", DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
    PC_DispStr( 0,  6, "                                                                                ", DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
    PC_DispStr( 0,  7, "                                                                                ", DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
    PC_DispStr( 0,  8, "                                                                                ", DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
    PC_DispStr( 0,  9, "                                                                                ", DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
    PC_DispStr( 0, 10, "                                                                                ", DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
    PC_DispStr( 0, 11, "                                                                                ", DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
    PC_DispStr( 0, 12, "                                                                                ", DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
    PC_DispStr( 0, 13, "                                                                                ", DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
    PC_DispStr( 0, 14, "                                                                                ", DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
    PC_DispStr( 0, 15, "                                                                                ", DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
    PC_DispStr( 0, 16, "                                                                                ", DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
    PC_DispStr( 0, 17, "                                                                                ", DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
    PC_DispStr( 0, 18, "                                                                                ", DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
    PC_DispStr( 0, 19, "                                                                                ", DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
    PC_DispStr( 0, 20, "                                                                                ", DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
    PC_DispStr( 0, 21, "                                                                                ", DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
    PC_DispStr( 0, 22, "                                                                                ", DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
    PC_DispStr( 0, 23, "                                                                                ", DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
    PC_DispStr( 0, 24, "                            <-PRESS 'ESC' TO QUIT->                             ", DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY + DISP_BLINK);
/*                                1111111111222222222233333333334444444444555555555566666666667777777777 */
/*                      01234567890123456789012345678901234567890123456789012345678901234567890123456789 */
}

/*$PAGE*/
/*
*********************************************************************************************************
*                                           UPDATE THE DISPLAY
*********************************************************************************************************
*/

static  void  TaskStartDisp (void)
{
    char   s[80];


    sprintf(s, "%5d", OSTaskCtr);                                  /* Display #tasks running               */
    PC_DispStr(18, 22, s, DISP_FGND_YELLOW + DISP_BGND_BLUE);

    sprintf(s, "%3d", OSCPUUsage);                                 /* Display CPU usage in %               */
    PC_DispStr(36, 22, s, DISP_FGND_YELLOW + DISP_BGND_BLUE);

    sprintf(s, "%5d", OSCtxSwCtr);                                 /* Display #context switches per second */
    PC_DispStr(18, 23, s, DISP_FGND_YELLOW + DISP_BGND_BLUE);

    sprintf(s, "V%4.2f", (float)OSVersion() * 0.01);               /* Display uC/OS-II's version number    */
    PC_DispStr(75, 24, s, DISP_FGND_YELLOW + DISP_BGND_BLUE);

    switch (_8087) {                                               /* Display whether FPU present          */
        case 0:
             PC_DispStr(71, 22, " NO  FPU ", DISP_FGND_YELLOW + DISP_BGND_BLUE);
             break;

        case 1:
             PC_DispStr(71, 22, " 8087 FPU", DISP_FGND_YELLOW + DISP_BGND_BLUE);
             break;

        case 2:
             PC_DispStr(71, 22, "80287 FPU", DISP_FGND_YELLOW + DISP_BGND_BLUE);
             break;

        case 3:
             PC_DispStr(71, 22, "80387 FPU", DISP_FGND_YELLOW + DISP_BGND_BLUE);
             break;
    }
}

/*$PAGE*/
/*
*********************************************************************************************************
*                                             CREATE TASKS
*********************************************************************************************************
*/

static  void  TaskStartCreateTasks (void)
{
	INT8U i;

	for (i = 0; i < N_TASK; i++) {
		TaskData[i] = '0' + i;
		OSTaskCreate(Task, (void*)&TaskData[i], &TaskStk[i][TASK_STK_SIZE - 1], i + 1);
	}
}

void Task(void * pdata) {
	INT8U err;

	INT8U optOX;   //voting task에서 O,X를 선택하기 위한 random 변수  짝수이면 O, 홀수이면 X
	INT8U master_task_num;   //master task의 number
	INT8U random_master;   //master task의 number를 구하기 위한 random 변수
	
	INT8U i, j;
	char msg[3];   //O, X인지 받아오기 위해 만든 변수
	char tmpOX;   //O, X를 잠시 저장해놓을 임시 변수
	
	int task_number = (int)(*(char*)pdata-48);//각 task의 index이다. pdata는 char타입이기 때문에 ascii 기준 -48을 하면 int형으로 바뀐다.
	int fgnd_color, bgnd_color;//배경색 O의 개수가 더 많으면 blue, X가 더 많으면 red
	char s[10];

	//task_number, pdata가 0-2이면 random task, 3이면 decision task
	if (*(char*)pdata == '3') {//decision task일 경우
		for (;;) {
			random_master = random(3);    //master task의 number를 random으로 선택
			OSMboxPostOpt(master_task, (void*)&random_master, OS_POST_OPT_BROADCAST);   // master task number를 mailbox로 전달
			OSFlagPend(vote_grp, 0x01, OS_FLAG_WAIT_SET_ALL + OS_FLAG_CONSUME, 0, &err);   //voting task(master task)로 부터 majority 받음 
			OSSemPend(majority_sem, 0, &err);   //공유변수에 접근하므로 semaphore로 보호
			if (cnt_O > 1) {
				//blue
				bgnd_color = DISP_BGND_BLUE;
				fgnd_color = DISP_FGND_BLUE;
			}
			else {
				//red
				bgnd_color = DISP_BGND_RED;
				fgnd_color = DISP_FGND_RED;
			}
			for (j = 5; j < 24; j++) {
				for (i = 0; i < 80; i++) {
					PC_DispChar(i, j, ' ', fgnd_color + bgnd_color);
				}
			}
			cnt_O = 0;
			OSSemPost(majority_sem);
			OSTimeDlyHMSM(0, 0, 5, 0);
		}
	}
	//Vote task
	else {
		for (;;) {
			optOX = random(100);   //짝수이면 O, 홀수이면 X
			if (optOX % 2 == 0) {
				tmpOX = 'O';
				OSQPost(queue_to_vote, (void *)&tmpOX);   //message queue에 O,X 전달
			}
			else {
				tmpOX = 'X';
				OSQPost(queue_to_vote, (void *)&tmpOX);   //message queue에 O,X 전달
			}
			master_task_num = *(int*)(OSMboxPend(master_task, 0, &err));   //master task가 누구인지 받아오기
			if (task_number == master_task_num) {
				for (i = 0; i < N_TASK - 1; i++) {
					msg[i] = *(char*)OSQPend(queue_to_vote, 0, &err);   //message queue에서 각 vote task에서 O, X를 받아옴
					PC_DispChar(9 + 18 * i, 4, msg[i], DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
					OSSemPend(majority_sem, 0, &err);   //공유변수에 접근하므로 semaphore로 보호
					if (msg[i] == 'O')
						cnt_O++;
					OSSemPost(majority_sem);
				}
				OSFlagPost(vote_grp, 0x01, OS_FLAG_SET, &err);   //majority를 다 구했기 때문에 event flag로 완료 알림
			}
			//OSTimeDlyHMSM(0, 0, 5, 0);
		}
	}
}

