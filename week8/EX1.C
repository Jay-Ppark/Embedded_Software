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
#define MSG_QUEUE_SIZE 3 //size of message queue

/*
*********************************************************************************************************
*                                              VARIABLES
*********************************************************************************************************
*/

OS_STK TaskStartStk[TASK_STK_SIZE];
OS_STK TaskStk[N_TASK][TASK_STK_SIZE];
char TaskData[N_TASK];
// semaphore 변수
OS_EVENT* num_generation_sem;
//mailbox word를 주고 받음
OS_EVENT* comparison_word;
//Event flag
OS_FLAG_GRP* display_end_grp;   // 화면을 모두 칠했다는 것에 대한 event flag
OS_FLAG_GRP* calculate_grp;		// random수를 다 선택해서 계산을 하라는 것에 대한 event flag
OS_FLAG_GRP* calculate_end_grp;		// 최대값을 다 계산해서 화면을 칠해도 된다는 것에 대한 event flag
//공유변수
int generated_num[3];
// err
INT8U err;

  
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
		//create semaphore initialize 1
		num_generation_sem = OSSemCreate(1);
		//create event flag initalize 0x00
		calculate_end_grp = OSFlagCreate(0x00, &err);
		display_end_grp = OSFlagCreate(0x00, &err);
		calculate_grp = OSFlagCreate(0x00, &err);
		//create mailbox
		comparison_word = OSMboxCreate((void*)0);
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

	INT8U rannum;
	
	INT8U i, j;
	char get_letter;	// 어떤 task가 가장 큰 수를 가지고 있는지 알려주는 변수 (R,G,B)
	int max_num;		// 최대값을 구하는 변수
	char tmp_letter;	// 색깔을 칠하기 위해서 어떤 문자를 가지고 있는지 나타내기 위한 임시변수
	int tmpindex;		// 최대값을 구할때 어떤 task가 가장 큰수를 가지고 있는지 나타내기 위한 임시변수
	char rannum_to_char;		//int형을 char형으로 출력하기 위해 형변환하려 가지고 있는 임시 변수
	
	int task_number = (int)(*(char*)pdata-48);//각 task의 index이다. pdata는 char타입이기 때문에 ascii 기준 -48을 하면 int형으로 바뀐다.
	int fgnd_color, bgnd_color;//배경색 random task 1~4가 w or l을 받았을때 화면에 칠해줄 색
	char s[10];

	//display task일 경우
	if (*(char*)pdata == '0') {
		for (;;) {
			OSFlagPend(calculate_end_grp, 0x01, OS_FLAG_WAIT_SET_ALL + OS_FLAG_CONSUME, 0, &err);	//최대값 계산이 끝날때까지 wait
			get_letter = *(char*)OSMboxPend(comparison_word, 0, &err);		//어떤 task가 최대값을 가지고 있는지 mailbox를 통해 전달 받음
			if (get_letter == 'R') {
				//red
				bgnd_color = DISP_BGND_RED;
				fgnd_color = DISP_FGND_RED;
			}
			else if (get_letter == 'B') {
				//blue
				bgnd_color = DISP_BGND_BLUE;
				fgnd_color = DISP_FGND_BLUE;
			}
			else {
				//green
				bgnd_color = DISP_BGND_GREEN;
				fgnd_color = DISP_FGND_GREEN;
			}
			for (j = 5; j < 24; j++) {
				for (i = 0; i < 80; i++) {
					PC_DispChar(i, j, ' ', fgnd_color + bgnd_color);
				}
			}
			OSTimeDlyHMSM(0, 0, 3, 0);		//delay
			OSFlagPost(display_end_grp, 0x0F, OS_FLAG_SET, &err);		//색을 다 칠했기 때문에 특정비트 set
		}
	}
	//Comparison task
	else if (*(char*)pdata == '1') {
		for (;;) {
			OSFlagPend(calculate_grp, 0x07, OS_FLAG_WAIT_SET_ALL + OS_FLAG_CONSUME, 0, &err);		//3개의 task에서 숫자를 모두 받아올때 까지 wait
			OSSemPend(num_generation_sem, 0, &err);		//공유변수를 사용하기 때문에 semaphore로 보호
			max_num = generated_num[0];
			tmpindex = 0;
			for (i = 1; i < 3; i++) {
				if (max_num < generated_num[i]) {
					max_num = generated_num[i];
					tmpindex = i;
				}
				else if (max_num == generated_num[i])
					tmpindex = 0;
			}
			OSSemPost(num_generation_sem);		// 공우변수 사용을 끝냈기 때문에 semaphore 변수 증가
			if (tmpindex == 0)
				tmp_letter = 'R';
			else if (tmpindex == 1)
				tmp_letter = 'B';
			else
				tmp_letter = 'G';
			OSMboxPost(comparison_word, (void*)&tmp_letter);		//최대값을 나타내는 task에 대한 문자를 전달
			OSFlagPost(calculate_end_grp, (1 << (task_number - 1)), OS_FLAG_SET, &err);		//계산이 끝났다는 것을 알림
			OSFlagPend(display_end_grp, (1 << (task_number - 1)), OS_FLAG_WAIT_SET_ALL + OS_FLAG_CONSUME, 0,&err);	//색칠이 끝날때 까지 wait
		}
	}
	//rannum task
	else {
		for (;;) {
			rannum = random(64);
			OSSemPend(num_generation_sem, 0, &err);		//공유변수에 접근하기 떄문에 semaphore를 이용해 보호
			generated_num[task_number-2] = rannum;
			sprintf(s, "%2d", rannum);
			PC_DispStr(9 + 18 * (task_number - 2), 4, s, DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
			OSSemPost(num_generation_sem);		//공유변수 사용을 끝냈기 때문에 semaphore 변수 증가
			OSFlagPost(calculate_grp, (1 << (task_number - 2)), OS_FLAG_SET, &err);		//random num을 다 골랐기 때문에 해당 tasknumber-2에 위치한 비트를 set(계산의 편의를 위해서 -2)
			OSFlagPend(display_end_grp, (1 << (task_number - 1)),OS_FLAG_WAIT_SET_ALL + OS_FLAG_CONSUME, 0, &err);	//색칠이 다끝날때까지 wait
			}
			//OSTimeDlyHMSM(0, 0, 5, 0);
		}
	}


