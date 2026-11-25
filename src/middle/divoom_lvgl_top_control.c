#include "divoom.h"
#include "lvgl.h"


typedef struct _divoom_lvgl_top_control_item
{
    void *top_handle;
    void *top_active_obj;
}divoom_lvgl_top_control_item, *Pdivoom_lvgl_top_control_item;

typedef struct _divoom_lvgl_top_control_ctrl
{
    void *lock_handle;
    void *top_queue;
    void *cur_top_handle;
    void *cur_top_active_obj;
    void *normal_active_obj;
}divoom_lvgl_top_control_ctrl, *Pdivoom_lvgl_top_control_ctrl;


static Pdivoom_lvgl_top_control_ctrl pdivoom_lvgl_top_control_handle = NULL;

/* 闹钟等页面会 lv_group_remove_all_objs，TOP 对象仍在 layer_top 但不在组内；focus 前必须重新入组。 */
static void divoom_lvgl_top_control_focus_obj(void *active_obj)
{
    lv_group_t *grp;

    if (active_obj == NULL || !lv_obj_is_valid(active_obj))
    {
        return;
    }
    grp = lv_group_get_default();
    if (grp == NULL)
    {
        return;
    }
    lv_group_add_obj(grp, active_obj);
    lv_group_focus_obj(active_obj);
}

/* 页面重建会释放旧按钮，同一地址可能被新控件占用。is_valid 分不出复用。
 * 仍在组里的控件直接聚焦。layer_top 上的卡片（留言、时间安排）被清出组后还要加回去。
 * 页面里既不在组、也不在 layer_top 的地址不再入组，避免焦点落到图案上。 */
static int divoom_lvgl_top_control_on_layer_top(lv_obj_t *obj)
{
    lv_obj_t *top = lv_layer_top();

    while (obj)
    {
        if (obj == top)
        {
            return 1;
        }
        obj = lv_obj_get_parent(obj);
    }
    return 0;
}

static void divoom_lvgl_top_control_restore_focus(void *active_obj)
{
    lv_group_t *grp;
    lv_obj_t *obj = (lv_obj_t *)active_obj;

    if (obj == NULL || !lv_obj_is_valid(obj))
    {
        return;
    }
    grp = lv_group_get_default();
    if (grp == NULL)
    {
        return;
    }
    if (lv_obj_get_group(obj) == grp || divoom_lvgl_top_control_on_layer_top(obj))
    {
        divoom_lvgl_top_control_focus_obj(obj);
    }
}

/**
 * NAME         divoom_lvgl_top_control_init
 * @BRIEF	    初始化
 * @PARAM	    none
                                    
 * @RETURN	    none
 * @RETVAL	
 */

void divoom_lvgl_top_control_init(void)
{
    pdivoom_lvgl_top_control_handle = divoom_system_malloc(sizeof(divoom_lvgl_top_control_ctrl));
    pdivoom_lvgl_top_control_handle->top_queue = divoom_queue_init(100, __func__, __LINE__);
    pdivoom_lvgl_top_control_handle->lock_handle = divoom_system_create_lock();
}



/**
 * NAME         divoom_lvgl_top_control_add_top
 * @BRIEF	    添加一个TOP层句柄，负责控件焦点的切换
 * @PARAM	    handle      TOP层的唯一标识
                active_obj  要设置为焦点的控件
                                    
 * @RETURN	    none
 * @RETVAL	
 */

void divoom_lvgl_top_control_add_top(void *handle, void *active_obj)
{
    divoom_system_get_lock(pdivoom_lvgl_top_control_handle->lock_handle);
    if(pdivoom_lvgl_top_control_handle == NULL)
    {
        divoom_system_free_lock(pdivoom_lvgl_top_control_handle->lock_handle);
        return;
    }
    divoom_print("aj test!\n");
    if(pdivoom_lvgl_top_control_handle->cur_top_handle == NULL)
    {
        //如果当前没有TOP菜单的话，将给当前的
        divoom_print("aj test!\n");
        pdivoom_lvgl_top_control_handle->cur_top_handle = handle;
        pdivoom_lvgl_top_control_handle->cur_top_active_obj = active_obj;
        lv_group_t * cur_group_obj = lv_group_get_default();
        pdivoom_lvgl_top_control_handle->normal_active_obj = lv_group_get_focused(cur_group_obj);
    }
    else
    {
        //如果当前有TOP菜单的话，先将当前的压入队列
        Pdivoom_lvgl_top_control_item pnew_item = divoom_system_malloc(sizeof(divoom_lvgl_top_control_item));
        
        divoom_print("aj test!\n");

        pnew_item->top_handle = pdivoom_lvgl_top_control_handle->cur_top_handle;
        pnew_item->top_active_obj = pdivoom_lvgl_top_control_handle->cur_top_active_obj;
        divoom_queue_push(pdivoom_lvgl_top_control_handle->top_queue, pnew_item);
        pdivoom_lvgl_top_control_handle->cur_top_handle = handle;
        pdivoom_lvgl_top_control_handle->cur_top_active_obj = active_obj;
    }
    divoom_lvgl_top_control_focus_obj(active_obj);
    divoom_system_free_lock(pdivoom_lvgl_top_control_handle->lock_handle);
}
int divoom_lvgl_top_control_compare_top_handle(void *item1, void *item2)
{
    
    Pdivoom_lvgl_top_control_item pcur_item = (Pdivoom_lvgl_top_control_item)item2;

    return pcur_item->top_handle == item1?0:1;
}


