#include "divoom_disp_picture_data.h"
#include "divoom_disp_picture32_data.h"
#include "divoom_disp_com.h"

void divoom_disp_picture_display(uint16 mode)
{
    uint8 *buf = NULL; 
    uint32 len;

    buf = divoom_disp_picture_get_raw_data(mode, &len);
    divoom_screen_disp_divoom_data(buf);
}

void divoom_disp_picture_get_small_rgb_data(uint16 mode, uint8 *rgb_buf)
{
    uint8 *buf = NULL; 
    uint32 len;

    buf = divoom_disp_picture_get_raw_data(mode, &len);
    divoom_image_decode_Iframe(buf, rgb_buf, DIVOOM_IMAGE_SMALL_WIDTH);
}

void divoom_disp_picture_get_rgb_data(uint16 mode, uint8 *rgb_buf)
{
    uint8 *buf = NULL; 
    uint32 len;

    buf = divoom_disp_picture_get_raw_data(mode, &len);
    divoom_image_decode_Iframe(buf, rgb_buf,DIVOOM_IMAGE_WIDTH);
}



void divoom_disp_picture_get_64rgb_data(uint16 mode, uint8 *rgb_buf)
{
    uint8 *buf = NULL; 
    uint32 len;

    buf = divoom_disp_picture_get_raw_data(mode, &len);
    divoom_image_decode_Iframe(buf, rgb_buf,DIVOOM_IMAGE_WIDTH_64);
}


