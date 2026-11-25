/*
 * AstroToo 产测显示走旧灯阵路径（屏驱已 stub），保留 API 避免 divoom_disp_ctrl 链接失败。
 */
#include "divoom.h"
#include "divoom_disp_ctrl.h"
#include "divoom_disp_test.h"

void divoom_disp_test_init(Pdivoom_disp_ctrl_product_test_info light_data)
{
	(void)light_data;
}

void divoom_disp_test_close(void)
{
}

void divoom_disp_test_update(Pdivoom_disp_ctrl_product_test_info light_data)
{
	(void)light_data;
}
