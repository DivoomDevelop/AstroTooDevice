#include "divoom.h"
#include "divoom_port_fs.h"
#include "sys/stat.h"
#include <sys/select.h>
#include <fcntl.h>
#include <unistd.h>
#ifndef DIVOOM_SIMULATOR
#include <mbedtls/sha1.h>
#endif



typedef struct _divoom_app_com_mode_file
{
	int disp_mode;
	char file_name[128];
}divoom_app_com_mode_file, *Pdivoom_app_com_mode_file;

const divoom_app_com_mode_file pdivoom_app_com_mode_file_table[] =
{
	{DIVOOM_DISP_POWER_ON, "/spiffs/poweron.bin"},
};

const char *divoom_app_com_from_mode_get_file_name(int mode)
{
	int i = 0;

	for(i = 0; i < sizeof(pdivoom_app_com_mode_file_table)/sizeof(pdivoom_app_com_mode_file_table[0]); i++)
	{
		if(pdivoom_app_com_mode_file_table[i].disp_mode == mode)
			return pdivoom_app_com_mode_file_table[i].file_name;
	}

    divoom_print("%s: it fails to get the data file!\n",__FUNCTION__);
	return NULL;
}

void divoom_app_com_strcpy(char *dest_str, char *src_str, int len)
{    
    if(dest_str == NULL || src_str == NULL)
    {
        return;
    }
    
    if(strlen(src_str) > len - 1)
    {
        memcpy(dest_str, src_str, len -1);
        dest_str[len - 1] = 0;
    }
    else
    {
        strcpy(dest_str,src_str);
    }
}  

//得到文件的后缀名
void divoom_app_com_get_file_ex_name(char *file_name, char *ex_name)
{    
    int len;
    if(file_name == NULL || ex_name == NULL)
    {
        return;
    }
    len = strlen(file_name)-1;
    while(len > 0 && file_name[len] != '.')
    {
        len --;
    }
    if(len == 0)
    {
        ex_name[0] = 0;
    }
    else
    {
        strcpy(ex_name, &file_name[len+1]);
    }
}  


char * divoom_app_com_copy_string(char *src_str, int len)
{
    char *dest_str;
    size_t src_len;
    size_t alloc_len;
    size_t copy_len;

    if(src_str == NULL)
    {
        return NULL;
    }
    /* Keep allocation and copy based on one source-length observation. */
    src_len = strlen(src_str);
    if(src_len == 0)
    {
        return NULL;
    }
    if(len <= 0)
    {
        alloc_len = src_len + 1;
        copy_len = src_len;
    }
    else
    {
        alloc_len = (size_t)len + 1;
        copy_len = (src_len <= (size_t)len) ? src_len : (size_t)len - 1;
    }
    dest_str = divoom_system_malloc_no_zero(alloc_len);
    if(dest_str == NULL)
    {
        return NULL;
    }
    if(copy_len > 0)
    {
        memcpy(dest_str, src_str, copy_len);
    }
    dest_str[copy_len] = '\0';
    return dest_str;
}

void divoom_app_com_write_movie_file(char *file_name, char *file_buf, int file_len, void *tol_size, int head_len)
{
	void *divoom_file_handle;
    char buf[DIVOOM_PAGE_SIZE];
	if(file_len == 0 || file_buf == NULL || file_name == NULL || tol_size == NULL)
	{
		return;
	}
	divoom_file_handle = divoom_port_file_open(file_name, "w");
	if(divoom_file_handle == NULL)
	{
		divoom_print("divoom_com_write_file: it fails to open the file: %s\n",file_name);
		return;
	}
    memset(buf, 0, sizeof(buf));
    memcpy(buf, tol_size, head_len);
	divoom_port_file_write(buf, DIVOOM_PAGE_SIZE, 1, divoom_file_handle);
	divoom_port_file_write(file_buf, file_len, 1, divoom_file_handle);
	divoom_port_file_close(divoom_file_handle);
}

void divoom_app_com_write_file(char *file_name, uint8 *file_buf, uint32 file_len)
{
	void *divoom_file_handle;
	int len = 0, read_len = 0;
	
	if(file_len == 0 || file_buf == NULL || file_name == NULL)
	{
		divoom_print("divoom_com_write_file data is NULL");
		return;
	}
	divoom_file_handle = divoom_port_file_open(file_name, "wb");
	if(divoom_file_handle == NULL)
	{
		divoom_print("divoom_com_write_file: it fails to open the file: %s\n",file_name);
		return;
	}

	while (file_len >= 1*1024)
	{
		len = (int)divoom_port_file_write(&file_buf[read_len], 1, 1*1024, divoom_file_handle);
		file_len -= len;
		read_len += len;
	}
	divoom_port_file_write(&file_buf[read_len], 1, file_len, divoom_file_handle);
	divoom_port_file_close(divoom_file_handle);
}

char *divoom_app_com_read_file(char *file_name, int *file_len)
{
    return divoom_port_file_read_all(file_name, file_len);
}

//返回文件大小
int divoom_app_com_get_file_size(char *file_name)
{
    return (int)divoom_port_file_size(file_name);
}

