#ifndef _divoom_load_message_h_
#define _divoom_load_message_h_

/**
 * NAME         divoom_load_message_init_lang
 * @BRIEF	开机初始化
 * @PARAM	lang : 语言类型
                    
 * @RETURN	NONE
 * @RETVAL	
 */

void divoom_load_message_init_lang(uint8 lang);

/**
 * NAME         divoom_load_message_set_lang
 * @BRIEF	切换语言，加载所有的相关信息
 * @PARAM	lang 
                    
 * @RETURN	NONE
 * @RETVAL	
 */

void divoom_load_message_set_lang(uint8 lang);

/**
 * NAME         divoom_load_message_get_value
 * @BRIEF	获取某个键的显示内容
 * @PARAM	key 
                    
 * @RETURN	NONE
 * @RETVAL	
 */

char *divoom_load_message_get_value(char *key);

#endif


