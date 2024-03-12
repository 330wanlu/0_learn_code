#ifndef _TASK_PARSER_H
#define _TASK_PARSER_H

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
//#include <unistd.h>
#include "FreeRTOS.h"
#include "queue.h"

#define TASK_MAX_NUM 1



typedef struct TagTaskState
{
	int success_flag;
	int error_code;
	struct TagTaskState *next;
}TaskState;

extern TaskState task_state[TASK_MAX_NUM];

typedef int (*FunAction)(int args);

typedef struct TagTaskParserNodeParm
{
	FunAction fun;
	int args;
}TaskParserNodeParm;

typedef enum TagErrorCode{
  ERROR_NO = 0,
	Error_1,
	Error_2,
	Error_3,
	Error_4,
	Error_5,
	Error_6,
	Error_7,
	Error_8,
	Error_9,
	Error_10,
	Error_11,
	Error_12,
	Error_13,
	Error_14,
	Error_15,
	Error_16,
	Error_17,
	Error_18,
	Error_19,
	Error_20,
	Error_21,
	Error_22,
	Error_23,
	Error_24,
	Error_25,
	Error_26,
	Error_27,
	Error_28,
	Error_29,
	Error_30,
	QI_YA_ERROR,
	CI_PIN_GUO_DUO
}ErrorCode;

typedef struct TagTaskParserParm
{
	TaskParserNodeParm execution;
	TaskParserNodeParm judge;
	TaskParserNodeParm failed_execution;
	TaskParserNodeParm failed_judge;
	int cur_index;
	int jump_index;
	int retry_num;
	int interval;
	int failed_retry_num;
	int failed_interval;
	ErrorCode error_code;
}TaskParserParm;

typedef struct TagTaskParser
{
	int cur_index;
	int jump_index;
	int retry_num;
	int interval;
	int failed_retry_num;
	int failed_interval;
	ErrorCode error_code;
	TaskParserNodeParm execution;
	TaskParserNodeParm judge;
	TaskParserNodeParm failed_execution;
	TaskParserNodeParm failed_judge;
	//TaskState *task_state;
	struct TagTaskParser* jump_to_step_ptr;
	struct TagTaskParser* succeed_act_ptr;
}TaskParser;

TaskParser* task_parser_new(TaskParserParm *task_parser_parm,int task_num);
int task_parser_delete(TaskParser *task_parser);
int task_parser_start(TaskParser *task_parser);

#endif //_TASK_PARSER_H