int divoom_app_com_close_file(void *fstream)
{
	return divoom_port_file_close(fstream);
}

int divoom_app_com_check_check_sum(void *file_handle)
{
    uint32 check_sum = 0, file_size, read_size, file_check_sum = -1;
    uint8 *file_buf;
    
    //先对文件进行CHECK_SUM
    file_buf = divoom_system_malloc(4096);
    if(file_buf == NULL)
    {
        divoom_print("the clock data is malloc_fail!\n");
        return 0;
    }
    divoom_port_file_seek(file_handle, 0, DIVOOM_PORT_SEEK_END);
    file_size = (uint32)divoom_port_file_tell(file_handle);
    divoom_port_file_seek(file_handle, 0, DIVOOM_PORT_SEEK_SET);  
    if(file_size > 4)
    {
        file_size -= 4;
        while(file_size > 0)
        {
            if(file_size > 4096)
            {
                read_size = (uint32)divoom_port_file_read(file_buf, 1, 4096, file_handle);
            }
            else
            {
                read_size = (uint32)divoom_port_file_read(file_buf, 1, file_size, file_handle);
            }
            if(read_size == 0)
            {
                break;
            }
            check_sum += divoom_app_com_get_check_sum(file_buf, read_size);
            file_size -= read_size;
        }
        if(0 == divoom_port_file_read(&file_check_sum, 1, sizeof(uint32), file_handle))
        {
            file_check_sum = 0;
        }
    }
    
    divoom_port_file_seek(file_handle, 0, DIVOOM_PORT_SEEK_SET);  
    divoom_system_free(file_buf);
    if(check_sum != file_check_sum)
    {
        divoom_print("err check sum at; %x, %x\n", check_sum, file_check_sum);
    }
    return check_sum == file_check_sum;
}

uint32 divoom_app_com_get_file_check_sum(void *file_handle, int *ret_size)
{
    uint32 check_sum = 0, file_size, read_size;
    uint8 *file_buf;
    
    //先对文件进行CHECK_SUM
    file_buf = divoom_system_malloc(4096);
    if(file_buf == NULL)
    {
        divoom_print("the clock data is malloc_fail!\n");
        return 0;
    }
    divoom_port_file_seek(file_handle, 0, DIVOOM_PORT_SEEK_END);
    file_size = (uint32)divoom_port_file_tell(file_handle);
    divoom_port_file_seek(file_handle, 0, DIVOOM_PORT_SEEK_SET);
    *ret_size = (int)file_size;
    while(file_size > 0)
    {
        if(file_size > 4096)
        {
            read_size = (uint32)divoom_port_file_read(file_buf, 1, 4096, file_handle);
        }
        else
        {
            read_size = (uint32)divoom_port_file_read(file_buf, 1, file_size, file_handle);
        }
        if(read_size == 0)
        {
            break;
        }
        check_sum += divoom_app_com_get_check_sum(file_buf, read_size);
        file_size -= read_size;
    }
    divoom_port_file_seek(file_handle, 0, DIVOOM_PORT_SEEK_SET);
    divoom_system_free(file_buf);
    return check_sum;
}

int divoom_app_com_check_file_exist(char  *dir_full_path)
{    
    if(strlen(dir_full_path) == 0)
    {
        return 0;
    }
    return divoom_port_file_exists(dir_full_path);
}

//确认这个文件的所有目录已经创建
int divoom_app_com_check_and_mk_dir(char  *dir_full_path)
{    
    int len;
    char *file_name, *find_ptr=dir_full_path;

    if(!dir_full_path || !dir_full_path[0])
        return -1;
    file_name = divoom_system_malloc(strlen(dir_full_path) + 1);
    if(!file_name)
        return -1;
    find_ptr++;
    while(1)
    {
        find_ptr = strchr(find_ptr, '/');
        if(find_ptr == NULL)
        {
            break;
        }
        len = (int)(find_ptr - dir_full_path);
        memcpy(file_name, dir_full_path, len);
        file_name[len] = 0;
        divoom_app_com_mk_dir(file_name);
        find_ptr ++;
    }
	/* 路径无末尾 '/' 时，上面循环不会创建最后一级目录（如 .../35） */
	divoom_app_com_mk_dir(dir_full_path);
    divoom_system_free(file_name);
    return 0;
}

//recursively delete all the file in the directory.
int divoom_app_com_mk_dir(char  *dir_full_path)
{
    return divoom_port_mkdir_p(dir_full_path);
}

int divoom_app_com_del_dir(char  *dir_full_path)
{    
    return divoom_port_remove_tree(dir_full_path);
}

#ifndef DIVOOM_SIMULATOR
int exec_command_system(const char *cmd)
{
    divoom_exec_cmd((int8 *)cmd, NULL, 0);
}

/**
 * 在后台执行命令，不等待输出、不超时结束（用于 arecord|aplay 等长驻管道）。
 * 内部 fork 后子进程 setsid + execl("/bin/sh","-c",cmd)，父进程立即返回。
 */
