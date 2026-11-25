#include "divoom.h"
#if DIVOOM_LVGL9_IS_SIM
#include "src/libs/qrcode/qrcodegen.h"
#else
/* 真机 lv_conf 关闭了 LVGL QR 控件，在本模块内独立编译已有编码器实现。 */
#undef LV_USE_QRCODE
#define LV_USE_QRCODE 1
#include "../thirdparty/qrcode/qrcodegen.c"
#endif
#include "divoom_qrencode.h"



/*
  Function:       divoom_qrencode_string
  Description:	  对字符串生成二维码
  Input:          string 字符串	strlen 字符串长度
  Return:         	返回一张rgb图片
  Author:         wangft
  Data:           2022-3-3
*/

Pdivoom_qrencode_data divoom_qrencode_string(char *string)
{
    Pdivoom_qrencode_data qrencode_data;
    uint8 *temp_buffer;
    uint8 *qr_buffer;
    uint8 color_value;
    uint32 data_len;
    uint32 buffer_len;
    uint32 pixel_count;
    uint32 pixel_offset;
    int version;
    int qr_size;
    int x;
    int y;

    if (!string || string[0] == '\0')
    {
        return NULL;
    }

    data_len = (uint32)strlen(string);
    version = qrcodegen_getMinFitVersion(qrcodegen_Ecc_LOW, data_len);
    if (version < qrcodegen_VERSION_MIN || version > qrcodegen_VERSION_MAX)
    {
        return NULL;
    }

    buffer_len = (uint32)qrcodegen_BUFFER_LEN_FOR_VERSION(version);
    temp_buffer = (uint8 *)divoom_system_malloc(buffer_len);
    qr_buffer = (uint8 *)divoom_system_malloc(buffer_len);
    if (!temp_buffer || !qr_buffer)
    {
        divoom_system_free(temp_buffer);
        divoom_system_free(qr_buffer);
        return NULL;
    }

    memcpy(temp_buffer, string, data_len);
    if (!qrcodegen_encodeBinary(temp_buffer, data_len, qr_buffer,
        qrcodegen_Ecc_LOW, version, version, qrcodegen_Mask_AUTO, false))
    {
        divoom_system_free(temp_buffer);
        divoom_system_free(qr_buffer);
        return NULL;
    }

    qr_size = qrcodegen_getSize(qr_buffer);
    if (qr_size <= 0 || qr_size > 255)
    {
        divoom_system_free(temp_buffer);
        divoom_system_free(qr_buffer);
        return NULL;
    }

    pixel_count = (uint32)qr_size * (uint32)qr_size;
    qrencode_data = (Pdivoom_qrencode_data)divoom_system_malloc(
        sizeof(divoom_qrencode_data) + pixel_count * 3U);
    if (!qrencode_data)
    {
        divoom_system_free(temp_buffer);
        divoom_system_free(qr_buffer);
        return NULL;
    }

    qrencode_data->rgb_data = (uint8 *)&qrencode_data[1];
    qrencode_data->pic_width = (uint8)qr_size;
    qrencode_data->pic_high = (uint8)qr_size;

    /* 保持旧接口语义：返回不带 quiet zone 的 RGB888 小图，黑白模块由显示层整数放大。 */
    for (y = 0; y < qr_size; y++)
    {
        for (x = 0; x < qr_size; x++)
        {
            color_value = qrcodegen_getModule(qr_buffer, x, y) ? 0U : 255U;
            pixel_offset = ((uint32)y * (uint32)qr_size + (uint32)x) * 3U;
            qrencode_data->rgb_data[pixel_offset] = color_value;
            qrencode_data->rgb_data[pixel_offset + 1U] = color_value;
            qrencode_data->rgb_data[pixel_offset + 2U] = color_value;
        }
    }

    divoom_system_free(temp_buffer);
    divoom_system_free(qr_buffer);
    return qrencode_data;
}

