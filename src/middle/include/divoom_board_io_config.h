#ifndef _DIVOOM_BOARD_IO_CONFIG_H_
#define _DIVOOM_BOARD_IO_CONFIG_H_

#define DIVOOM_HGD_BOARD_V1_0

//#define DIVOOM_YU_ZHONG

//#define DIVOOM_CONNECT_OLD_USE_DISK


#define		DIVOOM_KEY_LED_PIN				(44)

#ifdef DIVOOM_HGD_BOARD_V1_0
//这个文件作为所有的io口配置目录，避免多次自定义不好修改
//唤醒key，GPIO(DIVOOM_ONOFF_CHECK_KEY)
#define		DIVOOM_ONOFF_CHECK_KEY			(-1)
#define		DIVOOM_ONOFF_CHECK_MODE_KEY		(-1)
//#define		DIVOOM_ONOFF_POWERON_POWER		(13)
#define		DIVOOM_ONOFF_POWERON_POWER		(-1)
//#define		DIVOOM_ONOFF_POWEROFF_POWER		(12)
#ifdef DIVOOM_CONNECT_OLD_USE_DISK

#define		DIVOOM_KEY_ADC_PIN				(38)

//蜂呜器
#define DIVOOM_BUZZER_PWM_PIN				(21)
#else

#define		DIVOOM_KEY_ADC_PIN				(8)

//蜂呜器
#define DIVOOM_BUZZER_PWM_PIN				(9)

#endif

//断码屏
#define DIVOOM_LEDSEG_CLK_PIN				(-1)
#define DIVOOM_LEDSEG_DAT_PIN				(-1)
#define DIVOOM_LEDSEG_STB_PIN				(-1)


//led
#define		DIVOOM_LCD_PWM_GPIO				39


#define DIVOOM_LCD1_CS_GPIO        (40)		//12,13,14
#define DIVOOM_LCD2_CS_GPIO        (46)		//12,13,14
#define DIVOOM_LCD3_CS_GPIO        (10)		//12,13,14
#define DIVOOM_LCD4_CS_GPIO        (18)		//12,13,14
#define DIVOOM_LCD5_CS_GPIO        (17)		//12,13,14
#define DIVOOM_LCD_CS_GPIO        (40)		//12,13,14


#define DIVOOM_LCD_BK_LIGHT_GPIO  (39)
#define DIVOOM_LCD_RST_GPIO       (41)
#define DIVOOM_LCD_DC_GPIO        (42)
#define DIVOOM_LCD_PCLK_GPIO      (38)

#define DIVOOM_LCD_RD_GPIO		(-1)	//(15)

#define DIVOOM_LCD_DATA0_GPIO     (5)
#define DIVOOM_LCD_DATA1_GPIO     (16)
#define DIVOOM_LCD_DATA2_GPIO     (4)
#define DIVOOM_LCD_DATA3_GPIO     (15)
#define DIVOOM_LCD_DATA4_GPIO     (7)
#define DIVOOM_LCD_DATA5_GPIO     (6)
#define DIVOOM_LCD_DATA6_GPIO     (2)
#define DIVOOM_LCD_DATA7_GPIO     (1)


//单线数据LED灯
#define DIVOOM_ONELINE_LED_SPI_PIN_NUM_MOSI	(0)

#define DIVOOM_USBDET_PIN					(-1)
#define DIVOOM_LED1_PIN						(-1)
#define DIVOOM_LED2_PIN						(-1)

//天花板和VJ都用串口传输了

//divoom_spi.c start
#define DIVOOM_SPI_PIN_NUM_CLK				(-1)
#define DIVOOM_SPI_PIN_NUM_MOSI				(-1)
#define DIVOOM_SPI_PIN_NUM_MISO				(-1)
#define DIVOOM_SPI_PIN_NUM_CS				(-1)

#ifdef DIVOOM_CONNECT_OLD_USE_DISK

//divoom_spi.c end