int exec_command_system_background(const char *cmd)
{
    #if 0
    pid_t pid;
    if (!cmd || !*cmd)
        return -1;
    pid = fork();
    if (pid < 0)
        return -1;
    if (pid == 0) {
        setsid();
        execl("/bin/sh", "sh", "-c", cmd, (char *)NULL);
        _exit(127);
    }
    #endif
    return 0;
}
#endif /* !DIVOOM_SIMULATOR */

/**
 * @brief  执行command，通过result返回结果，需要与现在result分配好空间
 * @author zhangyu
 * @version 
 * @date 2015-09-08
 */
#define DIVOOM_EXEC_CMD_TIMEOUT_SEC  2  // 超时时间（秒）

#ifndef DIVOOM_SIMULATOR
void divoom_exec_cmd(int8 *cmd, int8 *result, int buf_len)   
{   
    (void)result;
    (void)buf_len;
    if (cmd && cmd[0])
        divoom_port_exec((const char *)cmd);
}
#endif /* !DIVOOM_SIMULATOR */


void divoom_app_com_del_file(char *file_name)
{
	divoom_port_remove_file(file_name);
}

void divoom_app_com_mk_dir_for_file_path(const char *file_path)
{
	char dir_path[256];
	const char *slash;
	int dir_len;

	if (!file_path || !file_path[0])
	{
		return;
	}
	slash = strrchr(file_path, '/');
	if (!slash || slash == file_path)
	{
		return;
	}
	dir_len = (int)(slash - file_path);
	if (dir_len <= 0 || dir_len >= (int)sizeof(dir_path))
	{
		return;
	}
	memcpy(dir_path, file_path, (size_t)dir_len);
	dir_path[dir_len] = '\0';
	divoom_app_com_check_and_mk_dir(dir_path);
}
void divoom_app_com_rename_file(char *file_name, char *new_file_name)
{
	if(divoom_port_rename_file(file_name, new_file_name) != 0)
	{
        divoom_print("it fails to rename, %s-> %s\n", file_name, new_file_name);
        divoom_app_com_copy_file(file_name, new_file_name);
        divoom_app_com_del_file(file_name);
        
	}
}

void divoom_app_com_copy_file(char *src_file, char *dest_file)
{
    divoom_port_copy_tree(src_file, dest_file);
}

void divoom_app_com_finishcallback(int ret)
{
}


int divoom_app_com_unicode_to_utf8(uint16* pInput, int len,char* ppOutput, int max)
{
    int out_len = 0;

    for(int i = 0; i < len; i++)
    {
        if(pInput[i] < 128) 
        {
            ppOutput[out_len] = pInput[i];
            out_len ++;
        }
        else if(pInput[i] < 0x0800) {
            ppOutput[out_len] = ((pInput[i] >> 6) & 0x1F) | 0xC0;
            out_len ++;
            ppOutput[out_len] = ((pInput[i] >> 0) & 0x3F) | 0x80;
            out_len ++;
        }
        else {
            ppOutput[out_len] = ((pInput[i] >> 12) & 0x0F) | 0xE0;
            out_len ++;
            ppOutput[out_len] = ((pInput[i] >> 6) & 0x3F) | 0x80;
            out_len ++;
            ppOutput[out_len] = ((pInput[i] >> 0) & 0x3F) | 0x80;
            out_len ++;
            
        }
    }
    ppOutput[out_len] = 0;
    return out_len;
}

int divoom_app_com_utf8_to_unicode(char* ppOutput,char* pInput, int max)
{
    int outputSize = 0;
    char *tmp = ppOutput;

    memset(ppOutput, 0, max);

    while (*pInput && (outputSize < max))
    {
    	/*utf8 single char ,utf8 */
        if (*pInput > 0x00 && *pInput <= 0x7F)
        {
            *tmp = *pInput;
            tmp++;
			/*little endian*/
            *tmp = 0;
        }
		/*utf8 double char ,utf8 */
        else if (((*pInput) & 0xE0) == 0xC0)
        {
            char low;
            char high = *pInput;
            pInput++;
            low = *pInput;
			/*check if it is illegal*/
            if ((low & 0xC0) != 0x80)
            {
				divoom_print("utf8 is illegal1, %d, %d\n",outputSize, max);
//                return 0;
				break;
            }

            *tmp = (high << 6) + (low & 0x3F);
            tmp++;
            *tmp = (high >> 2) & 0x07;
        }
		/*utf8 triple char ,utf8 */
        else if (((*pInput) & 0xF0) == 0xE0)
        {
            char middle, low;
            char high = *pInput;
            pInput++;
            middle = *pInput;
            pInput++;
            low = *pInput;
            if (((middle & 0xC0) != 0x80) || ((low & 0xC0) != 0x80))
            {
				divoom_print("utf8 is illegal2, %d, %d\n",outputSize, max);
//                return 0;
				break;
            }
			*tmp = (middle << 6) + (low & 0x7F);
            tmp++;
            *tmp = (high << 4) + ((middle >> 2) & 0x0F);
        }
        else
        {
			divoom_print("utf8 is illegal3, %d, %d\n",outputSize, max);
            break;
        }
		pInput ++;
        tmp ++;
        outputSize += 2;
    }
    if(outputSize < max)
    {
    	*tmp = 0;
        tmp++;
        *tmp = 0;
    }
    return outputSize;
}
/*
void divoom_com_print_buf(T_U8 *data, T_U16 len)
{
    T_U16 i;
    
    printf("divoom_com_print_buf: len: %d\n", len);

    for(i = 0; i < len; i++)
    {
        if(i % 16 == 0)
        {
            printf("\n");
        }
        printf("%02x ,",data[i]);
    }
            printf("\n");
}
*/

