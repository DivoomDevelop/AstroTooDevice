/**
 * ACM8625 应用层：功放上电（GPIO）+ 经 MCU 协议请求初始化
 *
 * 硬件拓扑：T113 --协议(I2C/UART)--> PY32F002BF1 MCU --I2C--> ACM8625；
 * T113 直连：PDN/PA-POWER(PE8/PE9)、I2S → ACM8625。
 * 功放上电由本模块拉 GPIO；ACM8625 的 I2C 初始化由 MCU 完成，T113 通过协议命令 MCU 执行。
 *
 * 音频路径（连接外部功放仅用 I2S）：
 *   LINEIN(L/R) → SOC 内部 codec 处理 → I2S 数字输出 → ACM8625(DAC+功放) → 喇叭。
 * 即 linein 接收的数据经 I2S 送到外部功放放出来。HPOUTR/HPOUTL 是 SOC 内部 codec 的
 * 模拟输出引脚，本板若未接到 ACM8625则可不接或另作耳机口；与功放相连的是 I2S 线。
 * 本模块的 vol/mute/mode 控制的是 ACM8625 这条通路的输出。
 */
#ifndef ACM8625_APP_INIT_H
#define ACM8625_APP_INIT_H

#ifdef __cplusplus
extern "C" {
#endif

/* 芯片 7bit 地址 0x15，I2C 读写 8bit 地址 */
#define SLAVE_IIC_WRITE_ADDR    0x2A
#define SLAVE_IIC_READ_ADDR     0x2B

/* T113 接 MCU：TWI2 多为 /dev/i2c-2 */
#define I2C_DEV    "/dev/i2c-2"
#define I2C_SLAVE_ADDR 0x15 //要7bit地址

/* 寄存器协议：reg0 未用；reg1 音量(0~16)；reg2 Mute(0=UnMute,1=Mute)；reg3 Mode(0=Music,1=Gaming) */
#define ACM8625_REG0_UNUSED     0
#define ACM8625_VOL_MIN          0
#define ACM8625_VOL_MAX          16
#define ACM8625_MUTE_OFF         0
#define ACM8625_MUTE_ON          1
#define ACM8625_MODE_MUSIC       0
#define ACM8625_MODE_GAMING      1

/* GPIO 默认：T113 PE8=136, PE9=137（公式: bank*32+pin, bank E=4） */
#define ACM8625_GPIO_PDN_PE8     136
#define ACM8625_GPIO_POWER_PE9   137

int divoom_tf_power_on_off(int flag);

int divoom_power_key_led_on_off(int flag);

/**
 * 功放上电（拉高 PE9、PE8）。
 * 使用 sysfs /sys/class/gpio。gpio_pdn / gpio_power 为 -1 表示跳过该引脚。
 * 返回 0 成功，负值为错误码。
 */
int divoom_acm8625_power_on(void);

/**
 * 功放断电（拉低 PE8、PE9）。
 */
int divoom_acm8625_power_off(void);

//0 music mode 1 game mode
int divoom_acm8625_set_mode(uint8 mode);

//0 unmute 1 mute
int divoom_acm8625_set_mute(uint8 status);

//vol 最大16
int divoom_acm8625_set_vol(uint8 vol);


int divoom_acm8625_init(void);

#ifdef __cplusplus
}
#endif

#endif /* ACM8625_APP_INIT_H */