uint8 *divoom_disp_picture_get_raw_data(uint16 mode, uint32 *pic_len)
{
	switch(mode)
	{       

		case DIVOOM_DISP_EXCHANGE2_POR:
			if(pic_len)
			{
				*pic_len = sizeof(divoom_picture_data_EXCHANGE2_POR);
			}
			return (T_U8 *)divoom_picture_data_EXCHANGE2_POR;
			break;

		case DIVOOM_DISP_EXCHANGE2_WAL:
			if(pic_len)
			{
				*pic_len = sizeof(divoom_picture_data_EXCHANGE2_WAL);
			}
			return (T_U8 *)divoom_picture_data_EXCHANGE2_WAL;
			break;

		case DIVOOM_DISP_EXCHANGE2_GHA:
			if(pic_len)
			{
				*pic_len = sizeof(divoom_picture_data_EXCHANGE2_GHA);
			}
			return (T_U8 *)divoom_picture_data_EXCHANGE2_GHA;
			break;

		case DIVOOM_DISP_EXCHANGE2_CMR:
			if(pic_len)
			{
				*pic_len = sizeof(divoom_picture_data_EXCHANGE2_CMR);
			}
			return (T_U8 *)divoom_picture_data_EXCHANGE2_CMR;
			break;

		case DIVOOM_DISP_EXCHANGE2_URN:
			if(pic_len)
			{
				*pic_len = sizeof(divoom_picture_data_EXCHANGE2_URN);
			}
			return (T_U8 *)divoom_picture_data_EXCHANGE2_URN;
			break;

		case DIVOOM_DISP_EXCHANGE2_MAR:
			if(pic_len)
			{
				*pic_len = sizeof(divoom_picture_data_EXCHANGE2_MAR);
			}
			return (T_U8 *)divoom_picture_data_EXCHANGE2_MAR;
			break;


		case DIVOOM_DISP_EXCHANGE2_MEX:
			if(pic_len)
			{
				*pic_len = sizeof(divoom_picture_data_EXCHANGE2_MEX);
			}
			return (T_U8 *)divoom_picture_data_EXCHANGE2_MEX;
			break;

		case DIVOOM_DISP_EXCHANGE2_TUN:
			if(pic_len)
			{
				*pic_len = sizeof(divoom_picture_data_EXCHANGE2_TUN);
			}
			return (T_U8 *)divoom_picture_data_EXCHANGE2_TUN;
			break;

		case DIVOOM_DISP_EXCHANGE2_KSA:
			if(pic_len)
			{
				*pic_len = sizeof(divoom_picture_data_EXCHANGE2_KSA);
			}
			return (T_U8 *)divoom_picture_data_EXCHANGE2_KSA;
			break;

		case DIVOOM_DISP_EXCHANGE2_QAT:
			if(pic_len)
			{
				*pic_len = sizeof(divoom_picture_data_EXCHANGE2_QAT);
			}
			return (T_U8 *)divoom_picture_data_EXCHANGE2_QAT;
			break;

		case DIVOOM_DISP_EXCHANGE2_ARG:
			if(pic_len)
			{
				*pic_len = sizeof(divoom_picture_data_EXCHANGE2_ARG);
			}
			return (T_U8 *)divoom_picture_data_EXCHANGE2_ARG;
			break;

		case DIVOOM_DISP_EXCHANGE2_AUS:
			if(pic_len)
			{
				*pic_len = sizeof(divoom_picture_data_EXCHANGE2_AUS);
			}
			return (T_U8 *)divoom_picture_data_EXCHANGE2_AUS;
			break;

		case DIVOOM_DISP_EXCHANGE2_BAR:
			if(pic_len)
			{
				*pic_len = sizeof(divoom_picture_data_EXCHANGE2_BAR);
			}
			return (T_U8 *)divoom_picture_data_EXCHANGE2_BAR;
			break;

		case DIVOOM_DISP_EXCHANGE2_BEL:
			if(pic_len)
			{
				*pic_len = sizeof(divoom_picture_data_EXCHANGE2_BEL);
			}
			return (T_U8 *)divoom_picture_data_EXCHANGE2_BEL;
			break;

		case DIVOOM_DISP_EXCHANGE2_CRO:
			if(pic_len)
			{
				*pic_len = sizeof(divoom_picture_data_EXCHANGE2_CRO);
			}
			return (T_U8 *)divoom_picture_data_EXCHANGE2_CRO;
			break;

		case DIVOOM_DISP_EXCHANGE2_DEN:
			if(pic_len)
			{
				*pic_len = sizeof(divoom_picture_data_EXCHANGE2_DEN);
			}
			return (T_U8 *)divoom_picture_data_EXCHANGE2_DEN;
			break;

		case DIVOOM_DISP_EXCHANGE2_ECU:
			if(pic_len)
			{
				*pic_len = sizeof(divoom_picture_data_EXCHANGE2_ECU);
			}
			return (T_U8 *)divoom_picture_data_EXCHANGE2_ECU;
			break;

		case DIVOOM_DISP_EXCHANGE2_ESP:
			if(pic_len)
			{
				*pic_len = sizeof(divoom_picture_data_EXCHANGE2_ESP);
			}
			return (T_U8 *)divoom_picture_data_EXCHANGE2_ESP;
			break;

		case DIVOOM_DISP_EXCHANGE2_FRA:
			if(pic_len)
			{
				*pic_len = sizeof(divoom_picture_data_EXCHANGE2_FRA);
			}
			return (T_U8 *)divoom_picture_data_EXCHANGE2_FRA;
			break;

		case DIVOOM_DISP_EXCHANGE2_GER:
			if(pic_len)
			{
				*pic_len = sizeof(divoom_picture_data_EXCHANGE2_GER);
			}
			return (T_U8 *)divoom_picture_data_EXCHANGE2_GER;
			break;

		case DIVOOM_DISP_EXCHANGE2_IRI:
			if(pic_len)
			{
				*pic_len = sizeof(divoom_picture_data_EXCHANGE2_IRI);
			}
			return (T_U8 *)divoom_picture_data_EXCHANGE2_IRI;
			break;

		case DIVOOM_DISP_EXCHANGE2_NED:
			if(pic_len)
			{
				*pic_len = sizeof(divoom_picture_data_EXCHANGE2_NED);
			}
			return (T_U8 *)divoom_picture_data_EXCHANGE2_NED;
			break;

		case DIVOOM_DISP_EXCHANGE2_POL:
			if(pic_len)
			{
				*pic_len = sizeof(divoom_picture_data_EXCHANGE2_POL);
			}
			return (T_U8 *)divoom_picture_data_EXCHANGE2_POL;
			break;

		case DIVOOM_DISP_EXCHANGE2_SEN:
			if(pic_len)
			{
				*pic_len = sizeof(divoom_picture_data_EXCHANGE2_SEN);
			}
			return (T_U8 *)divoom_picture_data_EXCHANGE2_SEN;
			break;

		case DIVOOM_DISP_EXCHANGE2_CRC:
			if(pic_len)
			{
				*pic_len = sizeof(divoom_picture_data_EXCHANGE2_CRC);
			}
			return (T_U8 *)(divoom_picture_data_EXCHANGE2_CRC);
			break;

		case DIVOOM_DISP_EXCHANGE2_ENG:
			if(pic_len)
			{
				*pic_len = sizeof(divoom_picture_data_EXCHANGE2_ENG);
			}
			return (T_U8 *)divoom_picture_data_EXCHANGE2_ENG;
			break;

		case DIVOOM_DISP_PIXOO64_FORMAT_RESET:
			if(pic_len)
			{
				*pic_len = sizeof(divoom_picture_data_PIXOO64_FORMAT_RESET);
			}
			return (T_U8 *)divoom_picture_data_PIXOO64_FORMAT_RESET;
			break;

		case DIVOOM_DISP_WEATHER__2_00:
			if(pic_len)
			{
				*pic_len = sizeof(divoom_picture_data_WEATHER__2_00);
			}
			return (T_U8 *)divoom_picture_data_WEATHER__2_00;
			break;

		case DIVOOM_DISP_WEATHER__2_01:
			if(pic_len)
			{
				*pic_len = sizeof(divoom_picture_data_WEATHER__2_01);
			}
			return (T_U8 *)divoom_picture_data_WEATHER__2_01;
			break;

		case DIVOOM_DISP_WEATHER__2_02:
			if(pic_len)
			{
				*pic_len = sizeof(divoom_picture_data_WEATHER__2_02);
			}
			return (T_U8 *)divoom_picture_data_WEATHER__2_02;
			break;

		case DIVOOM_DISP_WEATHER__2_03:
			if(pic_len)
			{
				*pic_len = sizeof(divoom_picture_data_WEATHER__2_03);
			}
			return (T_U8 *)divoom_picture_data_WEATHER__2_03;
			break;

		case DIVOOM_DISP_WEATHER__2_04:
			if(pic_len)
			{
				*pic_len = sizeof(divoom_picture_data_WEATHER__2_04);
			}
			return (T_U8 *)divoom_picture_data_WEATHER__2_04;
			break;

		case DIVOOM_DISP_WEATHER__2_05:
			if(pic_len)
			{
				*pic_len = sizeof(divoom_picture_data_WEATHER__2_05);
			}
			return (T_U8 *)divoom_picture_data_WEATHER__2_05;
			break;

		case DIVOOM_DISP_WEATHER__2_06:
			if(pic_len)
			{
				*pic_len = sizeof(divoom_picture_data_WEATHER__2_06);
			}
			return (T_U8 *)divoom_picture_data_WEATHER__2_06;
			break;

		case DIVOOM_DISP_WEATHER__2_07:
			if(pic_len)
			{
				*pic_len = sizeof(divoom_picture_data_WEATHER__2_07);
			}
			return (T_U8 *)divoom_picture_data_WEATHER__2_07;
			break;

		case DIVOOM_DISP_WEATHER__2_08:
			if(pic_len)
			{
				*pic_len = sizeof(divoom_picture_data_WEATHER__2_08);
			}
			return (T_U8 *)divoom_picture_data_WEATHER__2_08;
			break;

		case DIVOOM_DISP_WEATHER__2_09:
			if(pic_len)
			{
				*pic_len = sizeof(divoom_picture_data_WEATHER__2_09);
			}
			return (T_U8 *)divoom_picture_data_WEATHER__2_09;
			break;


		case DIVOOM_DISP_DIVOOM_TF_ADAPTER_ERR:
			if(pic_len)
			{
				*pic_len = sizeof(divoom_picture_data_DIVOOM_TF_ADAPTER_ERR);
			}
			return (T_U8 *)divoom_picture_data_DIVOOM_TF_ADAPTER_ERR;
			break;

		case DIVOOM_DISP_TEST_MODE_BLUE:
			if(pic_len)
			{
				*pic_len = sizeof(divoom_picture_data_test_mode_blue);
			}
			return (uint8 *)divoom_picture_data_test_mode_blue;
			break;

		case DIVOOM_DISP_TEST_MODE_RTC:
			if(pic_len)
			{
				*pic_len = sizeof(divoom_picture_data_test_mode_rtc);
			}
			return (uint8 *)divoom_picture_data_test_mode_rtc;
			break;

		case DIVOOM_DISP_TEST_ERROR_PIC:
			if(pic_len)
			{
				*pic_len = sizeof(divoom_picture_data_test_error_pic);
			}
			return (uint8 *)divoom_picture_data_test_error_pic;
			break;

		case DIVOOM_DISP_TEST_USB_IN_PIC:
			if(pic_len)
			{
				*pic_len = sizeof(divoom_picture_data_test_usb_in_pic);
			}
			return (uint8 *)divoom_picture_data_test_usb_in_pic;
			break;

		case DIVOOM_DISP_TEST_USB_OUT_PIC:
			if(pic_len)
			{
				*pic_len = sizeof(divoom_picture_data_test_usb_out_pic);
			}
			return (uint8 *)divoom_picture_data_test_usb_out_pic;
			break;



		case DIVOOM_DISP_BLUE_CONNECT_00:
			if(pic_len)
			{
				*pic_len = sizeof(divoom_picture_data_blue_connect_00);
			}
			return (T_U8 *)divoom_picture_data_blue_connect_00;
			break;
			
		case DIVOOM_DISP_BLUE_CONNECT_01:
			if(pic_len)
			{
				*pic_len = sizeof(divoom_picture_data_blue_connect_01);
			}
			return (T_U8 *)divoom_picture_data_blue_connect_01;
			break;

		case DIVOOM_DISP_BLUE_CONNECT_02:
			if(pic_len)
			{
				*pic_len = sizeof(divoom_picture_data_blue_connect_02);
			}
			return (T_U8 *)divoom_picture_data_blue_connect_02;
			break;

		case DIVOOM_DISP_BLUE_DISCONNECT:
			if(pic_len)
			{
				*pic_len = sizeof(divoom_picture_data_blue_disconnect);
			}
			return (T_U8 *)divoom_picture_data_blue_disconnect;
			break;

		case DIVOOM_DISP_BLUE_MODE_00:
			if(pic_len)
			{
				*pic_len = sizeof(divoom_picture_data_blue_mode_00);
			}
			return (T_U8 *)divoom_picture_data_blue_mode_00;
			break;

		case DIVOOM_DISP_BLUE_MODE_01:
			if(pic_len)
			{
				*pic_len = sizeof(divoom_picture_data_blue_mode_01);
			}
			return (T_U8 *)divoom_picture_data_blue_mode_01;
			break;
			
		case DIVOOM_DISP_DIVOOM_SD_ERR:
			if(pic_len)
			{
				*pic_len = sizeof(divoom_picture_data_DIVOOM_SD_ERR);
			}
			return (T_U8 *)divoom_picture_data_DIVOOM_SD_ERR;
			break;

		case DIVOOM_DISP_TEST_SD_ERR_PIC:
			if(pic_len)
			{
				*pic_len = sizeof(divoom_picture_data_test_sd_err_pic);
			}
			return (T_U8 *)divoom_picture_data_test_sd_err_pic;
			break;

		case DIVOOM_DISP_TEST_SD_PULL_IN_PIC:
			if(pic_len)
			{
				*pic_len = sizeof(divoom_picture_data_test_sd_pull_in_pic);
			}
			return (T_U8 *)divoom_picture_data_test_sd_pull_in_pic;
			break;

		case DIVOOM_DISP_TEST_SD_PULL_OUT_PIC:
			if(pic_len)
			{
				*pic_len = sizeof(divoom_picture_data_test_sd_pull_out_pic);
			}
			return (T_U8 *)divoom_picture_data_test_sd_pull_out_pic;
			break;


		case DIVOOM_DISP_DIVOOM_SD_OUT:
			if(pic_len)
			{
				*pic_len = sizeof(divoom_picture_data_DIVOOM_SD_OUT);
			}
			return (T_U8 *)divoom_picture_data_DIVOOM_SD_OUT;
			break;


		case DIVOOM_DISP_DIVOOM_POWER_ON:
			if(pic_len)
			{
				*pic_len = sizeof(divoom_picture_data_DIVOOM_POWER_ON);
			}
			return (T_U8 *)divoom_picture_data_DIVOOM_POWER_ON;
			break;

		case DIVOOM_DISP_SD_WAIT_SD_WAIT:
			if(pic_len)
			{
				*pic_len = sizeof(divoom_picture_data_SD_WAIT_SD_WAIT);
			}
			return (T_U8 *)divoom_picture_data_SD_WAIT_SD_WAIT;
			break;

		case DIVOOM_DISP_WIFI_CONFIG_00:
			if(pic_len)
			{
				*pic_len = sizeof(divoom_picture_data_wifi_config_00);
			}
			return (T_U8 *)divoom_picture_data_wifi_config_00;
			break;

		case DIVOOM_DISP_WIFI_CONFIG_01:
			if(pic_len)
			{
				*pic_len = sizeof(divoom_picture_data_wifi_config_01);
			}
			return (T_U8 *)divoom_picture_data_wifi_config_01;
			break;

		case DIVOOM_DISP_WIFI_CONFIG_02:
			if(pic_len)
			{
				*pic_len = sizeof(divoom_picture_data_wifi_config_02);
			}
			return (T_U8 *)divoom_picture_data_wifi_config_02;
			break;

		case DIVOOM_DISP_WIFI_CONNECTING_00:
			if(pic_len)
			{
				*pic_len = sizeof(divoom_picture_data_wifi_connecting_00);
			}
			return (T_U8 *)divoom_picture_data_wifi_connecting_00;
			break;

		case DIVOOM_DISP_WIFI_CONNECTING_01:
			if(pic_len)
			{
				*pic_len = sizeof(divoom_picture_data_wifi_connecting_01);
			}
			return (T_U8 *)divoom_picture_data_wifi_connecting_01;
			break;

		case DIVOOM_DISP_WIFI_CONNECTING_02:
			if(pic_len)
			{
				*pic_len = sizeof(divoom_picture_data_wifi_connecting_02);
			}
			return (T_U8 *)divoom_picture_data_wifi_connecting_02;
			break;

		case DIVOOM_DISP_WIFI_CONNECTING_03:
			if(pic_len)
			{
				*pic_len = sizeof(divoom_picture_data_wifi_connecting_03);
			}
			return (T_U8 *)divoom_picture_data_wifi_connecting_03;
			break;

		case DIVOOM_DISP_WIFI_CONNECTING_04:
			if(pic_len)
			{
				*pic_len = sizeof(divoom_picture_data_wifi_connecting_04);
			}
			return (T_U8 *)divoom_picture_data_wifi_connecting_04;
			break;

		case DIVOOM_DISP_WIFI_FAIL:
			if(pic_len)
			{
				*pic_len = sizeof(divoom_picture_data_wifi_fail);
			}
			return (T_U8 *)divoom_picture_data_wifi_fail;
			break;

		case DIVOOM_DISP_WIFI_OK:
			if(pic_len)
			{
				*pic_len = sizeof(divoom_picture_data_wifi_ok);
			}
			return (T_U8 *)divoom_picture_data_wifi_ok;
			break;

		case DIVOOM_DISP_WIFI_TIMEOUT:
			if(pic_len)
			{
				*pic_len = sizeof(divoom_picture_data_wifi_timeout);
			}
			return (T_U8 *)divoom_picture_data_wifi_timeout;
			break;


		case DIVOOM_DISP_EXCHANGE_AED:
			if(pic_len)
			{
				*pic_len = sizeof(divoom_picture_data_exchange_AED);
			}
			return (T_U8 *)divoom_picture_data_exchange_AED;
			break;

		case DIVOOM_DISP_EXCHANGE_AUD:
			if(pic_len)
			{
				*pic_len = sizeof(divoom_picture_data_exchange_AUD);
			}
			return (T_U8 *)divoom_picture_data_exchange_AUD;
			break;

		case DIVOOM_DISP_EXCHANGE_BHD:
			if(pic_len)
			{
				*pic_len = sizeof(divoom_picture_data_exchange_BHD);
			}
			return (T_U8 *)divoom_picture_data_exchange_BHD;
			break;

		case DIVOOM_DISP_EXCHANGE_BND:
			if(pic_len)
			{
				*pic_len = sizeof(divoom_picture_data_exchange_BND);
			}
			return (T_U8 *)divoom_picture_data_exchange_BND;
			break;

		case DIVOOM_DISP_EXCHANGE_BRL:
			if(pic_len)
			{
				*pic_len = sizeof(divoom_picture_data_exchange_BRL);
			}
			return (T_U8 *)divoom_picture_data_exchange_BRL;
			break;

		case DIVOOM_DISP_EXCHANGE_BUK:
			if(pic_len)
			{
				*pic_len = sizeof(divoom_picture_data_exchange_BUK);
			}
			return (T_U8 *)divoom_picture_data_exchange_BUK;
			break;

		case DIVOOM_DISP_EXCHANGE_BYR:
			if(pic_len)
			{
				*pic_len = sizeof(divoom_picture_data_exchange_BYR);
			}
			return (T_U8 *)divoom_picture_data_exchange_BYR;
			break;

		case DIVOOM_DISP_EXCHANGE_CAD:
			if(pic_len)
			{
				*pic_len = sizeof(divoom_picture_data_exchange_CAD);
			}
			return (T_U8 *)divoom_picture_data_exchange_CAD;
			break;

		case DIVOOM_DISP_EXCHANGE_CHF:
			if(pic_len)
			{
				*pic_len = sizeof(divoom_picture_data_exchange_CHF);
			}
			return (T_U8 *)divoom_picture_data_exchange_CHF;
			break;

		case DIVOOM_DISP_EXCHANGE_CLP:
			if(pic_len)
			{
				*pic_len = sizeof(divoom_picture_data_exchange_CLP);
			}
			return (T_U8 *)divoom_picture_data_exchange_CLP;
			break;

		case DIVOOM_DISP_EXCHANGE_CNY:
			if(pic_len)
			{
				*pic_len = sizeof(divoom_picture_data_exchange_CNY);
			}
			return (T_U8 *)divoom_picture_data_exchange_CNY;
			break;

		case DIVOOM_DISP_EXCHANGE_COP:
			if(pic_len)
			{
				*pic_len = sizeof(divoom_picture_data_exchange_COP);
			}
			return (T_U8 *)divoom_picture_data_exchange_COP;
			break;

		case DIVOOM_DISP_EXCHANGE_CRC:
			if(pic_len)
			{
				*pic_len = sizeof(divoom_picture_data_exchange_CRC);
			}
			return (T_U8 *)divoom_picture_data_exchange_CRC;
			break;

		case DIVOOM_DISP_EXCHANGE_DZD:
			if(pic_len)
			{
				*pic_len = sizeof(divoom_picture_data_exchange_DZD);
			}
			return (T_U8 *)divoom_picture_data_exchange_DZD;
			break;

		case DIVOOM_DISP_EXCHANGE_EGP:
			if(pic_len)
			{
				*pic_len = sizeof(divoom_picture_data_exchange_EGP);
			}
			return (T_U8 *)divoom_picture_data_exchange_EGP;
			break;

		case DIVOOM_DISP_EXCHANGE_ESD:
			if(pic_len)
			{
				*pic_len = sizeof(divoom_picture_data_exchange_ESD);
			}
			return (T_U8 *)divoom_picture_data_exchange_ESD;
			break;

		case DIVOOM_DISP_EXCHANGE_EUR:
			if(pic_len)
			{
				*pic_len = sizeof(divoom_picture_data_exchange_EUR);
			}
			return (T_U8 *)divoom_picture_data_exchange_EUR;
			break;

		case DIVOOM_DISP_EXCHANGE_GBP:
			if(pic_len)
			{
				*pic_len = sizeof(divoom_picture_data_exchange_GBP);
			}
			return (T_U8 *)divoom_picture_data_exchange_GBP;
			break;

		case DIVOOM_DISP_EXCHANGE_HKD:
			if(pic_len)
			{
				*pic_len = sizeof(divoom_picture_data_exchange_HKD);
			}
			return (T_U8 *)divoom_picture_data_exchange_HKD;
			break;

		case DIVOOM_DISP_EXCHANGE_IDR:
			if(pic_len)
			{
				*pic_len = sizeof(divoom_picture_data_exchange_IDR);
			}
			return (T_U8 *)divoom_picture_data_exchange_IDR;
			break;

		case DIVOOM_DISP_EXCHANGE_ILS:
			if(pic_len)
			{
				*pic_len = sizeof(divoom_picture_data_exchange_ILS);
			}
			return (T_U8 *)divoom_picture_data_exchange_ILS;
			break;

		case DIVOOM_DISP_EXCHANGE_INR:
			if(pic_len)
			{
				*pic_len = sizeof(divoom_picture_data_exchange_INR);
			}
			return (T_U8 *)divoom_picture_data_exchange_INR;
			break;

		case DIVOOM_DISP_EXCHANGE_IQD:
			if(pic_len)
			{
				*pic_len = sizeof(divoom_picture_data_exchange_IQD);
			}
			return (T_U8 *)divoom_picture_data_exchange_IQD;
			break;

		case DIVOOM_DISP_EXCHANGE_ISK:
			if(pic_len)
			{
				*pic_len = sizeof(divoom_picture_data_exchange_ISK);
			}
			return (T_U8 *)divoom_picture_data_exchange_ISK;
			break;

		case DIVOOM_DISP_EXCHANGE_JOD:
			if(pic_len)
			{
				*pic_len = sizeof(divoom_picture_data_exchange_JOD);
			}
			return (T_U8 *)divoom_picture_data_exchange_JOD;
			break;

		case DIVOOM_DISP_EXCHANGE_JPY:
			if(pic_len)
			{
				*pic_len = sizeof(divoom_picture_data_exchange_JPY);
			}
			return (T_U8 *)divoom_picture_data_exchange_JPY;
			break;

		case DIVOOM_DISP_EXCHANGE_KES:
			if(pic_len)
			{
				*pic_len = sizeof(divoom_picture_data_exchange_KES);
			}
			return (T_U8 *)divoom_picture_data_exchange_KES;
			break;

		case DIVOOM_DISP_EXCHANGE_KHR:
			if(pic_len)
			{
				*pic_len = sizeof(divoom_picture_data_exchange_KHR);
			}
			return (T_U8 *)divoom_picture_data_exchange_KHR;
			break;

		case DIVOOM_DISP_EXCHANGE_KRW:
			if(pic_len)
			{
				*pic_len = sizeof(divoom_picture_data_exchange_KRW);
			}
			return (T_U8 *)divoom_picture_data_exchange_KRW;
			break;

		case DIVOOM_DISP_EXCHANGE_KWD:
			if(pic_len)
			{
				*pic_len = sizeof(divoom_picture_data_exchange_KWD);
			}
			return (T_U8 *)divoom_picture_data_exchange_KWD;
			break;

		case DIVOOM_DISP_EXCHANGE_LAK:
			if(pic_len)
			{
				*pic_len = sizeof(divoom_picture_data_exchange_LAK);
			}
			return (T_U8 *)divoom_picture_data_exchange_LAK;
			break;

		case DIVOOM_DISP_EXCHANGE_LBP:
			if(pic_len)
			{
				*pic_len = sizeof(divoom_picture_data_exchange_LBP);
			}
			return (T_U8 *)divoom_picture_data_exchange_LBP;
			break;

		case DIVOOM_DISP_EXCHANGE_LKR:
			if(pic_len)
			{
				*pic_len = sizeof(divoom_picture_data_exchange_LKR);
			}
			return (T_U8 *)divoom_picture_data_exchange_LKR;
			break;

		case DIVOOM_DISP_EXCHANGE_MAD:
			if(pic_len)
			{
				*pic_len = sizeof(divoom_picture_data_exchange_MAD);
			}
			return (T_U8 *)divoom_picture_data_exchange_MAD;
			break;

		case DIVOOM_DISP_EXCHANGE_MOP:
			if(pic_len)
			{
				*pic_len = sizeof(divoom_picture_data_exchange_MOP);
			}
			return (T_U8 *)divoom_picture_data_exchange_MOP;
			break;

		case DIVOOM_DISP_EXCHANGE_MXN:
			if(pic_len)
			{
				*pic_len = sizeof(divoom_picture_data_exchange_MXN);
			}
			return (T_U8 *)divoom_picture_data_exchange_MXN;
			break;

		case DIVOOM_DISP_EXCHANGE_MYR:
			if(pic_len)
			{
				*pic_len = sizeof(divoom_picture_data_exchange_MYR);
			}
			return (T_U8 *)divoom_picture_data_exchange_MYR;
			break;

		case DIVOOM_DISP_EXCHANGE_NOK:
			if(pic_len)
			{
				*pic_len = sizeof(divoom_picture_data_exchange_NOK);
			}
			return (T_U8 *)divoom_picture_data_exchange_NOK;
			break;

		case DIVOOM_DISP_EXCHANGE_NZD:
			if(pic_len)
			{
				*pic_len = sizeof(divoom_picture_data_exchange_NZD);
			}
			return (T_U8 *)divoom_picture_data_exchange_NZD;
			break;

		case DIVOOM_DISP_EXCHANGE_OMR:
			if(pic_len)
			{
				*pic_len = sizeof(divoom_picture_data_exchange_OMR);
			}
			return (T_U8 *)divoom_picture_data_exchange_OMR;
			break;

		case DIVOOM_DISP_EXCHANGE_PHP:
			if(pic_len)
			{
				*pic_len = sizeof(divoom_picture_data_exchange_PHP);
			}
			return (T_U8 *)divoom_picture_data_exchange_PHP;
			break;

		case DIVOOM_DISP_EXCHANGE_QAR:
			if(pic_len)
			{
				*pic_len = sizeof(divoom_picture_data_exchange_QAR);
			}
			return (T_U8 *)divoom_picture_data_exchange_QAR;
			break;

		case DIVOOM_DISP_EXCHANGE_RUB:
			if(pic_len)
			{
				*pic_len = sizeof(divoom_picture_data_exchange_RUB);
			}
			return (T_U8 *)divoom_picture_data_exchange_RUB;
			break;

		case DIVOOM_DISP_EXCHANGE_SAR:
			if(pic_len)
			{
				*pic_len = sizeof(divoom_picture_data_exchange_SAR);
			}
			return (T_U8 *)divoom_picture_data_exchange_SAR;
			break;

		case DIVOOM_DISP_EXCHANGE_SGD:
			if(pic_len)
			{
				*pic_len = sizeof(divoom_picture_data_exchange_SGD);
			}
			return (T_U8 *)divoom_picture_data_exchange_SGD;
			break;

		case DIVOOM_DISP_EXCHANGE_SYP:
			if(pic_len)
			{
				*pic_len = sizeof(divoom_picture_data_exchange_SYP);
			}
			return (T_U8 *)divoom_picture_data_exchange_SYP;
			break;

		case DIVOOM_DISP_EXCHANGE_THB:
			if(pic_len)
			{
				*pic_len = sizeof(divoom_picture_data_exchange_THB);
			}
			return (T_U8 *)divoom_picture_data_exchange_THB;
			break;

		case DIVOOM_DISP_EXCHANGE_TRY:
			if(pic_len)
			{
				*pic_len = sizeof(divoom_picture_data_exchange_TRY);
			}
			return (T_U8 *)divoom_picture_data_exchange_TRY;
			break;

		case DIVOOM_DISP_EXCHANGE_TWD:
			if(pic_len)
			{
				*pic_len = sizeof(divoom_picture_data_exchange_TWD);
			}
			return (T_U8 *)divoom_picture_data_exchange_TWD;
			break;

		case DIVOOM_DISP_EXCHANGE_TZS:
			if(pic_len)
			{
				*pic_len = sizeof(divoom_picture_data_exchange_TZS);
			}
			return (T_U8 *)divoom_picture_data_exchange_TZS;
			break;

		case DIVOOM_DISP_EXCHANGE_UGX:
			if(pic_len)
			{
				*pic_len = sizeof(divoom_picture_data_exchange_UGX);
			}
			return (T_U8 *)divoom_picture_data_exchange_UGX;
			break;

		case DIVOOM_DISP_EXCHANGE_USD:
			if(pic_len)
			{
				*pic_len = sizeof(divoom_picture_data_exchange_USD);
			}
			return (T_U8 *)divoom_picture_data_exchange_USD;
			break;

		case DIVOOM_DISP_EXCHANGE_VND:
			if(pic_len)
			{
				*pic_len = sizeof(divoom_picture_data_exchange_VND);
			}
			return (T_U8 *)divoom_picture_data_exchange_VND;
			break;

		case DIVOOM_DISP_EXCHANGE_ZAR:
			if(pic_len)
			{
				*pic_len = sizeof(divoom_picture_data_exchange_ZAR);
			}
			return (T_U8 *)divoom_picture_data_exchange_ZAR;
			break;

		case DIVOOM_DISP_EXCHANGE_ZMK:
			if(pic_len)
			{
				*pic_len = sizeof(divoom_picture_data_exchange_ZMK);
			}
			return (T_U8 *)divoom_picture_data_exchange_ZMK;
			break;

		case DIVOOM_DISP_EXCHANGE_ALB:
			if(pic_len)
			{
				*pic_len = sizeof(divoom_picture_data_EXCHANGE_ALB);
			}
			return (T_U8 *)divoom_picture_data_EXCHANGE_ALB;
			break;

		case DIVOOM_DISP_EXCHANGE_AUT:
			if(pic_len)
			{
				*pic_len = sizeof(divoom_picture_data_EXCHANGE_AUT);
			}
			return (T_U8 *)divoom_picture_data_EXCHANGE_AUT;
			break;

		case DIVOOM_DISP_EXCHANGE_BEL:
			if(pic_len)
			{
				*pic_len = sizeof(divoom_picture_data_EXCHANGE_BEL);
			}
			return (T_U8 *)divoom_picture_data_EXCHANGE_BEL;
			break;

		case DIVOOM_DISP_EXCHANGE_BUL:
			if(pic_len)
			{
				*pic_len = sizeof(divoom_picture_data_EXCHANGE_BUL);
			}
			return (T_U8 *)divoom_picture_data_EXCHANGE_BUL;
			break;

		case DIVOOM_DISP_EXCHANGE_DEN:
			if(pic_len)
			{
				*pic_len = sizeof(divoom_picture_data_EXCHANGE_DEN);
			}
			return (T_U8 *)divoom_picture_data_EXCHANGE_DEN;
			break;

		case DIVOOM_DISP_EXCHANGE_ESP:
			if(pic_len)
			{
				*pic_len = sizeof(divoom_picture_data_EXCHANGE_ESP);
			}
			return (T_U8 *)divoom_picture_data_EXCHANGE_ESP;
			break;

		case DIVOOM_DISP_EXCHANGE_EST:
			if(pic_len)
			{
				*pic_len = sizeof(divoom_picture_data_EXCHANGE_EST);
			}
			return (T_U8 *)divoom_picture_data_EXCHANGE_EST;
			break;

		case DIVOOM_DISP_EXCHANGE_GRE:
			if(pic_len)
			{
				*pic_len = sizeof(divoom_picture_data_EXCHANGE_GRE);
			}
			return (T_U8 *)divoom_picture_data_EXCHANGE_GRE;
			break;


		case DIVOOM_DISP_EXCHANGE_IRL:
			if(pic_len)
			{
				*pic_len = sizeof(divoom_picture_data_EXCHANGE_IRL);
			}
			return (T_U8 *)divoom_picture_data_EXCHANGE_IRL;
			break;

		case DIVOOM_DISP_EXCHANGE_LAT:
			if(pic_len)
			{
				*pic_len = sizeof(divoom_picture_data_EXCHANGE_LAT);
			}
			return (T_U8 *)divoom_picture_data_EXCHANGE_LAT;
			break;

		case DIVOOM_DISP_EXCHANGE_LTU:
			if(pic_len)
			{
				*pic_len = sizeof(divoom_picture_data_EXCHANGE_LTU);
			}
			return (T_U8 *)divoom_picture_data_EXCHANGE_LTU;
			break;

		case DIVOOM_DISP_EXCHANGE_LUX:
			if(pic_len)
			{
				*pic_len = sizeof(divoom_picture_data_EXCHANGE_LUX);
			}
			return (T_U8 *)divoom_picture_data_EXCHANGE_LUX;
			break;

		case DIVOOM_DISP_EXCHANGE_MKD:
			if(pic_len)
			{
				*pic_len = sizeof(divoom_picture_data_EXCHANGE_MKD);
			}
			return (T_U8 *)divoom_picture_data_EXCHANGE_MKD;
			break;

		case DIVOOM_DISP_EXCHANGE_MON:
			if(pic_len)
			{
				*pic_len = sizeof(divoom_picture_data_EXCHANGE_MON);
			}
			return (T_U8 *)divoom_picture_data_EXCHANGE_MON;
			break;

		case DIVOOM_DISP_EXCHANGE_ROU:
			if(pic_len)
			{
				*pic_len = sizeof(divoom_picture_data_EXCHANGE_ROU);
			}
			return (T_U8 *)divoom_picture_data_EXCHANGE_ROU;
			break;

		case DIVOOM_DISP_EXCHANGE_SRB:
			if(pic_len)
			{
				*pic_len = sizeof(divoom_picture_data_EXCHANGE_SRB);
			}
			return (T_U8 *)divoom_picture_data_EXCHANGE_SRB;
			break;

		case DIVOOM_DISP_EXCHANGE_UKR:
			if(pic_len)
			{
				*pic_len = sizeof(divoom_picture_data_EXCHANGE_UKR);
			}
			return (T_U8 *)divoom_picture_data_EXCHANGE_UKR;
			break;

		case DIVOOM_DISP_EXCHANGE_BLR:
			if(pic_len)
			{
				*pic_len = sizeof(divoom_picture_data_EXCHANGE_BLR);
			}
			return (T_U8 *)divoom_picture_data_EXCHANGE_BLR;
			break;

		case DIVOOM_DISP_EXCHANGE_CZE:
			if(pic_len)
			{
				*pic_len = sizeof(divoom_picture_data_EXCHANGE_CZE);
			}
			return (T_U8 *)divoom_picture_data_EXCHANGE_CZE;
			break;

		case DIVOOM_DISP_EXCHANGE_FIN:
			if(pic_len)
			{
				*pic_len = sizeof(divoom_picture_data_EXCHANGE_FIN);
			}
			return (T_U8 *)divoom_picture_data_EXCHANGE_FIN;
			break;

		case DIVOOM_DISP_EXCHANGE_FRA:
			if(pic_len)
			{
				*pic_len = sizeof(divoom_picture_data_EXCHANGE_FRA);
			}
			return (T_U8 *)divoom_picture_data_EXCHANGE_FRA;
			break;

		case DIVOOM_DISP_EXCHANGE_GER:
			if(pic_len)
			{
				*pic_len = sizeof(divoom_picture_data_EXCHANGE_GER);
			}
			return (T_U8 *)divoom_picture_data_EXCHANGE_GER;
			break;

		case DIVOOM_DISP_EXCHANGE_HUN:
			if(pic_len)
			{
				*pic_len = sizeof(divoom_picture_data_EXCHANGE_HUN);
			}
			return (T_U8 *)divoom_picture_data_EXCHANGE_HUN;
			break;

		case DIVOOM_DISP_EXCHANGE_ITA:
			if(pic_len)
			{
				*pic_len = sizeof(divoom_picture_data_EXCHANGE_ITA);
			}
			return (T_U8 *)divoom_picture_data_EXCHANGE_ITA;
			break;

		case DIVOOM_DISP_EXCHANGE_NED:
			if(pic_len)
			{
				*pic_len = sizeof(divoom_picture_data_EXCHANGE_NED);
			}
			return (T_U8 *)divoom_picture_data_EXCHANGE_NED;
			break;

		case DIVOOM_DISP_EXCHANGE_POL:
			if(pic_len)
			{
				*pic_len = sizeof(divoom_picture_data_EXCHANGE_POL);
			}
			return (T_U8 *)divoom_picture_data_EXCHANGE_POL;
			break;

		case DIVOOM_DISP_EXCHANGE_SLO:
			if(pic_len)
			{
				*pic_len = sizeof(divoom_picture_data_EXCHANGE_SLO);
			}
			return (T_U8 *)divoom_picture_data_EXCHANGE_SLO;
			break;

		case DIVOOM_DISP_EXCHANGE_SVK:
			if(pic_len)
			{
				*pic_len = sizeof(divoom_picture_data_EXCHANGE_SVK);
			}
			return (T_U8 *)divoom_picture_data_EXCHANGE_SVK;
			break;

		case DIVOOM_DISP_EXCHANGE_SWE:
			if(pic_len)
			{
				*pic_len = sizeof(divoom_picture_data_EXCHANGE_SWE);
			}
			return (T_U8 *)divoom_picture_data_EXCHANGE_SWE;
			break;

		case DIVOOM_DISP_LOL_FAIL:
			if(pic_len)
			{
				*pic_len = sizeof(divoom_picture_data_lol_fail);
			}
			return (T_U8 *)divoom_picture_data_lol_fail;
			break;

		case DIVOOM_DISP_LOL_WIN:
			if(pic_len)
			{
				*pic_len = sizeof(divoom_picture_data_lol_win);
			}
			return (T_U8 *)divoom_picture_data_lol_win;
			break;

		case DIVOOM_DISP_APEX_KILL:
			if(pic_len)
			{
				*pic_len = sizeof(divoom_picture_data_apex_kill);
			}
			return (T_U8 *)divoom_picture_data_apex_kill;
			break;

		case DIVOOM_DISP_APEX_LEVEL_UP0000:
			if(pic_len)
			{
				*pic_len = sizeof(divoom_picture_data_apex_level_up0000);
			}
			return (T_U8 *)divoom_picture_data_apex_level_up0000;
			break;

		case DIVOOM_DISP_APEX_LEVEL_UP10000:
			if(pic_len)
			{
				*pic_len = sizeof(divoom_picture_data_apex_level_up10000);
			}
			return (T_U8 *)divoom_picture_data_apex_level_up10000;
			break;

		case DIVOOM_DISP_APEX_LEVEL_UP1200:
			if(pic_len)
			{
				*pic_len = sizeof(divoom_picture_data_apex_level_up1200);
			}
			return (T_U8 *)divoom_picture_data_apex_level_up1200;
			break;

		case DIVOOM_DISP_APEX_LEVEL_UP15000:
			if(pic_len)
			{
				*pic_len = sizeof(divoom_picture_data_apex_level_up15000);
			}
			return (T_U8 *)divoom_picture_data_apex_level_up15000;
			break;

		case DIVOOM_DISP_APEX_LEVEL_UP2800:
			if(pic_len)
			{
				*pic_len = sizeof(divoom_picture_data_apex_level_up2800);
			}
			return (T_U8 *)divoom_picture_data_apex_level_up2800;
			break;

		case DIVOOM_DISP_APEX_LEVEL_UP4800:
			if(pic_len)
			{
				*pic_len = sizeof(divoom_picture_data_apex_level_up4800);
			}
			return (T_U8 *)divoom_picture_data_apex_level_up4800;
			break;

		case DIVOOM_DISP_APEX_LEVEL_UP7200:
			if(pic_len)
			{
				*pic_len = sizeof(divoom_picture_data_apex_level_up7200);
			}
			return (T_U8 *)divoom_picture_data_apex_level_up7200;
			break;

		case DIVOOM_DISP_LOL_DEATH:
			if(pic_len)
			{
				*pic_len = sizeof(divoom_picture_data_lol_death);
			}
			return (T_U8 *)divoom_picture_data_lol_death;
			break;

		case DIVOOM_DISP_LOL_HELP:
			if(pic_len)
			{
				*pic_len = sizeof(divoom_picture_data_lol_help);
			}
			return (T_U8 *)divoom_picture_data_lol_help;
			break;

		case DIVOOM_DISP_LOL_KILL:
			if(pic_len)
			{
				*pic_len = sizeof(divoom_picture_data_lol_kill);
			}
			return (T_U8 *)divoom_picture_data_lol_kill;
			break;


		case DIVOOM_DISP_LOL_LEVEL:
			if(pic_len)
			{
				*pic_len = sizeof(divoom_picture_data_lol_level);
			}
			return (T_U8 *)divoom_picture_data_lol_level;
			break;

		case DIVOOM_DISP_LOL_MATCHS:
			if(pic_len)
			{
				*pic_len = sizeof(divoom_picture_data_lol_matchs);
			}
			return (T_U8 *)divoom_picture_data_lol_matchs;
			break;

		case DIVOOM_DISP_LOL_NEW:
			if(pic_len)
			{
				*pic_len = sizeof(divoom_picture_data_lol_new);
			}
			return (T_U8 *)divoom_picture_data_lol_new;
			break;


		case DIVOOM_DISP_WIFI_MODE_PIC:
			if(pic_len)
			{
				*pic_len = sizeof(divoom_picture_data_wifi_mode_pic);
			}
			return (uint8 *)divoom_picture_data_wifi_mode_pic;
			break;

		case DIVOOM_DISP_WIFI_OFF_PIC:
			if(pic_len)
			{
				*pic_len = sizeof(divoom_picture_data_wifi_off_pic);
			}
			return (uint8 *)divoom_picture_data_wifi_off_pic;
			break;


		case DIVOOM_DISP_LOADING_00:
			if(pic_len)
			{
				*pic_len = sizeof(divoom_picture_data_loading_00);
			}
			return (uint8 *)divoom_picture_data_loading_00;
			break;

		case DIVOOM_DISP_LOADING_01:
			if(pic_len)
			{
				*pic_len = sizeof(divoom_picture_data_loading_01);
			}
			return (uint8 *)divoom_picture_data_loading_01;
			break;

		case DIVOOM_DISP_LOADING_02:
			if(pic_len)
			{
				*pic_len = sizeof(divoom_picture_data_loading_02);
			}
			return (uint8 *)divoom_picture_data_loading_02;
			break;

		case DIVOOM_DISP_LOADING_03:
			if(pic_len)
			{
				*pic_len = sizeof(divoom_picture_data_loading_03);
			}
			return (uint8 *)divoom_picture_data_loading_03;
			break;

		case DIVOOM_DISP_LOADING_04:
			if(pic_len)
			{
				*pic_len = sizeof(divoom_picture_data_loading_04);
			}
			return (uint8 *)divoom_picture_data_loading_04;
			break;

		case DIVOOM_DISP_LOADING_05:
			if(pic_len)
			{
				*pic_len = sizeof(divoom_picture_data_loading_05);
			}
			return (uint8 *)divoom_picture_data_loading_05;
			break;

		case DIVOOM_DISP_LOADING_06:
			if(pic_len)
			{
				*pic_len = sizeof(divoom_picture_data_loading_06);
			}
			return (uint8 *)divoom_picture_data_loading_06;
			break;

		case DIVOOM_DISP_LOADING_07:
			if(pic_len)
			{
				*pic_len = sizeof(divoom_picture_data_loading_07);
			}
			return (uint8 *)divoom_picture_data_loading_07;
			break;
            
		case DIVOOM_DISP_DOWN_PIC2TV_00:
			if(pic_len)
			{
				*pic_len = sizeof(divoom_picture_data_down_pic2tv_00);
			}
			return (uint8 *)divoom_picture_data_down_pic2tv_00;
			break;

		case DIVOOM_DISP_DOWN_PIC2TV_01:
			if(pic_len)
			{
				*pic_len = sizeof(divoom_picture_data_down_pic2tv_01);
			}
			return (uint8 *)divoom_picture_data_down_pic2tv_01;
			break;

		case DIVOOM_DISP_DOWN_PIC2TV_02:
			if(pic_len)
			{
				*pic_len = sizeof(divoom_picture_data_down_pic2tv_02);
			}
			return (uint8 *)divoom_picture_data_down_pic2tv_02;
			break;

		case DIVOOM_DISP_DOWN_PIC2TV_03:
			if(pic_len)
			{
				*pic_len = sizeof(divoom_picture_data_down_pic2tv_03);
			}
			return (uint8 *)divoom_picture_data_down_pic2tv_03;
			break;

		case DIVOOM_DISP_DOWN_PIC2TV_04:
			if(pic_len)
			{
				*pic_len = sizeof(divoom_picture_data_down_pic2tv_04);
			}
			return (uint8 *)divoom_picture_data_down_pic2tv_04;
			break;

		case DIVOOM_DISP_DOWN_PIC2TV_05:
			if(pic_len)
			{
				*pic_len = sizeof(divoom_picture_data_down_pic2tv_05);
			}
			return (uint8 *)divoom_picture_data_down_pic2tv_05;
			break;

		case DIVOOM_DISP_DOWN_PIC2TV_06:
			if(pic_len)
			{
				*pic_len = sizeof(divoom_picture_data_down_pic2tv_06);
			}
			return (uint8 *)divoom_picture_data_down_pic2tv_06;
			break;

		case DIVOOM_DISP_DOWN_PIC2TV_07:
			if(pic_len)
			{
				*pic_len = sizeof(divoom_picture_data_down_pic2tv_07);
			}
			return (uint8 *)divoom_picture_data_down_pic2tv_07;
			break;

		case DIVOOM_DISP_DOWN_PIC2TV_08:
			if(pic_len)
			{
				*pic_len = sizeof(divoom_picture_data_down_pic2tv_08);
			}
			return (uint8 *)divoom_picture_data_down_pic2tv_08;
			break;

		case DIVOOM_DISP_DOWN_PIC2TV_09:
			if(pic_len)
			{
				*pic_len = sizeof(divoom_picture_data_down_pic2tv_09);
			}
			return (uint8 *)divoom_picture_data_down_pic2tv_09;
			break;

		case DIVOOM_DISP_DOWN_PIC2TV_10:
			if(pic_len)
			{
				*pic_len = sizeof(divoom_picture_data_down_pic2tv_10);
			}
			return (uint8 *)divoom_picture_data_down_pic2tv_10;
			break;

		case DIVOOM_DISP_DOWN_PIC2TV_11:
			if(pic_len)
			{
				*pic_len = sizeof(divoom_picture_data_down_pic2tv_11);
			}
			return (uint8 *)divoom_picture_data_down_pic2tv_11;
			break;


		case DIVOOM_DISP_ALARM_CLOCK_ADD_PIC:
			if(pic_len)
			{
				*pic_len = sizeof(divoom_picture_data_alarm_clock_add_pic);
			}
			return (uint8 *)divoom_picture_data_alarm_clock_add_pic;
			break;

		case DIVOOM_DISP_ALARM_CLOCK_DEL_PIC:
			if(pic_len)
			{
				*pic_len = sizeof(divoom_picture_data_alarm_clock_del_pic);
			}
			return (uint8 *)divoom_picture_data_alarm_clock_del_pic;
			break;

		case DIVOOM_DISP_ALARM_CLOCK_END_PIC:
			if(pic_len)
			{
				*pic_len = sizeof(divoom_picture_data_alarm_clock_end_pic);
			}
			return (uint8 *)divoom_picture_data_alarm_clock_end_pic;
			break;
            
		case DIVOOM_DISP_ALARM_SNOOZ_PIC:
			if(pic_len)
			{
				*pic_len = sizeof(divoom_picture_data_alarm_snooz_pic);
			}
			return (uint8 *)divoom_picture_data_alarm_snooz_pic;
			break;

		case DIVOOM_DISP_BOOT_SAVE_PIC:
			if(pic_len)
			{
				*pic_len = sizeof(divoom_picture_data_boot_save_pic);
			}
			return (uint8 *)divoom_picture_data_boot_save_pic;
			break;

		case DIVOOM_DISP_COUNT_DOWN_CANCEL:
			if(pic_len)
			{
				*pic_len = sizeof(divoom_picture_data_count_down_cancel);
			}
			return (uint8 *)divoom_picture_data_count_down_cancel;
			break;

		case DIVOOM_DISP_COUNT_DOWN_TIME_UP:
			if(pic_len)
			{
				*pic_len = sizeof(divoom_picture_data_count_down_time_up);
			}
			return (uint8 *)divoom_picture_data_count_down_time_up;
			break;


		case DIVOOM_DISP_FM_SEARCH_PIC:
			if(pic_len)
			{
				*pic_len = sizeof(divoom_picture_data_fm_search_pic);
			}
			return (uint8 *)divoom_picture_data_fm_search_pic;
			break;

		case DIVOOM_DISP_HOME_PAGE_HOT:
			if(pic_len)
			{
				*pic_len = sizeof(divoom_picture_data_home_page_hot);
			}
			return (uint8 *)divoom_picture_data_home_page_hot;
			break;
            


		case DIVOOM_DISP_NEW_STOPWATCH:
			if(pic_len)
			{
				*pic_len = sizeof(divoom_picture_data_new_stopwatch);
			}
			return (uint8 *)divoom_picture_data_new_stopwatch;
			break;

		case DIVOOM_DISP_PICTURE_DOWNLOAD1:
			if(pic_len)
			{
				*pic_len = sizeof(divoom_picture_data_picture_download1);
			}
			return (uint8 *)divoom_picture_data_picture_download1;
			break;

		case DIVOOM_DISP_PICTURE_DOWNLOAD2:
			if(pic_len)
			{
				*pic_len = sizeof(divoom_picture_data_picture_download2);
			}
			return (uint8 *)divoom_picture_data_picture_download2;
			break;

		case DIVOOM_DISP_PICTURE_DOWNLOAD3:
			if(pic_len)
			{
				*pic_len = sizeof(divoom_picture_data_picture_download3);
			}
			return (uint8 *)divoom_picture_data_picture_download3;
			break;

		case DIVOOM_DISP_PICTURE_DOWNLOAD4:
			if(pic_len)
			{
				*pic_len = sizeof(divoom_picture_data_picture_download4);
			}
			return (uint8 *)divoom_picture_data_picture_download4;
			break;

		case DIVOOM_DISP_PICTURE_DOWNLOAD5:
			if(pic_len)
			{
				*pic_len = sizeof(divoom_picture_data_picture_download5);
			}
			return (uint8 *)divoom_picture_data_picture_download5;
			break;

		case DIVOOM_DISP_PICTURE_DOWNLOAD6:
			if(pic_len)
			{
				*pic_len = sizeof(divoom_picture_data_picture_download6);
			}
			return (uint8 *)divoom_picture_data_picture_download6;
			break;

		case DIVOOM_DISP_PICTURE_DOWNLOAD7:
			if(pic_len)
			{
				*pic_len = sizeof(divoom_picture_data_picture_download7);
			}
			return (uint8 *)divoom_picture_data_picture_download7;
			break;

		case DIVOOM_DISP_PICTURE_DOWNLOAD8:
			if(pic_len)
			{
				*pic_len = sizeof(divoom_picture_data_picture_download8);
			}
			return (uint8 *)divoom_picture_data_picture_download8;
			break;

		case DIVOOM_DISP_PICTURE_DOWNLOAD_ERR:
			if(pic_len)
			{
				*pic_len = sizeof(divoom_picture_data_picture_download_err);
			}
			return (uint8 *)divoom_picture_data_picture_download_err;
			break;



		case DIVOOM_DISP_UPDATE_PIC:
			if(pic_len)
			{
				*pic_len = sizeof(divoom_picture_data_update_pic);
			}
			return (uint8 *)divoom_picture_data_update_pic;
			break;

		case DIVOOM_DISP_USER_DEFINE_PIC:
			if(pic_len)
			{
				*pic_len = sizeof(divoom_picture_data_user_define_pic);
			}
			return (uint8 *)divoom_picture_data_user_define_pic;
			break;

		case DIVOOM_DISP_USER_DEFINE_TV_PIC:
			if(pic_len)
			{
				*pic_len = sizeof(divoom_picture_data_user_define_tv_pic);
			}
			return (uint8 *)divoom_picture_data_user_define_tv_pic;
			break;


		case DIVOOM_DISP_NEW_STOPWATCH_39:
			if(pic_len)
			{
				*pic_len = sizeof(divoom_picture_data_new_stopwatch_39);
			}
			return (uint8 *)divoom_picture_data_new_stopwatch_39;
			break;

		case DIVOOM_DISP_NEW_STOPWATCH_18:
			if(pic_len)
			{
				*pic_len = sizeof(divoom_picture_data_new_stopwatch_18);
			}
			return (uint8 *)divoom_picture_data_new_stopwatch_18;
			break;

		case DIVOOM_DISP_NEW_NOISE_8:
			if(pic_len)
			{
				*pic_len = sizeof(divoom_picture_data_new_noise_8);
			}
			return (uint8 *)divoom_picture_data_new_noise_8;
			break;

		case DIVOOM_DISP_NEW_STOPWATCH_20:
			if(pic_len)
			{
				*pic_len = sizeof(divoom_picture_data_new_stopwatch_20);
			}
			return (uint8 *)divoom_picture_data_new_stopwatch_20;
			break;

		case DIVOOM_DISP_NEW_STOPWATCH_4:
			if(pic_len)
			{
				*pic_len = sizeof(divoom_picture_data_new_stopwatch_4);
			}
			return (uint8 *)divoom_picture_data_new_stopwatch_4;
			break;

		case DIVOOM_DISP_NEW_NOISE_2:
			if(pic_len)
			{
				*pic_len = sizeof(divoom_picture_data_new_noise_2);
			}
			return (uint8 *)divoom_picture_data_new_noise_2;
			break;



		case DIVOOM_DISP_NEW_STOPWATCH_58:
			if(pic_len)
			{
				*pic_len = sizeof(divoom_picture_data_new_stopwatch_58);
			}
			return (uint8 *)divoom_picture_data_new_stopwatch_58;
			break;

		case DIVOOM_DISP_TIMEROVER_NO:
			if(pic_len)
			{
				*pic_len = sizeof(divoom_picture_data_timerover_no);
			}
			return (uint8 *)divoom_picture_data_timerover_no;
			break;


		case DIVOOM_DISP_NEW_NOISE_16:
			if(pic_len)
			{
				*pic_len = sizeof(divoom_picture_data_new_noise_16);
			}
			return (uint8 *)divoom_picture_data_new_noise_16;
			break;

		case DIVOOM_DISP_NEW_STOPWATCH_13:
			if(pic_len)
			{
				*pic_len = sizeof(divoom_picture_data_new_stopwatch_13);
			}
			return (uint8 *)divoom_picture_data_new_stopwatch_13;
			break;
            
		case DIVOOM_DISP_TIMER_ALL_7:
			if(pic_len)
			{
				*pic_len = sizeof(divoom_picture_data_timer_all_7);
			}
			return (uint8 *)divoom_picture_data_timer_all_7;
			break;

		case DIVOOM_DISP_TIMER_ALL_2:
			if(pic_len)
			{
				*pic_len = sizeof(divoom_picture_data_timer_all_2);
			}
			return (uint8 *)divoom_picture_data_timer_all_2;
			break;



		case DIVOOM_DISP_NEW_STOPWATCH_34:
			if(pic_len)
			{
				*pic_len = sizeof(divoom_picture_data_new_stopwatch_34);
			}
			return (uint8 *)divoom_picture_data_new_stopwatch_34;
			break;


		case DIVOOM_DISP_NEW_STOPWATCH_55:
			if(pic_len)
			{
				*pic_len = sizeof(divoom_picture_data_new_stopwatch_55);
			}
			return (uint8 *)divoom_picture_data_new_stopwatch_55;
			break;

		case DIVOOM_DISP_NEW_STOPWATCH_43:
			if(pic_len)
			{
				*pic_len = sizeof(divoom_picture_data_new_stopwatch_43);
			}
			return (uint8 *)divoom_picture_data_new_stopwatch_43;
			break;

		case DIVOOM_DISP_TIMER_ALL_5:
			if(pic_len)
			{
				*pic_len = sizeof(divoom_picture_data_timer_all_5);
			}
			return (uint8 *)divoom_picture_data_timer_all_5;
			break;

		case DIVOOM_DISP_TIMER_ALL_8:
			if(pic_len)
			{
				*pic_len = sizeof(divoom_picture_data_timer_all_8);
			}
			return (uint8 *)divoom_picture_data_timer_all_8;
			break;


		case DIVOOM_DISP_NEW_STOPWATCH_46:
			if(pic_len)
			{
				*pic_len = sizeof(divoom_picture_data_new_stopwatch_46);
			}
			return (uint8 *)divoom_picture_data_new_stopwatch_46;
			break;


		case DIVOOM_DISP_TIMER_ALL_19:
			if(pic_len)
			{
				*pic_len = sizeof(divoom_picture_data_timer_all_19);
			}
			return (uint8 *)divoom_picture_data_timer_all_19;
			break;


		case DIVOOM_DISP_NEW_NOISE_17:
			if(pic_len)
			{
				*pic_len = sizeof(divoom_picture_data_new_noise_17);
			}
			return (uint8 *)divoom_picture_data_new_noise_17;
			break;

		case DIVOOM_DISP_NEW_STOPWATCH_23:
			if(pic_len)
			{
				*pic_len = sizeof(divoom_picture_data_new_stopwatch_23);
			}
			return (uint8 *)divoom_picture_data_new_stopwatch_23;
			break;

		case DIVOOM_DISP_NEW_STOPWATCH_48:
			if(pic_len)
			{
				*pic_len = sizeof(divoom_picture_data_new_stopwatch_48);
			}
			return (uint8 *)divoom_picture_data_new_stopwatch_48;
			break;

		case DIVOOM_DISP_NEW_STOPWATCH_32:
			if(pic_len)
			{
				*pic_len = sizeof(divoom_picture_data_new_stopwatch_32);
			}
			return (uint8 *)divoom_picture_data_new_stopwatch_32;
			break;

		case DIVOOM_DISP_NEW_STOPWATCH_38:
			if(pic_len)
			{
				*pic_len = sizeof(divoom_picture_data_new_stopwatch_38);
			}
			return (uint8 *)divoom_picture_data_new_stopwatch_38;
			break;

		case DIVOOM_DISP_NEW_STOPWATCH_11:
			if(pic_len)
			{
				*pic_len = sizeof(divoom_picture_data_new_stopwatch_11);
			}
			return (uint8 *)divoom_picture_data_new_stopwatch_11;
			break;

		case DIVOOM_DISP_NEW_STOPWATCH_29:
			if(pic_len)
			{
				*pic_len = sizeof(divoom_picture_data_new_stopwatch_29);
			}
			return (uint8 *)divoom_picture_data_new_stopwatch_29;
			break;



		case DIVOOM_DISP_NEW_STOPWATCH_42:
			if(pic_len)
			{
				*pic_len = sizeof(divoom_picture_data_new_stopwatch_42);
			}
			return (uint8 *)divoom_picture_data_new_stopwatch_42;
			break;

		case DIVOOM_DISP_NEW_STOPWATCH_16:
			if(pic_len)
			{
				*pic_len = sizeof(divoom_picture_data_new_stopwatch_16);
			}
			return (uint8 *)divoom_picture_data_new_stopwatch_16;
			break;


		case DIVOOM_DISP_NEW_NOISE_11:
			if(pic_len)
			{
				*pic_len = sizeof(divoom_picture_data_new_noise_11);
			}
			return (uint8 *)divoom_picture_data_new_noise_11;
			break;

		case DIVOOM_DISP_TIMER_ALL_4:
			if(pic_len)
			{
				*pic_len = sizeof(divoom_picture_data_timer_all_4);
			}
			return (uint8 *)divoom_picture_data_timer_all_4;
			break;

		case DIVOOM_DISP_NEW_STOPWATCH_25:
			if(pic_len)
			{
				*pic_len = sizeof(divoom_picture_data_new_stopwatch_25);
			}
			return (uint8 *)divoom_picture_data_new_stopwatch_25;
			break;


		case DIVOOM_DISP_NEW_STOPWATCH_9:
			if(pic_len)
			{
				*pic_len = sizeof(divoom_picture_data_new_stopwatch_9);
			}
			return (uint8 *)divoom_picture_data_new_stopwatch_9;
			break;

		case DIVOOM_DISP_NEW_STOPWATCH_7:
			if(pic_len)
			{
				*pic_len = sizeof(divoom_picture_data_new_stopwatch_7);
			}
			return (uint8 *)divoom_picture_data_new_stopwatch_7;
			break;

		case DIVOOM_DISP_NEW_STOPWATCH_53:
			if(pic_len)
			{
				*pic_len = sizeof(divoom_picture_data_new_stopwatch_53);
			}
			return (uint8 *)divoom_picture_data_new_stopwatch_53;
			break;

		case DIVOOM_DISP_NEW_STOPWATCH_1:
			if(pic_len)
			{
				*pic_len = sizeof(divoom_picture_data_new_stopwatch_1);
			}
			return (uint8 *)divoom_picture_data_new_stopwatch_1;
			break;

		case DIVOOM_DISP_TIMER_ALL_16:
			if(pic_len)
			{
				*pic_len = sizeof(divoom_picture_data_timer_all_16);
			}
			return (uint8 *)divoom_picture_data_timer_all_16;
			break;

		case DIVOOM_DISP_NEW_STOPWATCH_31:
			if(pic_len)
			{
				*pic_len = sizeof(divoom_picture_data_new_stopwatch_31);
			}
			return (uint8 *)divoom_picture_data_new_stopwatch_31;
			break;

		case DIVOOM_DISP_TIMER_ALL_13:
			if(pic_len)
			{
				*pic_len = sizeof(divoom_picture_data_timer_all_13);
			}
			return (uint8 *)divoom_picture_data_timer_all_13;
			break;

		case DIVOOM_DISP_NEW_STOPWATCH_6:
			if(pic_len)
			{
				*pic_len = sizeof(divoom_picture_data_new_stopwatch_6);
			}
			return (uint8 *)divoom_picture_data_new_stopwatch_6;
			break;

		case DIVOOM_DISP_NEW_NOISE_9:
			if(pic_len)
			{
				*pic_len = sizeof(divoom_picture_data_new_noise_9);
			}
			return (uint8 *)divoom_picture_data_new_noise_9;
			break;

		case DIVOOM_DISP_NEW_NOISE_10:
			if(pic_len)
			{
				*pic_len = sizeof(divoom_picture_data_new_noise_10);
			}
			return (uint8 *)divoom_picture_data_new_noise_10;
			break;

		case DIVOOM_DISP_NEW_NOISE_7:
			if(pic_len)
			{
				*pic_len = sizeof(divoom_picture_data_new_noise_7);
			}
			return (uint8 *)divoom_picture_data_new_noise_7;
			break;

		case DIVOOM_DISP_DIGIT_1_PIC:
			if(pic_len)
			{
				*pic_len = sizeof(divoom_picture_data_digit_1_pic);
			}
			return (T_U8 *)divoom_picture_data_digit_1_pic;
			break;

		case DIVOOM_DISP_DIGIT_2_PIC:
			if(pic_len)
			{
				*pic_len = sizeof(divoom_picture_data_digit_2_pic);
			}
			return (T_U8 *)divoom_picture_data_digit_2_pic;
			break;

		case DIVOOM_DISP_DIGIT_3_PIC:
			if(pic_len)
			{
				*pic_len = sizeof(divoom_picture_data_digit_3_pic);
			}
			return (T_U8 *)divoom_picture_data_digit_3_pic;
			break;

		case DIVOOM_DISP_DIGIT_4_PIC:
			if(pic_len)
			{
				*pic_len = sizeof(divoom_picture_data_digit_4_pic);
			}
			return (T_U8 *)divoom_picture_data_digit_4_pic;
			break;

		case DIVOOM_DISP_DIGIT_5_PIC:
			if(pic_len)
			{
				*pic_len = sizeof(divoom_picture_data_digit_5_pic);
			}
			return (T_U8 *)divoom_picture_data_digit_5_pic;
			break;


		case DIVOOM_DISP_NEW_STOPWATCH_17:
			if(pic_len)
			{
				*pic_len = sizeof(divoom_picture_data_new_stopwatch_17);
			}
			return (uint8 *)divoom_picture_data_new_stopwatch_17;
			break;

		case DIVOOM_DISP_NEW_STOPWATCH_22:
			if(pic_len)
			{
				*pic_len = sizeof(divoom_picture_data_new_stopwatch_22);
			}
			return (uint8 *)divoom_picture_data_new_stopwatch_22;
			break;


		case DIVOOM_DISP_TIMER_ALL_21:
			if(pic_len)
			{
				*pic_len = sizeof(divoom_picture_data_timer_all_21);
			}
			return (uint8 *)divoom_picture_data_timer_all_21;
			break;

		case DIVOOM_DISP_NEW_STOPWATCH_14:
			if(pic_len)
			{
				*pic_len = sizeof(divoom_picture_data_new_stopwatch_14);
			}
			return (uint8 *)divoom_picture_data_new_stopwatch_14;
			break;

		case DIVOOM_DISP_NEW_STOPWATCH_2:
			if(pic_len)
			{
				*pic_len = sizeof(divoom_picture_data_new_stopwatch_2);
			}
			return (uint8 *)divoom_picture_data_new_stopwatch_2;
			break;

		case DIVOOM_DISP_TIMER_ALL_14:
			if(pic_len)
			{
				*pic_len = sizeof(divoom_picture_data_timer_all_14);
			}
			return (uint8 *)divoom_picture_data_timer_all_14;
			break;

		case DIVOOM_DISP_NEW_STOPWATCH_50:
			if(pic_len)
			{
				*pic_len = sizeof(divoom_picture_data_new_stopwatch_50);
			}
			return (uint8 *)divoom_picture_data_new_stopwatch_50;
			break;


		case DIVOOM_DISP_NEW_NOISE_6:
			if(pic_len)
			{
				*pic_len = sizeof(divoom_picture_data_new_noise_6);
			}
			return (uint8 *)divoom_picture_data_new_noise_6;
			break;

		case DIVOOM_DISP_NEW_STOPWATCH_3:
			if(pic_len)
			{
				*pic_len = sizeof(divoom_picture_data_new_stopwatch_3);
			}
			return (uint8 *)divoom_picture_data_new_stopwatch_3;
			break;

		case DIVOOM_DISP_NEW_STOPWATCH_41:
			if(pic_len)
			{
				*pic_len = sizeof(divoom_picture_data_new_stopwatch_41);
			}
			return (uint8 *)divoom_picture_data_new_stopwatch_41;
			break;

		case DIVOOM_DISP_NEW_STOPWATCH_0:
			if(pic_len)
			{
				*pic_len = sizeof(divoom_picture_data_new_stopwatch_0);
			}
			return (uint8 *)divoom_picture_data_new_stopwatch_0;
			break;

		case DIVOOM_DISP_TIMER_ALL_18:
			if(pic_len)
			{
				*pic_len = sizeof(divoom_picture_data_timer_all_18);
			}
			return (uint8 *)divoom_picture_data_timer_all_18;
			break;

		case DIVOOM_DISP_NEW_STOPWATCH_12:
			if(pic_len)
			{
				*pic_len = sizeof(divoom_picture_data_new_stopwatch_12);
			}
			return (uint8 *)divoom_picture_data_new_stopwatch_12;
			break;

		case DIVOOM_DISP_NEW_STOPWATCH_8:
			if(pic_len)
			{
				*pic_len = sizeof(divoom_picture_data_new_stopwatch_8);
			}
			return (uint8 *)divoom_picture_data_new_stopwatch_8;
			break;


		case DIVOOM_DISP_NEW_STOPWATCH_24:
			if(pic_len)
			{
				*pic_len = sizeof(divoom_picture_data_new_stopwatch_24);
			}
			return (uint8 *)divoom_picture_data_new_stopwatch_24;
			break;


		case DIVOOM_DISP_NEW_NOISE_12:
			if(pic_len)
			{
				*pic_len = sizeof(divoom_picture_data_new_noise_12);
			}
			return (uint8 *)divoom_picture_data_new_noise_12;
			break;

		case DIVOOM_DISP_NEW_NOISE_15:
			if(pic_len)
			{
				*pic_len = sizeof(divoom_picture_data_new_noise_15);
			}
			return (uint8 *)divoom_picture_data_new_noise_15;
			break;

		case DIVOOM_DISP_NEW_NOISE_13:
			if(pic_len)
			{
				*pic_len = sizeof(divoom_picture_data_new_noise_13);
			}
			return (uint8 *)divoom_picture_data_new_noise_13;
			break;


		case DIVOOM_DISP_NEW_NOISE_5:
			if(pic_len)
			{
				*pic_len = sizeof(divoom_picture_data_new_noise_5);
			}
			return (uint8 *)divoom_picture_data_new_noise_5;
			break;



		case DIVOOM_DISP_NEW_STOPWATCH_47:
			if(pic_len)
			{
				*pic_len = sizeof(divoom_picture_data_new_stopwatch_47);
			}
			return (uint8 *)divoom_picture_data_new_stopwatch_47;
			break;

		case DIVOOM_DISP_NEW_NOISE_0:
			if(pic_len)
			{
				*pic_len = sizeof(divoom_picture_data_new_noise_0);
			}
			return (uint8 *)divoom_picture_data_new_noise_0;
			break;


		case DIVOOM_DISP_TIMER_ALL_1:
			if(pic_len)
			{
				*pic_len = sizeof(divoom_picture_data_timer_all_1);
			}
			return (uint8 *)divoom_picture_data_timer_all_1;
			break;


		case DIVOOM_DISP_NEW_STOPWATCH_33:
			if(pic_len)
			{
				*pic_len = sizeof(divoom_picture_data_new_stopwatch_33);
			}
			return (uint8 *)divoom_picture_data_new_stopwatch_33;
			break;


		case DIVOOM_DISP_TIMER_ALL_20:
			if(pic_len)
			{
				*pic_len = sizeof(divoom_picture_data_timer_all_20);
			}
			return (uint8 *)divoom_picture_data_timer_all_20;
			break;

		case DIVOOM_DISP_TIMER_ALL_22:
			if(pic_len)
			{
				*pic_len = sizeof(divoom_picture_data_timer_all_22);
			}
			return (uint8 *)divoom_picture_data_timer_all_22;
			break;

		case DIVOOM_DISP_NEW_STOPWATCH_59:
			if(pic_len)
			{
				*pic_len = sizeof(divoom_picture_data_new_stopwatch_59);
			}
			return (uint8 *)divoom_picture_data_new_stopwatch_59;
			break;

		case DIVOOM_DISP_NEW_NOISE_4:
			if(pic_len)
			{
				*pic_len = sizeof(divoom_picture_data_new_noise_4);
			}
			return (uint8 *)divoom_picture_data_new_noise_4;
			break;

		case DIVOOM_DISP_TIMER_ALL_17:
			if(pic_len)
			{
				*pic_len = sizeof(divoom_picture_data_timer_all_17);
			}
			return (uint8 *)divoom_picture_data_timer_all_17;
			break;

		case DIVOOM_DISP_TIMER_ALL_0:
			if(pic_len)
			{
				*pic_len = sizeof(divoom_picture_data_timer_all_0);
			}
			return (uint8 *)divoom_picture_data_timer_all_0;
			break;


		case DIVOOM_DISP_TIMER_ALL_6:
			if(pic_len)
			{
				*pic_len = sizeof(divoom_picture_data_timer_all_6);
			}
			return (uint8 *)divoom_picture_data_timer_all_6;
			break;

		case DIVOOM_DISP_NEW_STOPWATCH_30:
			if(pic_len)
			{
				*pic_len = sizeof(divoom_picture_data_new_stopwatch_30);
			}
			return (uint8 *)divoom_picture_data_new_stopwatch_30;
			break;

		case DIVOOM_DISP_NEW_STOPWATCH_27:
			if(pic_len)
			{
				*pic_len = sizeof(divoom_picture_data_new_stopwatch_27);
			}
			return (uint8 *)divoom_picture_data_new_stopwatch_27;
			break;

		case DIVOOM_DISP_TIMEROVER:
			if(pic_len)
			{
				*pic_len = sizeof(divoom_picture_data_timerover);
			}
			return (uint8 *)divoom_picture_data_timerover;
			break;

		case DIVOOM_DISP_NEW_STOPWATCH_45:
			if(pic_len)
			{
				*pic_len = sizeof(divoom_picture_data_new_stopwatch_45);
			}
			return (uint8 *)divoom_picture_data_new_stopwatch_45;
			break;

		case DIVOOM_DISP_TIMER_ALL_11:
			if(pic_len)
			{
				*pic_len = sizeof(divoom_picture_data_timer_all_11);
			}
			return (uint8 *)divoom_picture_data_timer_all_11;
			break;

		case DIVOOM_DISP_NEW_SCORE:
			if(pic_len)
			{
				*pic_len = sizeof(divoom_picture_data_new_score);
			}
			return (uint8 *)divoom_picture_data_new_score;
			break;

		case DIVOOM_DISP_TIMER_ALL_3:
			if(pic_len)
			{
				*pic_len = sizeof(divoom_picture_data_timer_all_3);
			}
			return (uint8 *)divoom_picture_data_timer_all_3;
			break;

		case DIVOOM_DISP_TIMER_ALL_15:
			if(pic_len)
			{
				*pic_len = sizeof(divoom_picture_data_timer_all_15);
			}
			return (uint8 *)divoom_picture_data_timer_all_15;
			break;


		case DIVOOM_DISP_NEW_STOPWATCH_35:
			if(pic_len)
			{
				*pic_len = sizeof(divoom_picture_data_new_stopwatch_35);
			}
			return (uint8 *)divoom_picture_data_new_stopwatch_35;
			break;

		case DIVOOM_DISP_NEW_STOPWATCH_28:
			if(pic_len)
			{
				*pic_len = sizeof(divoom_picture_data_new_stopwatch_28);
			}
			return (uint8 *)divoom_picture_data_new_stopwatch_28;
			break;

		case DIVOOM_DISP_NEW_STOPWATCH_49:
			if(pic_len)
			{
				*pic_len = sizeof(divoom_picture_data_new_stopwatch_49);
			}
			return (uint8 *)divoom_picture_data_new_stopwatch_49;
			break;

		case DIVOOM_DISP_NEW_ALARM:
			if(pic_len)
			{
				*pic_len = sizeof(divoom_picture_data_new_alarm);
			}
			return (uint8 *)divoom_picture_data_new_alarm;
			break;


		case DIVOOM_DISP_NEW_STOPWATCH_10:
			if(pic_len)
			{
				*pic_len = sizeof(divoom_picture_data_new_stopwatch_10);
			}
			return (uint8 *)divoom_picture_data_new_stopwatch_10;
			break;


		case DIVOOM_DISP_TIMER_ALL_10:
			if(pic_len)
			{
				*pic_len = sizeof(divoom_picture_data_timer_all_10);
			}
			return (uint8 *)divoom_picture_data_timer_all_10;
			break;

		case DIVOOM_DISP_NEW_STOPWATCH_5:
			if(pic_len)
			{
				*pic_len = sizeof(divoom_picture_data_new_stopwatch_5);
			}
			return (uint8 *)divoom_picture_data_new_stopwatch_5;
			break;

		case DIVOOM_DISP_TIMER_CANCEL:
			if(pic_len)
			{
				*pic_len = sizeof(divoom_picture_data_timer_cancel);
			}
			return (uint8 *)divoom_picture_data_timer_cancel;
			break;

		case DIVOOM_DISP_NEW_STOPWATCH_54:
			if(pic_len)
			{
				*pic_len = sizeof(divoom_picture_data_new_stopwatch_54);
			}
			return (uint8 *)divoom_picture_data_new_stopwatch_54;
			break;


		case DIVOOM_DISP_NEW_STOPWATCH_37:
			if(pic_len)
			{
				*pic_len = sizeof(divoom_picture_data_new_stopwatch_37);
			}
			return (uint8 *)divoom_picture_data_new_stopwatch_37;
			break;

		case DIVOOM_DISP_NEW_STOPWATCH_44:
			if(pic_len)
			{
				*pic_len = sizeof(divoom_picture_data_new_stopwatch_44);
			}
			return (uint8 *)divoom_picture_data_new_stopwatch_44;
			break;

		case DIVOOM_DISP_NEW_STOPWATCH_56:
			if(pic_len)
			{
				*pic_len = sizeof(divoom_picture_data_new_stopwatch_56);
			}
			return (uint8 *)divoom_picture_data_new_stopwatch_56;
			break;


		case DIVOOM_DISP_NEW_STOPWATCH_15:
			if(pic_len)
			{
				*pic_len = sizeof(divoom_picture_data_new_stopwatch_15);
			}
			return (uint8 *)divoom_picture_data_new_stopwatch_15;
			break;


		case DIVOOM_DISP_NEW_STOPWATCH_51:
			if(pic_len)
			{
				*pic_len = sizeof(divoom_picture_data_new_stopwatch_51);
			}
			return (uint8 *)divoom_picture_data_new_stopwatch_51;
			break;

		case DIVOOM_DISP_NEW_STOPWATCH_52:
			if(pic_len)
			{
				*pic_len = sizeof(divoom_picture_data_new_stopwatch_52);
			}
			return (uint8 *)divoom_picture_data_new_stopwatch_52;
			break;

		case DIVOOM_DISP_TIMERCANCEL:
			if(pic_len)
			{
				*pic_len = sizeof(divoom_picture_data_timercancel);
			}
			return (uint8 *)divoom_picture_data_timercancel;
			break;

		case DIVOOM_DISP_NEW_STOPWATCH_57:
			if(pic_len)
			{
				*pic_len = sizeof(divoom_picture_data_new_stopwatch_57);
			}
			return (uint8 *)divoom_picture_data_new_stopwatch_57;
			break;

		case DIVOOM_DISP_NEW_STOPWATCH_19:
			if(pic_len)
			{
				*pic_len = sizeof(divoom_picture_data_new_stopwatch_19);
			}
			return (uint8 *)divoom_picture_data_new_stopwatch_19;
			break;


		case DIVOOM_DISP_TIMER_ALL_12:
			if(pic_len)
			{
				*pic_len = sizeof(divoom_picture_data_timer_all_12);
			}
			return (uint8 *)divoom_picture_data_timer_all_12;
			break;


		case DIVOOM_DISP_NEW_NOISE_3:
			if(pic_len)
			{
				*pic_len = sizeof(divoom_picture_data_new_noise_3);
			}
			return (uint8 *)divoom_picture_data_new_noise_3;
			break;


		case DIVOOM_DISP_TIMER_ALL_9:
			if(pic_len)
			{
				*pic_len = sizeof(divoom_picture_data_timer_all_9);
			}
			return (uint8 *)divoom_picture_data_timer_all_9;
			break;

		case DIVOOM_DISP_NEW_STOPWATCH_40:
			if(pic_len)
			{
				*pic_len = sizeof(divoom_picture_data_new_stopwatch_40);
			}
			return (uint8 *)divoom_picture_data_new_stopwatch_40;
			break;

		case DIVOOM_DISP_NEW_NOISE_14:
			if(pic_len)
			{
				*pic_len = sizeof(divoom_picture_data_new_noise_14);
			}
			return (uint8 *)divoom_picture_data_new_noise_14;
			break;


		case DIVOOM_DISP_NEW_STOPWATCH_36:
			if(pic_len)
			{
				*pic_len = sizeof(divoom_picture_data_new_stopwatch_36);
			}
			return (uint8 *)divoom_picture_data_new_stopwatch_36;
			break;

		case DIVOOM_DISP_NEW_STOPWATCH_26:
			if(pic_len)
			{
				*pic_len = sizeof(divoom_picture_data_new_stopwatch_26);
			}
			return (uint8 *)divoom_picture_data_new_stopwatch_26;
			break;


		case DIVOOM_DISP_NEW_NOISE_1:
			if(pic_len)
			{
				*pic_len = sizeof(divoom_picture_data_new_noise_1);
			}
			return (uint8 *)divoom_picture_data_new_noise_1;
			break;

		case DIVOOM_DISP_NEW_STOPWATCH_21:
			if(pic_len)
			{
				*pic_len = sizeof(divoom_picture_data_new_stopwatch_21);
			}
			return (uint8 *)divoom_picture_data_new_stopwatch_21;
			break;

		case DIVOOM_DISP_TIMERCANCEL_NO:
			if(pic_len)
			{
				*pic_len = sizeof(divoom_picture_data_timercancel_no);
			}
			return (uint8 *)divoom_picture_data_timercancel_no;
			break;


		default:
            divoom_print("mode: %d\n", mode);
			return NULL;
	}
	return NULL;
}