/* JL 音量 0~16 共 17 档，等比例对应 0~100（档位 × 100 / 16） */
static const T_U8 divoom_vol_level_table[DIVOOM_MAX_VOL_VALUE + 1] =
	{0, 6, 12, 18, 25, 31, 37, 43, 50, 56, 62, 68, 75, 81, 87, 93, 100};

uint8 divoom_com_convert_vol(T_U8 vol_level)
{
	if (vol_level > DIVOOM_MAX_VOL_VALUE)
	{
		vol_level = DIVOOM_MAX_VOL_VALUE;
	}
	return divoom_vol_level_table[vol_level];
}

int divoom_com_vol_ui_to_level(uint8 ui_vol)
{
	int i, best = 0, best_diff = 256;

	for (i = 0; i <= DIVOOM_MAX_VOL_VALUE; i++)
	{
		int diff = (int)ui_vol - (int)divoom_vol_level_table[i];

		if (diff < 0)
		{
			diff = -diff;
		}
		if (diff < best_diff)
		{
			best_diff = diff;
			best = i;
		}
	}
	return best;
}

uint8 divoom_com_vol_snap_ui(uint8 ui_vol)
{
	return divoom_vol_level_table[divoom_com_vol_ui_to_level(ui_vol)];
}

uint8 divoom_com_vol_step_ui(uint8 ui_vol, int delta)
{
	int level = divoom_com_vol_ui_to_level(ui_vol);

	level += delta;
	if (level < 0)
	{
		level = 0;
	}
	if (level > DIVOOM_MAX_VOL_VALUE)
	{
		level = DIVOOM_MAX_VOL_VALUE;
	}
	return divoom_vol_level_table[level];
}

uint8 divoom_com_hfp_call_level_to_ui(uint8 level)
{
	if (level > 15u)
	{
		level = 15u;
	}
	return (uint8)(((unsigned)level * 100u) / 15u);
}

T_U32 divoom_app_com_get_check_sum(T_U8 *data, T_U16 len)
{    
    T_U32 i, check_sum = 0;
    
    for(i = 0; i < len; i ++)
    {
        check_sum += data[i];
    }
    return check_sum;
}


char * divoom_app_com_get_float_from_config(char *config, char *item_id, float *item_value)
{    
	char *find = NULL;

    if(config == NULL)
    {
        *item_value = 0;
        return NULL;
    }
    find = strstr(config, item_id);
    if(find == NULL)
    {
        *item_value = 0;
        return NULL;
    }
    find += strlen(item_id) + 1;
    *item_value = atof(find);
    return find;
}

void divoom_app_com_float_to_str(char *buf, int buf_size, float val, int decimals)
{
    long long scale = 1;
    long long int_part;
    long long frac_part;
    int i;
    int negative = 0;
    double dval;

    if (buf == NULL || buf_size <= 0)
    {
        return;
    }
    buf[0] = '\0';
    if (decimals < 0)
    {
        decimals = 0;
    }
    if (decimals > 6)
    {
        decimals = 6;
    }
    for (i = 0; i < decimals; i++)
    {
        scale *= 10;
    }

    dval = (double)val;
    if ((dval * 0.0) != 0.0)
    {
        return;
    }
    if (dval < 0.0)
    {
        negative = 1;
        dval = -dval;
    }

    int_part = (long long)dval;
    if (decimals == 0)
    {
        snprintf(buf, (size_t)buf_size, "%s%lld", negative ? "-" : "", int_part);
        return;
    }

    frac_part = (long long)((dval - (double)int_part) * (double)scale + 0.5);
    if (frac_part >= scale)
    {
        int_part++;
        frac_part -= scale;
    }
    snprintf(buf, (size_t)buf_size, "%s%lld.%0*lld", negative ? "-" : "", int_part, decimals, frac_part);
}

void divoom_app_com_format_double(char *buf, int buf_size, double val, int decimals, int min_width, int force_plus, const char *suffix)
{
    char num_buf[64];
    int len;
    int pad;
    int i;

    if (buf == NULL || buf_size <= 0)
    {
        return;
    }
    buf[0] = '\0';
    divoom_app_com_float_to_str(num_buf, (int)sizeof(num_buf), (float)val, decimals);
    if (num_buf[0] == '\0')
    {
        return;
    }

    if (force_plus && val > 0.0)
    {
        if ((int)sizeof(num_buf) > 1)
        {
            memmove(num_buf + 1, num_buf, strlen(num_buf) + 1);
            num_buf[0] = '+';
        }
    }

    len = (int)strlen(num_buf);
    pad = min_width - len;
    if (pad > 0 && pad < buf_size - 1)
    {
        for (i = 0; i < pad; i++)
        {
            buf[i] = ' ';
        }
        strncpy(buf + pad, num_buf, (size_t)(buf_size - pad - 1));
        buf[buf_size - 1] = '\0';
    }
    else
    {
        strncpy(buf, num_buf, (size_t)(buf_size - 1));
        buf[buf_size - 1] = '\0';
    }

    if (suffix != NULL && suffix[0] != '\0')
    {
        strncat(buf, suffix, (size_t)(buf_size - strlen(buf) - 1));
    }
}


