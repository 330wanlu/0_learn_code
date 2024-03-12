#include "task_parser.h"

TaskState task_state[TASK_MAX_NUM] = {0x00};

TaskParser* jump_to_node_with_index(TaskParser *task_parser,int index)
{
	int i = 0;
	TaskParser *tmp_task_parser = NULL;
	if(NULL == task_parser || 0 > index)
	{
		return NULL;
	}
	tmp_task_parser = task_parser;
	for(i = 0;i<index;++i)
	{
		tmp_task_parser = tmp_task_parser->succeed_act_ptr;
	}
	return tmp_task_parser;
}

TaskParser* jump_to_last_node(TaskParser *task_parser)
{
	TaskParser *tmp_task_parser = NULL;
	if(NULL == task_parser)
	{
		return NULL;
	}
	tmp_task_parser = task_parser;
	while(NULL != tmp_task_parser->succeed_act_ptr)
	{
		tmp_task_parser = tmp_task_parser->succeed_act_ptr;
		if((int)tmp_task_parser->succeed_act_ptr==0xffffffff)
		{
			break;
		}
	}
	return tmp_task_parser;
}

//task_parser_parm最后一个节点为回到初始状态
TaskParser* task_parser_new(TaskParserParm *task_parser_parm,int task_num)
{
	static int task_total_num = 0;
		int i = 0;
	TaskParser *task_parser = NULL;
	TaskParser *tmp_task_parser = NULL;
	if(0 >= task_num || NULL == task_parser_parm)
	{
		return NULL;
	}
	task_parser = (TaskParser*)malloc(sizeof(TaskParser));
	if(NULL == task_parser)
	{
		return NULL;
	}
	task_parser->succeed_act_ptr = NULL;
	task_parser->jump_to_step_ptr = NULL;
	tmp_task_parser = task_parser;

	for(i = 0;i<task_num;++i)
	{
		tmp_task_parser->execution = (task_parser_parm + i)->execution;
		tmp_task_parser->judge = (task_parser_parm + i)->judge;
		tmp_task_parser->failed_execution = (task_parser_parm + i)->failed_execution;
		tmp_task_parser->failed_judge = (task_parser_parm + i)->failed_judge;
		tmp_task_parser->cur_index = (task_parser_parm + i)->cur_index;
		tmp_task_parser->jump_index = (task_parser_parm + i)->jump_index;
		tmp_task_parser->retry_num = (task_parser_parm + i)->retry_num;
		tmp_task_parser->interval = (task_parser_parm + i)->interval;
		tmp_task_parser->failed_retry_num = (task_parser_parm + i)->failed_retry_num;
		tmp_task_parser->failed_interval = (task_parser_parm + i)->failed_interval;
		tmp_task_parser->error_code = (task_parser_parm + i)->error_code;
		//if(task_total_num >=  TASK_MAX_NUM)
		//	task_total_num = TASK_MAX_NUM - 1;
		//tmp_task_parser->task_state = &task_state[task_total_num];
		if(task_num > (i + 1))
		{
			tmp_task_parser->succeed_act_ptr = (TaskParser*)malloc(sizeof(TaskParser));
			tmp_task_parser = tmp_task_parser->succeed_act_ptr;
			tmp_task_parser->succeed_act_ptr = NULL;
		}
	}
	tmp_task_parser = task_parser;
	for(i = 0;i<task_num;++i)
	{
		if(0 <= tmp_task_parser->jump_index)
		{
			//printf("i = %d tmp_task_parser->jump_index = %d\n",i,tmp_task_parser->jump_index);
			tmp_task_parser->jump_to_step_ptr = jump_to_node_with_index(task_parser,tmp_task_parser->jump_index);
		}
		else
		{
			tmp_task_parser->jump_to_step_ptr = NULL;
		}
		tmp_task_parser = tmp_task_parser->succeed_act_ptr;
	}
	return task_parser;
}

int task_parser_delete(TaskParser *task_parser)
{
	TaskParser *tmp_task_parser = task_parser;
	TaskParser *tmp_tmp_task_parser = NULL;
	while(NULL != tmp_task_parser)
	{
		tmp_tmp_task_parser = tmp_task_parser;
		tmp_task_parser = tmp_task_parser->succeed_act_ptr;
		free(tmp_tmp_task_parser);
	}
	task_parser = NULL;
	return 0;
}