#define		DIVOOM_I2S_CHIP_EN				(-1)
#define		DIVOOM_I2S_BCK_IO				(48)
#define		DIVOOM_I2S_DATAIN_IO			(47)
#define		DIVOOM_I2S_DATA_OUT_IO			(-1)
#define		DIVOOM_I2S_WS_IO				(45)


//sd use start
#define		DIVOOM_SD_DETECT_IO				(-1)
#define		DIVOOM_SD_D0_IO					(40)
#define		DIVOOM_SD_D1_IO					(41)
#define		DIVOOM_SD_D2_IO					(42)
#define		DIVOOM_SD_D3_IO					(2)
#define		DIVOOM_SD_CLK_IO				(1)
#define		DIVOOM_SD_CMD_IO				(39)
//sd use end
#else
//i2s use
#define		DIVOOM_I2S_CHIP_EN				(-1)
#define		DIVOOM_I2S_BCK_IO				(19)
#define		DIVOOM_I2S_DATAIN_IO			(20)
#define		DIVOOM_I2S_DATA_OUT_IO			(-1)
#define		DIVOOM_I2S_WS_IO				(3)
//i2s use

//sd use start
#define		DIVOOM_SD_DETECT_IO				(-1)
#define		DIVOOM_SD_D0_IO					(14)
#define		DIVOOM_SD_D1_IO					(21)
#define		DIVOOM_SD_D2_IO					(45)
#define		DIVOOM_SD_D3_IO					(48)
#define		DIVOOM_SD_CLK_IO				(47)
#define		DIVOOM_SD_CMD_IO				(13)
//sd use end


#endif

//i2c
#define		DIVOOM_I2C_CLK_IO				(12)
#define		DIVOOM_I2C_DATA_IO				(11)


//uart

//没有用到TX定一个没有用到的IO
#define DIVOOM_UART_TX_IO					(-1)
#define DIVOOM_UART_RX_IO					(-1)


//reset io
#define		RESET_IO						(-1)
//reset io end

#define		DIVOOM_AMPLIFIER_POWER			(-1)

//Line In 检测 GPIO (需要根据实际硬件修改)
//设置为 -1 表示禁用 GPIO 检测
#define		DIVOOM_LINE_IN_DETECT_GPIO		(-1)
//Line In 检测有效电平 (1: 高电平表示插入, 0: 低电平表示插入)
#define		DIVOOM_LINE_IN_DETECT_ACTIVE_LEVEL	(0)

//UAC (USB Audio Class) 配置
//UAC 采样率 (支持 44100, 48000 等)
#define		DIVOOM_UAC_SAMPLE_RATE			(48000)
//UAC 通道数
#define		DIVOOM_UAC_CHANNELS				(2)
//UAC 设备检测间隔 (毫秒)
#define		DIVOOM_UAC_DETECT_INTERVAL_MS	(500)
//UAC 播放设备 (本地播放设备)
#define		DIVOOM_UAC_PLAYBACK_DEVICE		"hw:0,0"

#else
//这个文件作为所有的io口配置目录，避免多次自定义不好修改
//唤醒key，GPIO(DIVOOM_ONOFF_CHECK_KEY)
#define		DIVOOM_ONOFF_CHECK_KEY			(-1)
#define		DIVOOM_ONOFF_CHECK_MODE_KEY		(-1)
//#define		DIVOOM_ONOFF_POWERON_POWER		(13)
#define		DIVOOM_ONOFF_POWERON_POWER		(-1)
//#define		DIVOOM_ONOFF_POWEROFF_POWER		(12)

#define		DIVOOM_KEY_ADC_PIN				(38)

//蜂呜器
#define DIVOOM_BUZZER_PWM_PIN				(21)


//断码屏
#define DIVOOM_LEDSEG_CLK_PIN				(-1)
#define DIVOOM_LEDSEG_DAT_PIN				(-1)
#define DIVOOM_LEDSEG_STB_PIN				(-1)


//led
#define		DIVOOM_LCD_PWM_GPIO				11