char * divoom_app_com_get_int_from_config(char *config, char *item_id, int *item_value)
{    
	char *find = NULL;

    if(config == NULL)
    {
        *item_value = 0;
        return NULL;
    }
    find = strstr(config, item_id);
    if(find == NULL)
    {
        *item_value = 0;
        return NULL;
    }
    find += strlen(item_id) + 1;
    *item_value = atoi(find);
    return find;
}


char * divoom_app_com_get_str_from_config_no_decode(char *config, char *item_id, char *item_value)
{    
	char *find = NULL;
    int max_len = 0;

    *item_value = 0;
    if(config == NULL)
    {
        return NULL;
    }
    
    find = strstr(config, item_id);
    if(find == NULL)
    {
        return NULL;
    }
    find += strlen(item_id) + 1;
    while(*find && *find != '&')
    {
        *item_value = *find;
        find ++;
        item_value ++;
        max_len ++;
        if(max_len > 126)
        {
            break;
        }
    }
    *item_value = 0;
    return find;
}


char * divoom_app_com_get_str_from_config(char *config, char *item_id, char *item_value)
{    
	char *find = NULL;

    *item_value = 0;
    if(config == NULL)
    {
        return NULL;
    }
    
    find = divoom_app_com_get_str_from_config_no_decode(config, item_id, item_value);
    divoom_twitter_signature_url_decode(item_value, strlen(item_value));
    return find;
}

char * divoom_app_com_get_str_from_config_ex(char *config, char *item_id, char *item_value)
{    
	char *find = NULL, *src = item_value;

    *item_value = '\0';
    if(config == NULL)
    {
        return NULL;
    }
    
    find = strstr(config, item_id);
    if(find == NULL)
    {
        return NULL;
    }
    find += strlen(item_id) + 1;
    while(*find && *find != '&')
    {
        *item_value = *find;
        find ++;
        item_value ++;
    }
    *item_value = '\0';
    divoom_twitter_signature_url_decode(src, strlen(src));
    return find;
}

/*********************************************************************
  Function:        divoom_app_com_get_str_list_from_config
  Description:      从配置中获取指定字段类型的所有值
                    
  Para:            	config 配置
  					item_id 要获取的字段
                   	item_vaule  存储该字段的值
  Return:           个数
  Author:           wangft
**********************************************************************/

int divoom_app_com_get_str_list_from_config(char *config, char *item_id, char item_vaule[][48])
{
	char *p = config;
	char *q = config;
	int i = 0;

	if(config == NULL)
	{
		return 0;
	}

	while ((p = strstr(p, item_id)))
	{
		p = strchr(p, '=');
		p++;

		if ((q = strchr(p, '&')))
		{
			if (q > p)
			{
				strncpy(item_vaule[i], p, q-p);
				item_vaule[i][q-p] = '\0';
			}
			else
			{
				item_vaule[i][0] = '\0';
			}
			i++;
		}
	}
	return i;
}

/*********************************************************************
  Function:        divoom_app_com_skip_char
  Description:      跳过若干次同一个字符，返回后面位置
                    
  Para:             net_data:获取数据失败，1：获取数据成功
                    ch
                    time
  Return:           指针
  Author:           albert
**********************************************************************/

char *divoom_app_com_skip_char(char *src_str, char ch, int time)
{  
    int i;
    char *dest_str = src_str;

    for(i = 0; i < time; i ++)
    {
        dest_str = strchr(dest_str, ch);
        if(dest_str == NULL)
        {
            return NULL;
        }
        dest_str ++;
    }
    return dest_str;
}

/*********************************************************************
  Function:        divoom_app_com_get_string_stop_ch
  Description:     拷贝字符串，到某个结束
                    
  Para:             src_str
                    dest_str
                    ch
  Return:           指针
  Author:           albert
**********************************************************************/


char * divoom_app_com_get_string_stop_ch(char *src_str, char *dest_str, char ch)
{  
    if(src_str == NULL || dest_str == NULL)
    {
        return NULL;
    }
    while(*src_str && *src_str != ch)
    {
        *dest_str = *src_str;
        dest_str ++;
        src_str ++;
    }
    *dest_str = 0;
    
    if(*src_str == ch)
    {
        src_str ++;
    }
    return src_str;
}


/*********************************************************************
  Function:        divoom_app_com_image_get_cache_data
  Description:     拷贝相关图像数据
                    
  Para:             pcache_info
                    item
                    pos
  Return:           
  Author:           albert
**********************************************************************/