int task_parser_start(TaskParser *task_parser)
{
	int try_count = 0,failed_try_count = 0;
	TaskParser *tmp_task_parser = NULL;
	TaskParser *last_step = NULL;
	if(NULL == task_parser)
	{
		return -1;
	}
	tmp_task_parser = task_parser;
	
	while(NULL != tmp_task_parser)
	{
		try_count = 0;
		failed_try_count = 0;
		if(NULL != tmp_task_parser->execution.fun)
			tmp_task_parser->execution.fun(tmp_task_parser->execution.args);
		if(NULL == tmp_task_parser->judge.fun)
		{
			tmp_task_parser = tmp_task_parser->succeed_act_ptr;
		}
		else
		{
			if(0 == tmp_task_parser->judge.fun(tmp_task_parser->judge.args))
			{
				tmp_task_parser = tmp_task_parser->succeed_act_ptr;
			}
			else//判断条件执行失败
			{
				while(1)
				{
					vTaskDelay(tmp_task_parser->interval);//
					try_count++;
					if(try_count >= tmp_task_parser->retry_num)//等待时间里判断条件执行失败，执行失败措施 
					{
						//tmp_task_parser->task_state->error_code = tmp_task_parser->error_code;
						if(NULL == tmp_task_parser->failed_execution.fun)//如果没有失败措施，执行跳转或者复位
						{
							if(NULL != tmp_task_parser->jump_to_step_ptr)
							{
								tmp_task_parser = tmp_task_parser->jump_to_step_ptr;
								break;
							}
							else
							{
								return tmp_task_parser->error_code;
							}
							//else if(NULL != (last_step = jump_to_last_node(task_parser)))//有复位则执行复位操作
							//{
							//	if(NULL != last_step->execution.fun)
							//		last_step->execution.fun(last_step->execution.args);
							//	return -1;
							//}
						}
						else
						{
							if(NULL != tmp_task_parser->failed_execution.fun)
								tmp_task_parser->failed_execution.fun(tmp_task_parser->failed_execution.args);
							if(NULL == tmp_task_parser->failed_judge.fun)
							{
								tmp_task_parser = tmp_task_parser->succeed_act_ptr;
								break;
							}
							else
							{
								if(0 == tmp_task_parser->failed_judge.fun(tmp_task_parser->failed_judge.args))
								{
									tmp_task_parser = tmp_task_parser->succeed_act_ptr;
									break;
								}
								else
								{
									while(1)
									{
										vTaskDelay(tmp_task_parser->failed_interval);//
										failed_try_count++;
										if(failed_try_count >= tmp_task_parser->failed_retry_num)//等待时间里判断条件执行失败，执行跳转或者复位  
										{
											if(NULL != tmp_task_parser->jump_to_step_ptr)
											{
												tmp_task_parser = tmp_task_parser->jump_to_step_ptr;
												break;
											}
											else
											{
												return tmp_task_parser->error_code;
											}
											//else if(NULL != (last_step = jump_to_last_node(task_parser)))//有复位则执行复位操作
											//{
											//	last_step->execution.fun(last_step->execution.args);
											//	return -1;
											//}
										}
										if(0 == tmp_task_parser->failed_judge.fun(tmp_task_parser->failed_judge.args))
										{
											tmp_task_parser = tmp_task_parser->succeed_act_ptr;
											break;
										}
									}
									break;
								}
							}
						}
					}
					if(0 == tmp_task_parser->judge.fun(tmp_task_parser->judge.args))
					{
						tmp_task_parser = tmp_task_parser->succeed_act_ptr;
						break;
					}
				}
			}
		}
	}
	return 0;
}

#if 1
int fun_action1(void *data)
{
	char *tmpdata = (char*)data;
	printf("fun_action1:%s\n",(char*)tmpdata);
	return 0;
}
int fun_judge1(void *data)
{
	char *tmpdata = (char*)data;
	printf("fun_judge1:%s\n",(char*)tmpdata);
	return 0;
}
int fun_failed_action1(void *data)
{
	char *tmpdata = (char*)data;
	printf("fun_failed_action1:%s\n",(char*)tmpdata);
	return 0;
}
int fun_failed_judge1(void *data)
{
	char *tmpdata = (char*)data;
	printf("fun_failed_judge1:%s\n",(char*)tmpdata);
	return 0;
}

int fun_action2(void *data)
{
	char *tmpdata = (char*)data;
	printf("fun_action2:%s\n",(char*)tmpdata);
	return 0;
}
int fun_judge2(void *data)
{
	char *tmpdata = (char*)data;
	printf("fun_judge2:%s\n",(char*)tmpdata);
	return 0;
}
int fun_failed_action2(void *data)
{
	char *tmpdata = (char*)data;
	printf("fun_failed_action2:%s\n",(char*)tmpdata);
	return 0;
}
int fun_failed_judge2(void *data)
{
	char *tmpdata = (char*)data;
	printf("fun_failed_judge2:%s\n",(char*)tmpdata);
	return 0;
}

int fun_action3(void *data)
{
	char *tmpdata = (char*)data;
	printf("fun_action3:%s\n",(char*)tmpdata);
	return 0;
}
int fun_judge3(void *data)
{
	char *tmpdata = (char*)data;
	printf("fun_judge3:%s\n",(char*)tmpdata);
	return 1;
}
int fun_failed_action3(void *data)
{
	char *tmpdata = (char*)data;
	printf("fun_failed_action3:%s\n",(char*)tmpdata);
	return 0;
}
int fun_failed_judge3(void *data)
{
	char *tmpdata = (char*)data;
	printf("fun_failed_judge3:%s\n",(char*)tmpdata);
	return 1;
}

TaskParserParm task_parser_array[] = {
	{fun_action1,"111",NULL,"111",fun_failed_action1,"111",fun_failed_judge1,"111",0,-1,3,10,3,10},
	{fun_action2,"222",NULL,"222",fun_failed_action2,"222",fun_failed_judge2,"222",1,-1,3,10,3,10},
	{fun_action3,"333",fun_judge3,"333",fun_failed_action3,"333",fun_failed_judge3,"333",2,-1,3,10,3,10}
};

int main(void)
{
	TaskParser *task_parser = task_parser_new(&task_parser_array[0],3);
	task_parser_start(task_parser);
	task_parser_delete(task_parser);
	return 0;
}

#endif











