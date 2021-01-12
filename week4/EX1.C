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
#define N_TASK 5

/*
*********************************************************************************************************
*                                              VARIABLES
*********************************************************************************************************
*/

OS_STK TaskStartStk[TASK_STK_SIZE];
OS_STK TaskStk[N_TASK][TASK_STK_SIZE];
char TaskData[N_TASK];

OS_EVENT randomsem;


OS_EVENT* mbox_to_random[4];
OS_EVENT* mbox_to_decision[4];

OS_EVENT* queue_to_random;
OS_EVENT* queue_to_decision;

void * decision_queue[4];
void * random_queue[4];//random task에서 decision task로 보내는queue의 배열
INT8U select=1; // 메일박스일때는 1 메시지 큐일때는 2

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
		// 총 4개의 mailbox OSMboxCreate 사용해서 생성 (your code)
		mbox_to_random[0] = OSMboxCreate((void*)0);
		mbox_to_random[1] = OSMboxCreate((void*)0);
		mbox_to_random[2] = OSMboxCreate((void*)0);
		mbox_to_random[3] = OSMboxCreate((void*)0);
		mbox_to_decision[0] = OSMboxCreate((void*)0);
		mbox_to_decision[1] = OSMboxCreate((void*)0);
		mbox_to_decision[2] = OSMboxCreate((void*)0);
		mbox_to_decision[3] = OSMboxCreate((void*)0);
	}
	else if (select == 2) {
		// 크기가 4인 queue OSQCreate사용해서 생성 (your code)
		
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

	INT8U push_number;
	int get_number[4];
	
	INT8U i, j;

	INT8U min;		// 최솟값을 담을 변수
	INT8U min_task;	
	int task_number = (int)(*(char*)pdata-48);//각 task의 index이다. pdata는 char타입이기 때문에 ascii 기준 -48을 하면 int형으로 바뀐다.

	char push_letter;//W OR L을 저장 Decision task에서 생성한다.
	char get_letter;//W or L을 저장 decision task에서 만든 문자를 Random task1~4중 하나가 받는다.

	int fgnd_color, bgnd_color;//배경색 random task 1~4가 w or l을 받았을때 화면에 칠해줄 색

	char s[10];//

	//task_number, pdata가 0-3이면 random task, 4이면 decision task
	if (*(char*)pdata == '4') {//decision task일 경우
		for (;;) {
			for (i = 0;  i < N_TASK - 1; i++) {
				if (select == 1) {
					// randomnumber가 넘어올 때 까지 기다림. get_number배열에 값 저장 (your code)
					get_number[i] = *(int*)(OSMboxPend(mbox_to_random[i], 0, &err));
				}
				else if (select == 2) {
					// randomnumber가 넘어올 때 까지 기다림. get_number배열에 값 저장 (your code)
				}
			}
			min = get_number[0];
			min_task = 0;
			for (i = 1; i < N_TASK - 1; i++) {
				//random task가 보낸 4개의 랜덤 숫자중 가장 작은 수 찾기
				if (get_number[i] < min) {
					min = get_number[i];
					min_task = i;
				}
			}
			for (i = 0; i < N_TASK - 1; i++) {
				if (i == min_task) {//가장 작은 task에게는 w를 아닌 것에게는 l를 보낸다.
					push_letter = 'W';
				}
				else {
					push_letter = 'L';
				}
				if (select == 1) {
					// (your code)
					OSMboxPost(mbox_to_decision[i], (void*) &push_letter);
				}
				else if (select == 2) {
					// (your code)
				}
			}
			OSTimeDlyHMSM(0, 0, 5, 0);
		}
	}
	else {
		for (;;) {
			push_number = random(64);
			sprintf(s, "%2d", push_number);

			PC_DispStr(0 + 18 * task_number, 4, "task", DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
			PC_DispChar(4 + 18 * task_number, 4, *(char*)pdata, DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
			PC_DispStr(6 + 18 * task_number, 4, s, DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);

			if (select == 1) {
				// random task에서 decision task로 post 후 pend로 기다림 (your code)
				OSMboxPost(mbox_to_random[task_number], (void*)&push_number);
				get_letter = *(char*)(OSMboxPend(mbox_to_decision[task_number], 0, &err));
			}
			else if (select == 2) {
				// random task에서 decision task로 post 후 pend로 기다림 (your code)
			}
			
			if (*(char*)pdata == '0') {
				bgnd_color = DISP_BGND_RED;
				fgnd_color = DISP_FGND_RED;
			}
			else if (*(char*)pdata == '1') {
				bgnd_color = DISP_BGND_CYAN;
				fgnd_color = DISP_FGND_CYAN;
			}
			else if (*(char*)pdata == '2') {
				bgnd_color = DISP_BGND_BLUE;
				fgnd_color = DISP_FGND_BLUE;
			}
			else if (*(char*)pdata == '3') {
				bgnd_color = DISP_BGND_GREEN;
				fgnd_color = DISP_FGND_GREEN;
			}
			PC_DispStr(8 + 18 * task_number, 4, "[", DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
			PC_DispStr(10 + 18 * task_number, 4, "]", DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
			PC_DispChar(9 + 18 * task_number, 4, get_letter, DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);

			if (get_letter == 'W') {
				for (j = 5; j < 24; j++) {
					for (i = 0; i < 80; i++) {
						PC_DispChar(i, j, ' ', fgnd_color + bgnd_color);
					}
				}
			}
			OSTimeDlyHMSM(0, 0, 5, 0);
		}
	}
}