void divoom_app_com_image_get_cache_data(void *pcache_info, void *item, int pos)
{
	Pdivoom_manage_image_item_info file_info, pfile_list;

    pfile_list = (Pdivoom_manage_image_item_info)pcache_info;
    file_info = (Pdivoom_manage_image_item_info)item;
    
    memcpy(&pfile_list[pos], file_info, sizeof(divoom_manage_image_item_info));
}

/*********************************************************************
  Function:        divoom_app_com_large_pic
  Description:     放大图像
                    
  Para:             rgb_data
                    raw_width
                    new_width
  Return:           
  Author:           albert
**********************************************************************/


void divoom_app_com_large_pic(uint8 *rgb_data, int raw_width, int new_width)
{
    uint8 *raw_line, *new_line,r, g, b;
    T_S16 i, j , k, large;

    large = new_width / raw_width;
    raw_line = &rgb_data[(raw_width *raw_width - 1) * 3];
    for(i = 0; i < raw_width; i++)
    {
        new_line = &rgb_data[((new_width - (1 + i) *large) *new_width + large * raw_width - 1) * 3];
        //先取一行
        if(new_width > large * raw_width)
        {
            memset(&new_line[3], 0, (new_width - large * raw_width) * 3);
        }
        for(j = 0; j < raw_width; j++)
        {
            r = raw_line[0];
            g = raw_line[1];
            b = raw_line[2];
            for(k = 0; k < large; k ++)
            {
                new_line[0] = r;
                new_line[1] = g;
                new_line[2] = b;
                new_line -=3;
            }
            raw_line -= 3;
        }
        new_line = &rgb_data[(new_width - (1 + i) *large) *new_width * 3];
        for(j = 1; j < large; j ++)
        {
            memcpy(&new_line[j * new_width * 3], new_line, new_width * 3);
        }
    }
}
void divoom_esp_jpeg_decode(uint8 *image_data, int image_size, uint8 *disp_pic_buf)
{
}

/*********************************************************************
  Function:        divoom_app_com_get_file_sha1sum
  Description:     得到文件的SHA256的值
                    
  Para:             file_name
                    sha1sum_info
  Return:           
  Author:           albert
**********************************************************************/

void divoom_app_com_get_file_sha1sum(const char* file_name, char *sha1sum_info) {
#ifdef DIVOOM_SIMULATOR
    FILE *fp;
    char *buf, *ptr;
    char cmd[128];

    if (sha1sum_info)
        sha1sum_info[0] = 0;
    if (!file_name)
        return;

    sprintf(cmd, "/usr/bin/sha1sum %s", file_name);

    fp = popen(cmd, "r");
    if (fp == NULL) {
        divoom_print("Failed to run command\n");
        return;
    }
    buf = divoom_system_malloc(10240);
    if (buf == NULL) {
        pclose(fp);
        return;
    }
    ptr = fgets(buf, 10240 - 1, fp);
    pclose(fp);
    if (ptr == NULL) {
        divoom_system_free(buf);
        return;
    }
    divoom_print("buf: %s\n", buf);
    ptr = strchr(buf, ' ');
    if (ptr && sha1sum_info) {
        int len = ptr - buf;
        memcpy(sha1sum_info, buf, len);
        sha1sum_info[len] = 0;
    }
    divoom_system_free(buf);
#else
    unsigned char digest[20];
    char *buf;
    void *fp;
    size_t n;
    int i;
    mbedtls_sha1_context sha1;

    if (sha1sum_info)
        sha1sum_info[0] = 0;
    if (!file_name)
        return;

    fp = divoom_port_file_open(file_name, "rb");
    if (!fp)
        return;

    buf = divoom_system_malloc(4096);
    if (!buf) {
        divoom_port_file_close(fp);
        return;
    }

    mbedtls_sha1_init(&sha1);
    mbedtls_sha1_starts(&sha1);
    while ((n = divoom_port_file_read(buf, 1, 4096, fp)) > 0)
        mbedtls_sha1_update(&sha1, (const unsigned char *)buf, n);
    mbedtls_sha1_finish(&sha1, digest);
    mbedtls_sha1_free(&sha1);
    divoom_system_free(buf);
    divoom_port_file_close(fp);

    if (!sha1sum_info)
        return;
    for (i = 0; i < 20; i++)
        sprintf(sha1sum_info + i * 2, "%02x", digest[i]);
    sha1sum_info[40] = 0;
#endif
}


/*********************************************************************
  Function:        divoom_app_com_set_image_opa
  Description:     修改图像某个区域的OPA值
                    
  Para:             image_buf    RGB565A8缓冲区
                    x
                    y
                    width
                    high
                    opa_value
                    screen_w
                    screen_h
  Return:           none
**********************************************************************/

void divoom_app_com_set_image_opa(uint8 *image_buf, int x, int y, int width, int high, int opa_value, int screen_w, int screen_h)
{
    uint8 *alpha_buf;

    if(image_buf == NULL || screen_w <= 0 || screen_h <= 0) {
        return;
    }
    alpha_buf = image_buf + (size_t)screen_w * (size_t)screen_h * 2u;
    
    for(int i = 0; i < high && i + y < screen_h; i ++)
    {
        uint8 *cur_alpha_buf = &alpha_buf[(i + y) * screen_w + x];
        for(int j = 0; j < width && j + x < screen_w; j++)
        {
            cur_alpha_buf[0] = (uint8)opa_value;
            cur_alpha_buf++;
        }
    }

}

