#include "divoom.h"
#include "lvgl.h"
#include "divoom_menu_control.h"
#include "divoom_load_message.h"

typedef struct _divoom_load_message_item
{
    char *key;
    char *value;
}divoom_load_message_item, *Pdivoom_load_message_item;

typedef struct _divoom_load_message_ctrl
{
    void *lock;
    int lang_flag;
    void *message_queue;
}divoom_load_message_ctrl, *Pdivoom_load_message_ctrl;

#define DIVOOM_LANG_BASE_PATH  "/usr/share/divoom_app/lang/"

typedef struct _divoom_load_message_lang
{
    int lang_id;
    char *dir_path;
}divoom_load_message_lang, *Pdivoom_load_message_lang;

const divoom_load_message_lang divoom_lang_file_name[] =
{
    {DIVOOM_LVGL_MENU_LANG_ENGLISH,  "en"},
    {DIVOOM_LVGL_MENU_LANG_SC,       "sc"},
    {DIVOOM_LVGL_MENU_LANG_JP,       "ja"},
    {DIVOOM_LVGL_MENU_LANG_Thai,     "th"},
    {DIVOOM_LVGL_MENU_LANG_French,   "fr"},
    {DIVOOM_LVGL_MENU_LANG_Italy,    "it"},
    {DIVOOM_LVGL_MENU_LANG_Vietnam,  "vi"},
    {DIVOOM_LVGL_MENU_LANG_Spanish,  "es"},
    {DIVOOM_LVGL_MENU_LANG_Germany,  "de"},
    {DIVOOM_LVGL_MENU_LANG_Russian,  "ru"},
    {DIVOOM_LVGL_MENU_LANG_Portuguese, "pt"  },
    {DIVOOM_LVGL_MENU_LANG_Korean,  "ko" },
    {DIVOOM_LVGL_MENU_LANG_Swedish,   "sw" },
    {DIVOOM_LVGL_MENU_LANG_Ukraine,  "uk"},
    {DIVOOM_LVGL_MENU_LANG_Danish,       "da"},
    {DIVOOM_LVGL_MENU_LANG_TC,       "tc"},
    {DIVOOM_LVGL_MENU_LANG_Turkey,      "tr" },//土耳其
    {DIVOOM_LVGL_MENU_LANG_Indonesian,   "in"},    //印尼语
    {DIVOOM_LVGL_MENU_LANG_Poland,       "pl"},
};

Pdivoom_load_message_ctrl pdivoom_load_message_handle = NULL;

/* XML 字面换行与 \n 转义统一为单个 LF，避免 LVGL 将 \r\n 断成空行。
 * 原地写入：仅缩短或等长替换，write 始终 <= read，不扩容、不越界。 */
static void divoom_load_message_normalize_newlines(char *value)
{
    char *read;
    char *write;

    if (!value)
    {
        return;
    }
    read = value;
    write = value;
    while (*read)
    {
        if (read[0] == '\\' && read[1] == 'n')
        {
            *write++ = '\n';
            read += 2;
        }
        else if (read[0] == '\r' && read[1] == '\n')
        {
            *write++ = '\n';
            read += 2;
        }
        else if (read[0] == '\r')
        {
            *write++ = '\n';
            read += 1;
        }
        else
        {
            *write++ = *read++;
        }
    }
    *write = '\0';
}

/**
 * NAME         divoom_load_message_init_lang
 * @BRIEF	开机初始化
 * @PARAM	lang : 语言类型

 * @RETURN	NONE
 * @RETVAL
 */

void divoom_load_message_init_lang(uint8 lang)
{
    pdivoom_load_message_handle = divoom_system_malloc(sizeof(divoom_load_message_ctrl));
    if(pdivoom_load_message_handle == NULL)
    {
        return;
    }

    if(lang >= sizeof(divoom_lang_file_name)/sizeof(divoom_lang_file_name[0]))
    {
        lang = 0;
    }
    pdivoom_load_message_handle->lang_flag = -1;
    pdivoom_load_message_handle->lock = divoom_system_create_lock();
    pdivoom_load_message_handle->message_queue = divoom_queue_init(1000, __func__, __LINE__);
    divoom_load_message_set_lang(lang);
}

void divoom_load_message_free_item(void *item)
{
    Pdivoom_load_message_item pitem = (Pdivoom_load_message_item)item;

    divoom_system_free(pitem->key);
    divoom_system_free(pitem->value);
    divoom_system_free(pitem);
}

/**
 * NAME         divoom_load_message_get_all_info
 * @BRIEF	加载所有的信息
 * @PARAM	raw_info : HTML内容

 * @RETURN	NONE
 * @RETVAL
 */

