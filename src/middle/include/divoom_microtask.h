#ifndef __divoom_microtask_h
#define __divoom_microtask_h

typedef void DIVOOM_MICROTASK_CALLBACK(void *data);

void divoom_microtask_resume(uint8 task_id);
void divoom_microtask_update(uint8 task_id, uint32 time);
void divoom_microtask_pause(uint8 task_id);
uint8 divoom_microtask_create(DIVOOM_MICROTASK_CALLBACK *pcallback, uint32 time);
uint8 divoom_microtask_create_para(DIVOOM_MICROTASK_CALLBACK *pcallback, uint32 time, void *para);
void divoom_microtask_destroy(uint8 task_id);
void divoom_microtask_init(void);


#endif