#define DIVOOM_LCD1_CS_GPIO        (12)		//12,13,14
#define DIVOOM_LCD2_CS_GPIO        (13)		//12,13,14
#define DIVOOM_LCD3_CS_GPIO        (14)		//12,13,14
#define DIVOOM_LCD4_CS_GPIO        (15)		//12,13,14
#define DIVOOM_LCD5_CS_GPIO        (5)		//12,13,14
#define DIVOOM_LCD_CS_GPIO        (12)		//12,13,14


#define DIVOOM_LCD_BK_LIGHT_GPIO  (11)
#define DIVOOM_LCD_RST_GPIO       (4)
#define DIVOOM_LCD_DC_GPIO        (6)
#define DIVOOM_LCD_PCLK_GPIO      (7)

#define DIVOOM_LCD_RD_GPIO		(-1)	//(15)

#define DIVOOM_LCD_DATA0_GPIO     (16)
#define DIVOOM_LCD_DATA1_GPIO     (17)
#define DIVOOM_LCD_DATA2_GPIO     (18)
#define DIVOOM_LCD_DATA3_GPIO     (8)
#define DIVOOM_LCD_DATA4_GPIO     (3)
#define DIVOOM_LCD_DATA5_GPIO     (46)
#define DIVOOM_LCD_DATA6_GPIO     (9)
#define DIVOOM_LCD_DATA7_GPIO     (10)



//单线数据LED灯
#define DIVOOM_ONELINE_LED_SPI_PIN_NUM_MOSI	(0)

#define DIVOOM_USBDET_PIN					(-1)
#define DIVOOM_LED1_PIN						(-1)
#define DIVOOM_LED2_PIN						(-1)

//天花板和VJ都用串口传输了

//divoom_spi.c start
#define DIVOOM_SPI_PIN_NUM_CLK				(25)
#define DIVOOM_SPI_PIN_NUM_MOSI				(33)
#define DIVOOM_SPI_PIN_NUM_MISO				(-1)
#define DIVOOM_SPI_PIN_NUM_CS				(26)

//divoom_spi.c end

//i2s use
#define		DIVOOM_I2S_CHIP_EN				(-1)
#define		DIVOOM_I2S_BCK_IO				(48)
#define		DIVOOM_I2S_DATAIN_IO			(47)
#define		DIVOOM_I2S_DATA_OUT_IO			(-1)
#define		DIVOOM_I2S_WS_IO				(45)
//i2s use

//sd use start
#define		DIVOOM_SD_DETECT_IO				(-1)
#define		DIVOOM_SD_D0_IO					(40)
#define		DIVOOM_SD_D1_IO					(41)
#define		DIVOOM_SD_D2_IO					(42)
#define		DIVOOM_SD_D3_IO					(2)
#define		DIVOOM_SD_CLK_IO				(1)
#define		DIVOOM_SD_CMD_IO				(39)
//sd use end


//i2c
#define		DIVOOM_I2C_CLK_IO				(1)
#define		DIVOOM_I2C_DATA_IO				(39)




//uart

//没有用到TX定一个没有用到的IO
#define DIVOOM_UART_TX_IO					(-1)
#define DIVOOM_UART_RX_IO					(32)


//reset io
#define		RESET_IO						(-1)
//reset io end

#define		DIVOOM_AMPLIFIER_POWER			(-1)

//Line In 检测 GPIO (需要根据实际硬件修改)
//设置为 -1 表示禁用 GPIO 检测
#define		DIVOOM_LINE_IN_DETECT_GPIO		(-1)
//Line In 检测有效电平 (1: 高电平表示插入, 0: 低电平表示插入)
#define		DIVOOM_LINE_IN_DETECT_ACTIVE_LEVEL	(0)

//UAC (USB Audio Class) 配置
//UAC 采样率 (支持 44100, 48000 等)
#define		DIVOOM_UAC_SAMPLE_RATE			(48000)
//UAC 通道数
#define		DIVOOM_UAC_CHANNELS				(2)
//UAC 设备检测间隔 (毫秒)
#define		DIVOOM_UAC_DETECT_INTERVAL_MS	(500)
//UAC 播放设备 (本地播放设备)
#define		DIVOOM_UAC_PLAYBACK_DEVICE		"hw:0,0"

#endif

#endif