void divoom_load_message_get_all_info(char *raw_info)
{
    int len;
    Pdivoom_load_message_item pcur_item;
    char *start_ptr, *end_ptr;

    start_ptr = raw_info;
    while(start_ptr)
    {
        start_ptr = strstr(start_ptr, "<string name=\"");
        if(start_ptr == NULL)
        {
            break;
        }
        pcur_item = divoom_system_malloc(sizeof(divoom_load_message_item));
        start_ptr += strlen("<string name=\"");
        end_ptr = strstr(start_ptr, "\">");
        if(end_ptr == NULL)
        {
            break;
        }
        len = (uint32)(end_ptr - start_ptr);
        pcur_item->key = divoom_system_malloc(len + 1);
        memcpy(pcur_item->key, start_ptr, len);
        pcur_item->key[len] = 0;

        start_ptr = end_ptr + strlen("\">");
        end_ptr = strstr(start_ptr, "</string>");
        if(end_ptr == NULL)
        {
            break;
        }
        len = (uint32)(end_ptr - start_ptr);
        pcur_item->value = divoom_system_malloc(len + 1);
        memcpy(pcur_item->value, start_ptr, len);
        pcur_item->value[len] = 0;
        divoom_load_message_normalize_newlines(pcur_item->value);
        divoom_queue_push(pdivoom_load_message_handle->message_queue, pcur_item);
        start_ptr = end_ptr + strlen("</string>");
    }
}

/**
 * NAME         divoom_load_message_set_lang
 * @BRIEF	切换语言，加载所有的相关信息
 * @PARAM	lang

 * @RETURN	NONE
 * @RETVAL
 */

void divoom_load_message_set_lang(uint8 lang)
{
    //语言没有，暂时先返回
    
    if(lang >= sizeof(divoom_lang_file_name)/sizeof(divoom_lang_file_name[0]))
    {        
        return;
    }
    if(pdivoom_load_message_handle == NULL)
    {
        return;
    }
    divoom_system_get_lock(pdivoom_load_message_handle->lock);
    if(pdivoom_load_message_handle->lang_flag != lang)
    {
        int pos = 0;
        char file_name[256];

        divoom_print("update lang: %d\n", lang);
        for(int i = 0; i < sizeof(divoom_lang_file_name)/sizeof(divoom_lang_file_name[0]); i++)
        {
            if(divoom_lang_file_name[i].lang_id == lang)
            {
                pos = i;
                break;
            }
        }
        //加载基础语言
        sprintf(file_name,"%sstrings.xml", DIVOOM_LANG_BASE_PATH);
        void *lang_file_info = divoom_app_com_read_file(file_name, NULL);
        if(lang_file_info)
        {
            divoom_print("it ok to read lang file: %s!\n", file_name);
            divoom_queue_pop_all(pdivoom_load_message_handle->message_queue, divoom_load_message_free_item);
            pdivoom_load_message_handle->lang_flag = lang;
            divoom_load_message_get_all_info(lang_file_info);
            divoom_system_free(lang_file_info);
        }
        else
        {
            divoom_print("it fails to read lang file: %s!\n", file_name);
        }

        //加载不同语言
        sprintf(file_name,"%s%s/strings.xml", DIVOOM_LANG_BASE_PATH, divoom_lang_file_name[pos].dir_path);
        lang_file_info = divoom_app_com_read_file(file_name, NULL);
        if(lang_file_info)
        {
            divoom_print("it ok to read lang file: %s!\n", file_name);
            pdivoom_load_message_handle->lang_flag = lang;
            divoom_load_message_get_all_info(lang_file_info);
            divoom_system_free(lang_file_info);
        }
        else
        {
            divoom_print("it fails to read lang file: %s!\n", file_name);
        }
    }

    divoom_system_free_lock(pdivoom_load_message_handle->lock);
}

int divoom_load_message_compare_key(void *item1, void *item2)
{
    Pdivoom_load_message_item pitem = (Pdivoom_load_message_item)item2;

    return strcmp(item1, pitem->key);
}

/**
 * NAME         divoom_load_message_get_value
 * @BRIEF	获取某个键的显示内容
 * @PARAM	key

 * @RETURN	NONE
 * @RETVAL
 */

char *divoom_load_message_get_value(char *key)
{
    Pdivoom_load_message_item pitem;

    /* 开机期 btcore/充电等非 UI 线程可能早于 menu_control_init 调进来，
     * 此时语言表还没建，退化成返回 key，与查不到 key 的行为一致 */
    if(pdivoom_load_message_handle == NULL)
    {
        return key;
    }

    divoom_system_get_lock(pdivoom_load_message_handle->lock);
    pitem = divoom_queue_get_cmp(pdivoom_load_message_handle->message_queue, key, divoom_load_message_compare_key);
    divoom_system_free_lock(pdivoom_load_message_handle->lock);

    if(pitem)
    {
        return pitem->value;
    }
    else
    {
        return key;
    }
}


