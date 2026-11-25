#ifndef _divoom_save_led_h_
#define _divoom_save_led_h_

/*********************************************************
  Function:		divoom_save_led_init
  Description:		 ????????????????????????????????
  Input:			handle       
                           size
  Return:			
************************************************************/

void *divoom_save_led_init(uint8 *data);


/*********************************************************
  Function:		divoom_save_led_save
  Description:		 ????????
  Input:			data       
                           len
  Return:			
************************************************************/

void divoom_save_led_save(void *handle, uint8 *data, uint16 len);


/*********************************************************
  Function:		divoom_save_led_exit
  Description:		???????
  Input:			handle       
                           flag   ???豸?????????У???????
  Return:			
************************************************************/

void divoom_save_led_exit(void * handle);

#endif