/**
 * NAME         divoom_lvgl_top_control_update_top
 * @BRIEF	    更新一个句柄的焦点，负责控件焦点的切换
 * @PARAM	    handle      TOP层的唯一标识
                active_obj  要设置为焦点的控件
                                    
 * @RETURN	    none
 * @RETVAL	
 */

void divoom_lvgl_top_control_update_top(void *handle, void *active_obj)
{
    divoom_system_get_lock(pdivoom_lvgl_top_control_handle->lock_handle);
    if(pdivoom_lvgl_top_control_handle == NULL)
    {
        divoom_system_free_lock(pdivoom_lvgl_top_control_handle->lock_handle);
        return;
    }
    if(pdivoom_lvgl_top_control_handle->cur_top_handle == NULL)
    {
        divoom_print("aj test!\n");
    }
    else if(pdivoom_lvgl_top_control_handle->cur_top_handle == handle)
    {
        divoom_print("aj test!\n");
        pdivoom_lvgl_top_control_handle->cur_top_active_obj = active_obj;
        divoom_lvgl_top_control_focus_obj(active_obj);
    }
    else
    {
        //如果当前有TOP菜单的话，先将当前的压入队列
        Pdivoom_lvgl_top_control_item pcur_item;
        
        pcur_item = divoom_queue_pop_cmp(pdivoom_lvgl_top_control_handle->top_queue, handle, divoom_lvgl_top_control_compare_top_handle);

        if(pcur_item)
        {
            divoom_print("aj test!\n");
            pdivoom_lvgl_top_control_handle->cur_top_active_obj = active_obj;
        }
    }
    divoom_system_free_lock(pdivoom_lvgl_top_control_handle->lock_handle);
}


/**
 * NAME         divoom_lvgl_top_control_remove_top
 * @BRIEF	    移除一个TOP层句柄，负责控件焦点的还原
 * @PARAM	    handle      TOP层的唯一标识
                                    
 * @RETURN	    none
 * @RETVAL	
 */

void divoom_lvgl_top_control_remove_top(void *handle)
{
    Pdivoom_lvgl_top_control_item pcur_item;
    
    if(pdivoom_lvgl_top_control_handle == NULL)
    {
        return;
    }
    divoom_system_get_lock(pdivoom_lvgl_top_control_handle->lock_handle);
    if(handle == pdivoom_lvgl_top_control_handle->cur_top_handle)
    {

        pcur_item = divoom_queue_pop(pdivoom_lvgl_top_control_handle->top_queue);
        if(pcur_item == NULL)
        {
            divoom_print("aj test!\n");
            pdivoom_lvgl_top_control_handle->cur_top_handle = NULL;
            pdivoom_lvgl_top_control_handle->cur_top_active_obj = NULL;
            divoom_lvgl_top_control_restore_focus(pdivoom_lvgl_top_control_handle->normal_active_obj);
        }
        else
        {
            divoom_print("aj test!\n");
            pdivoom_lvgl_top_control_handle->cur_top_handle = pcur_item->top_handle;
            pdivoom_lvgl_top_control_handle->cur_top_active_obj = pcur_item->top_active_obj;
            divoom_lvgl_top_control_focus_obj(pdivoom_lvgl_top_control_handle->cur_top_active_obj);
            divoom_system_free(pcur_item);
        }
        
    }
    else
    {
        pcur_item = divoom_queue_pop_cmp(pdivoom_lvgl_top_control_handle->top_queue, handle, divoom_lvgl_top_control_compare_top_handle);
        if(pcur_item)
        {
            divoom_system_free(pcur_item);            
        }
        divoom_print("aj test!\n");
    }
    divoom_system_free_lock(pdivoom_lvgl_top_control_handle->lock_handle);
}


/**
 * NAME         divoom_lvgl_top_control_update_active
 * @BRIEF	    当系统焦点发生变化时，调用这个函数，一般在菜单DISP之后调用
 * @PARAM	    
                                    
 * @RETURN	    none
 * @RETVAL	
 */

void divoom_lvgl_top_control_update_active(void)
{
    lv_group_t * cur_group_obj;
    
    divoom_system_get_lock(pdivoom_lvgl_top_control_handle->lock_handle);
    if(pdivoom_lvgl_top_control_handle == NULL || pdivoom_lvgl_top_control_handle->cur_top_handle == NULL)
    {
        divoom_system_free_lock(pdivoom_lvgl_top_control_handle->lock_handle);
        return;
    }
    
    if(lv_obj_is_valid(pdivoom_lvgl_top_control_handle->cur_top_active_obj))
    {
        divoom_print("aj test!\n");
        cur_group_obj = lv_group_get_default();
        pdivoom_lvgl_top_control_handle->normal_active_obj = lv_group_get_focused(cur_group_obj);
        divoom_lvgl_top_control_focus_obj(pdivoom_lvgl_top_control_handle->cur_top_active_obj);
    }
    divoom_system_free_lock(pdivoom_lvgl_top_control_handle->lock_handle);
}

void divoom_lvgl_layer_top_raise(void *obj)
{
    lv_obj_t *layer_obj = (lv_obj_t *)obj;

    if (layer_obj == NULL || !lv_obj_is_valid(layer_obj))
    {
        return;
    }
    if (lv_obj_has_flag(layer_obj, LV_OBJ_FLAG_HIDDEN))
    {
        return;
    }
    lv_obj_move_foreground(layer_obj);
}