/*********************************************************************
  Function:        divoom_app_com_get_rand_index
  Description:     得到一个不同的随机数
                    
  Para:             cur_index
                    max_index
  Return:           随机数
**********************************************************************/

int divoom_app_com_get_rand_index(int cur_index, int max_index)
{
    int new_index;

    if(max_index <= 1)
        return 0;
    do{
        new_index = rand()%max_index;
    }while(new_index == cur_index);

    return new_index;
}

/*********************************************************************
  Function:        divoom_app_com_get_resouce_type
  Description:     得到资源文件的类型
                    
  Para:             file_name
  Return:           文件类型
**********************************************************************/
static int divoom_app_com_get_resouce_type_internal(char *file_name, int allow_download)
{
    int read_bytes, try_time = 0;
    uint8 file_buf[4];
    void *file_handle;

    divoom_app_com_get_resouce_type_retry_table:

    file_handle = divoom_port_file_open(file_name, "rb");
    if(file_handle == NULL)
    {
        if(allow_download && try_time == 0)
        {
            divoom_print("it fails to open %s and try\n", file_name);
            divoom_manage_image_redownload_local_file(file_name, 1);
            try_time ++;
            goto divoom_app_com_get_resouce_type_retry_table;
        }
        else
        {
            divoom_print("it fails to open %s\n", file_name);
            return DIVOOM_RESOURCE_TYPE_ERR;
        }
    }
    read_bytes = (int)divoom_port_file_read(file_buf, 1, 4, file_handle);
    if(4 != read_bytes)
    {
        if(allow_download && try_time == 0)
        {
            divoom_print("it fails to read %s: %d and try\n", file_name, read_bytes);
            divoom_port_file_close(file_handle);
            divoom_manage_image_redownload_local_file(file_name, 1);
            try_time ++;
            goto divoom_app_com_get_resouce_type_retry_table;
        }
        else
        {
            divoom_print("it fails to read %s: %d\n", file_name, read_bytes);
            divoom_port_file_close(file_handle);
            return DIVOOM_RESOURCE_TYPE_ERR;
        }
    }
    divoom_port_file_close(file_handle);
    //divoom_print_buf(file_buf, 4);
    if(memcmp(file_buf, "RIFF", 4) == 0)
    {
        return DIVOOM_RESOURCE_TYPE_WEBP;
    }
    else if(file_buf[0] == 0xFF && file_buf[1] == 0xD8)
    {
        return DIVOOM_RESOURCE_TYPE_JPEG;
    }
    else if(memcmp(file_buf, "GIF", 3) == 0)
    {
        return DIVOOM_RESOURCE_TYPE_GIF;
    }
    else if(memcmp(file_buf, "DIVM", 4) == 0)
    {
        return DIVOOM_RESOURCE_TYPE_JPEG_GIF;
    }
    else if(memcmp(file_buf, "\x89\x50\x4e\x47", 4) == 0)
    {
        return DIVOOM_RESOURCE_TYPE_PNG;
    }
    else if(file_buf[0] == 0)
    {
        return DIVOOM_RESOURCE_TYPE_MP4;
    }
    else
    {
        return DIVOOM_RESOURCE_TYPE_DIVOOM;
    }
}

int divoom_app_com_get_resouce_type(char *file_name)
{
    return divoom_app_com_get_resouce_type_internal(file_name, 1);
}

/* Render workers must not wait for a missing file to download. */
int divoom_app_com_get_local_resouce_type(char *file_name)
{
    return divoom_app_com_get_resouce_type_internal(file_name, 0);
}


/*********************************************************
  * FUNCTION		: divoom_app_com_convert_rgb_bgra
  * 		        : 将RGB转化为BGRA方式
  * IUPUT			src_rgb
                    dest_rgb
                    src_w
                    src_h
  * OUTPUT			none
  *********************************************************/


void divoom_app_com_convert_rgb_bgra(uint8 *src_rgb, uint8 *dest_rgb, int src_w, int src_h)
{
    for(int i = 0; i < src_h * src_w; i++)
    {
        dest_rgb[i * 4] = src_rgb[i * 3 + 2];
        dest_rgb[i * 4 + 1] = src_rgb[i * 3 + 1];
        dest_rgb[i * 4 + 2] = src_rgb[i * 3 ];
        dest_rgb[i * 4 + 3] = 255;
    }
}

/*********************************************************
  * FUNCTION		: divoom_app_com_abs_value
  * 		        取两个数差的绝对值
  * IUPUT			value1
                    value2
  * OUTPUT			abs value
  *********************************************************/


double divoom_app_com_abs_value(double value1, double value2)
{
    if(value1 > value2)
    {
        return value1 - value2;
    }
    else
    {
        return value2 - value1;
    }
}

/**
 * 获取UTF-8字符的字节数
 */
