#ifndef _divoom_cyc_buf_h_
#define _divoom_cyc_buf_h_

/**
* NAME         divoom_cyc_buf_init
* @BRIEF	创建环形缓冲区
* @PARAM	buf_size
* @RETURN	返回句柄
* @RETVAL
*/

void *divoom_cyc_buf_init(int buf_size);


/**
* NAME         divoom_cyc_buf_read
* @BRIEF	读缓冲区数据
* @PARAM	handle
            buf
            size
* @RETURN	数据长度
* @RETVAL
*/

int divoom_cyc_buf_read(void *handle, uint8 *buf, int size);

/**
* NAME       divoom_cyc_buf_get_reset_data_size
* @BRIEF	在数据小于最大缓冲时，可以将缓冲复位
* @PARAM	handle
* @RETURN	none
* @RETVAL
*/

int divoom_cyc_buf_get_reset_data_size(void *handle);

/**
* NAME       divoom_cyc_buf_get_clear_data
* @BRIEF	清除所有数据
* @PARAM	handle
* @RETURN	none
* @RETVAL
*/

void divoom_cyc_buf_clear_data(void *handle);

/**
* NAME         divoom_cyc_buf_write
* @BRIEF	写缓冲区数据
* @PARAM	handle
            buf
            size
* @RETURN	数据长度
* @RETVAL
*/

int divoom_cyc_buf_write(void *handle, uint8 *buf, int size);

/**
* NAME         divoom_cyc_buf_get_free_size
* @BRIEF	得到缓冲区剩余空间大小
* @PARAM	handle
* @RETURN	空间大小
* @RETVAL
*/

int divoom_cyc_buf_get_free_size(void *handle);

/**
* NAME         divoom_cyc_buf_get_data_size
* @BRIEF	得到缓冲区有效数据空间大小
* @PARAM	handle
* @RETURN	空间大小
* @RETVAL
*/

int divoom_cyc_buf_get_data_size(void *handle);


/**
* NAME         divoom_cyc_buf_exit
* @BRIEF	释放句柄
* @PARAM	handle
* @RETURN	
* @RETVAL
*/

void divoom_cyc_buf_exit(void *handle);

#endif


