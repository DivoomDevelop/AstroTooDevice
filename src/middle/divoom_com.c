
#include "divoom.h"

void  divoom_com_pack_uint32(uint8 *data, uint32 value)
{
    data[0] = value & 0xFF;
    data[1] = (value >> 8) & 0xFF;
    data[2] = (value >> 16) & 0xFF;
    data[3] = (value >> 24) & 0xFF;
}


uint32  divoom_com_unpack_uint32(uint8 *data)
{
    uint32 value;

    value = data[0] + ((uint32)data[1] << 8) + ((uint32)data[2] << 16) + ((uint32)data[3] << 24);
    return value;
}


void  divoom_com_pack_uint16(uint8 *data, uint16 value)
{
    data[0] = value & 0xFF;
    data[1] = (value >> 8) & 0xFF;
}


uint16 divoom_com_unpack_uint16(uint8 *data)
{
    uint16 value;
    value = data[0] + ((uint32)data[1] << 8);
    return value;
}


void divoom_com_str_to_low(char *data)
{
    char *ptr = data;

    while(*ptr)
    {
        if(*ptr >= 'A' && *ptr <= 'Z')
        {
            *ptr = *ptr -'A' + 'a';
        }    
        ptr ++;
    }

}


int divoom_com_utf8_to_unicode(char* ppOutput,char* pInput, int max)
{
    int outputSize = 0;
    char *tmp = ppOutput;

    memset(ppOutput, 0, strlen(pInput) * 2);

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
                return 0;
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
                return 0;
            }
			*tmp = (middle << 6) + (low & 0x7F);
            tmp++;
            *tmp = (high << 4) + ((middle >> 2) & 0x0F);
        }
        else
        {
            return 0;
        }
		pInput ++;
        tmp ++;
        outputSize += 2;
    }
	*tmp = 0;
    tmp++;
    *tmp = 0;
    return outputSize;
}


void divoom_com_print_buf(uint8 *data, uint16 len)
{
    uint16 i;
    
    divoom_print("divoom_com_print_buf: len: %d\n", len);

    for(i = 0; i < len; i++)
    {
        divoom_print("%02x:%02x\n", i, data[i]);
    }
    divoom_print("\n");
}

void divoom_com_print_buf_rgb(uint8 *data, uint16 len)
{
    uint16 i, j;
	char buf[48 + 1] = {0};
	
    divoom_print("divoom_com_print_buf: len: %d\n", len);

    for(i = 0; i < len; )
    {
        if(i % 16 == 0)
        {
           // ESP_LOGI("","\n");
        }

		for (j = 0; j < 48; )
		{
			sprintf(&buf[j], "%02x ", data[i]);
			j += 3;
			i++;
			if (i >= len)
			{
				buf[j] = '\0';
				break;
			}
		}
		divoom_print("%s\n", buf);
    }
    divoom_print("\n");
}