int divoom_app_com_utf8_char_len(unsigned char c) {
    if (c < 0x80) return 1;
    if ((c & 0xE0) == 0xC0) return 2;
    if ((c & 0xF0) == 0xE0) return 3;
    if ((c & 0xF8) == 0xF0) return 4;
    return 1; // 无效字符当作1字节处理
}

/**
 * 将UTF-8字符转换为Unicode码点
 */
uint32_t divoom_app_com_utf8_to_codepoint(const unsigned char *str, int len) {
    uint32_t codepoint = 0;
    
    switch (len) {
        case 1:
            return str[0];
        case 2:
            codepoint = ((str[0] & 0x1F) << 6) | (str[1] & 0x3F);
            break;
        case 3:
            codepoint = ((str[0] & 0x0F) << 12) | 
                       ((str[1] & 0x3F) << 6) | 
                       (str[2] & 0x3F);
            break;
        case 4:
            codepoint = ((str[0] & 0x07) << 18) | 
                       ((str[1] & 0x3F) << 12) | 
                       ((str[2] & 0x3F) << 6) | 
                       (str[3] & 0x3F);
            break;
    }
    return codepoint;
}

/**
 * 过滤UTF-8字符串中的表情符号
 * @param in_info 输入的UTF-8字符串
 * @param out_info 输出的过滤后的UTF-8字符串
 */
void divoom_app_com_filter_emj(char *in_info, char *out_info) {
    size_t remaining;

    if (!in_info || !out_info) {
        if (out_info) *out_info = '\0';
        return;
    }

    const unsigned char *in = (const unsigned char *)in_info;
    unsigned char *out = (unsigned char *)out_info;
    remaining = strlen(in_info);

    while (remaining > 0) {
        int char_len = divoom_app_com_utf8_char_len(*in);
        int valid_utf8 = 1;

        if ((size_t)char_len > remaining) {
            break;
        }
        for (int i = 1; i < char_len; i++) {
            if ((in[i] & 0xC0U) != 0x80U) {
                valid_utf8 = 0;
                break;
            }
        }
        if (!valid_utf8) {
            char_len = 1;
        }

        uint32_t codepoint = divoom_app_com_utf8_to_codepoint(in, char_len);
        int is_emoji = 0;

        // 检查是否是表情符号
        if ((codepoint >= 0x1F300 && codepoint <= 0x1F9FF) ||  // 主要表情符号区块
            (codepoint >= 0x2600 && codepoint <= 0x26FF) ||    // 杂项符号
            (codepoint >= 0x2700 && codepoint <= 0x27BF) ||    // 装饰符号
            (codepoint >= 0xFE00 && codepoint <= 0xFE0F) ||    // 变体选择器
            (codepoint >= 0x1F3FB && codepoint <= 0x1F3FF))    // 肤色修饰符
        {
            is_emoji = 1;
        }

        if (!is_emoji) {
            // 不是表情符号，复制原字符
            for (int i = 0; i < char_len; i++) {
                *out++ = in[i];
            }
        }
        in += char_len;
        remaining -= (size_t)char_len;
    }
    
    *out = '\0'; // 添加字符串结束符
}

/**
 * 版本2：创建新字符串（更安全的方式）
 * @param in_str 输入字符串
 * @param out_str 输出字符串
 * @param out_size 输出缓冲区大小
 * @return 处理后的字符串长度，如果缓冲区不足返回-1
 */
int divoom_app_com_add_escape_quote_safe(const char *in_str, char *out_str, size_t out_size) {
    if (!in_str || !out_str || out_size < 2) {
        if (out_str && out_size > 0) {
            *out_str = '\0';
        }
        return -1;
    }

    const char *in = in_str;
    char *out = out_str;
    const char *const end = out_str + out_size - 1;
    
    while (*in && out < end) {
        if (*in == '\'') {
            // 需要两个字节的空间来存储两个单引号
            if (out + 1 >= end) {
                *out_str = '\0';
                return -1;
            }
            *out++ = '\'';
            *out++ = '\'';
        } else {
            *out++ = *in;
        }
        in++;
    }
    
    if (*in) {
        *out_str = '\0';
        return -1;
    }
    
    *out = '\0';
    return (int)(out - out_str);
}



/**
* NAME         divoom_app_com_rgb888_to_rgb565
* @BRIEF	将RGB888缓冲区转换为565
* @PARAM	dest_buf          
                    src_buf   
                    image_size
* @RETURN	 NONE
* @RETVAL
*/

void divoom_app_com_rgb888_to_rgb565(uint8 *dest_buf, uint8 *src_buf, int image_size)
{
    uint16 rgb565_value;
    int i, r, g, b, offset = 0, dest_off = 0;
    for(i = 0; i < image_size; i++)
    {
        r = src_buf[offset];
        g = src_buf[offset+1];
        b = src_buf[offset+2];
        rgb565_value = divoom_aw_rgb_to_rgb565(r, g, b);
        dest_buf[dest_off] = rgb565_value & 0xff;
        dest_off ++;
        dest_buf[dest_off] = (rgb565_value >> 8) & 0xff;
        dest_off ++;
        offset += 3;
    }
}
