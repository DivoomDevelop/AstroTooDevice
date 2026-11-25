/**
 * ACM8625 应用层：功放上电（GPIO）+ 经 MCU 协议请求初始化
 * 硬件：T113 --协议--> PY32F002BF1 MCU --I2C--> ACM8625；T113 直连 PDN/PA-POWER、I2S。
 * 协议：addr 7bit=0x15；SLAVE_IIC_WRITE_ADDR=0x2A, SLAVE_IIC_READ_ADDR=0x2B；
 *       reg0 未用，reg1 音量(0~16)，reg2 Mute(0=UnMute/1=Mute)，reg3 Mode(0=Music/1=Gaming)。
 * 编译：可被主程序链接，或定义 STANDALONE 做独立测试。
 */
#include <fcntl.h>
#include "divoom.h"

/**
 * @brief 从I2C设备读取指定寄存器的值
 * @param fd I2C设备文件描述符
 * @param reg 寄存器地址
 * @param data 存储读取结果的指针
 * @return 成功返回0，失败返回-1
 */
int i2c_read_reg(int fd, unsigned char reg, unsigned char *data) 
{
    // 先写入要读取的寄存器地址
    if (write(fd, &reg, 1) != 1) 
	{
        perror("Failed to write register address");
        return -1;
    }
    // 读取寄存器的值
    if (read(fd, data, 1) != 1) 
	{
        perror("Failed to read data");
        return -1;
    }
    return 0;
}

/**
 * @brief 向I2C设备指定寄存器写入值
 * @param fd I2C设备文件描述符
 * @param reg 寄存器地址
 * @param data 要写入的值
 * @return 成功返回0，失败返回-1
 */
int i2c_write_reg(int fd, unsigned char reg, unsigned char data) 
{
    unsigned char buf[2] = {reg, data};
    // 写入寄存器地址和对应的值
    if (write(fd, buf, 2) != 2) 
	{
        perror("Failed to write data");
        return -1;
    }
    return 0;
}

static int gpio_export(int gpio)
{
	char path[64], buf[8];
	int fd, len;

	if (gpio < 0)
		return 0;

	snprintf(path, sizeof(path), "/sys/class/gpio/gpio%d", gpio);
	if (divoom_port_file_exists(path))
		return 0;

	fd = open("/sys/class/gpio/export", O_WRONLY);
	if (fd < 0)
		return -errno;
	len = snprintf(buf, sizeof(buf), "%d", gpio);
	if (write(fd, buf, len) != len) {
		close(fd);
		return -errno;
	}
	close(fd);
	divoom_port_delay_ms((50000) / 1000U);
	return 0;
}

static int gpio_set_direction_out(int gpio)
{
	char path[64];
	int fd;

	if (gpio < 0)
		return 0;
	snprintf(path, sizeof(path), "/sys/class/gpio/gpio%d/direction", gpio);
	fd = open(path, O_WRONLY);
	if (fd < 0)
		return -errno;
	if (write(fd, "out", 3) != 3) {
		close(fd);
		return -errno;
	}
	close(fd);
	return 0;
}

static int gpio_set_value(int gpio, int value)
{
	char path[64];
	int fd;

	if (gpio < 0)
		return 0;
	snprintf(path, sizeof(path), "/sys/class/gpio/gpio%d/value", gpio);
	fd = open(path, O_WRONLY);
	if (fd < 0)
		return -errno;
	if (write(fd, value ? "1" : "0", 1) != 1) {
		close(fd);
		return -errno;
	}
	close(fd);
	return 0;
}

int divoom_power_key_led_on_off(int flag)
{
	int ret;

	ret = gpio_export(111); //PD15 (3*32 + 15)
	if (ret) return ret;
	ret = gpio_set_direction_out(111);
	if (ret) return ret;

	if (flag == 1)
		gpio_set_value(111, 1);
	else
		gpio_set_value(111, 0);
	return 0;
}

int divoom_tf_power_on_off(int flag)
{
	int ret;

	ret = gpio_export(164); //PF4 (5*32 + 4)
	if (ret) return ret;
	ret = gpio_set_direction_out(164);
	if (ret) return ret;

	if (flag == 1)
		gpio_set_value(164, 1);
	else
		gpio_set_value(164, 0);
	return 0;
}

int divoom_acm8625_power_on(void)
{
	int ret;

	ret = gpio_export(ACM8625_GPIO_PDN_PE8);
	if (ret) return ret;
	ret = gpio_set_direction_out(ACM8625_GPIO_PDN_PE8);
	if (ret) return ret;
	ret = gpio_export(ACM8625_GPIO_POWER_PE9);
	if (ret) return ret;
	ret = gpio_set_direction_out(ACM8625_GPIO_POWER_PE9);
	if (ret) return ret;

	gpio_set_value(ACM8625_GPIO_POWER_PE9, 1);
	divoom_port_delay_ms((10000) / 1000U);
	gpio_set_value(ACM8625_GPIO_PDN_PE8, 1);
	return 0;
}

int divoom_acm8625_power_off(void)
{
	gpio_set_value(ACM8625_GPIO_PDN_PE8, 0);
	gpio_set_value(ACM8625_GPIO_POWER_PE9, 0);
	return 0;
}

//0 music mode 1 game mode
int divoom_acm8625_set_mode(uint8 mode)
{
	int fd;
    unsigned char read_data;

	// 1. 打开I2C设备文件
	fd = open(I2C_DEV, O_RDWR);
	if (fd < 0) 
	{
		perror("Failed to open I2C device");
		return -1;
	}

	// 2. 设置I2C从设备地址
	if (ioctl(fd, I2C_SLAVE, I2C_SLAVE_ADDR) < 0) 
	{
		perror("Failed to set I2C slave address");
		close(fd);
		return -1;
	}

	// reg3写入 0 music mode, 1 gaming mode
	if (i2c_write_reg(fd, 0x03, mode) != 0) 
	{
		close(fd);
		return -1;
	}

	if (i2c_read_reg(fd, 0x03, &read_data) != 0) 
	{
		close(fd);
		return -1;
	}
	divoom_print("Read success: reg 0x03 = 0x%02X\n", read_data);

	// 5. 关闭设备文件
	close(fd);
	return 0;
}

//0 unmute 1 mute
int divoom_acm8625_set_mute(uint8 status)
{
	int fd;
    unsigned char read_data;

	// 1. 打开I2C设备文件
	fd = open(I2C_DEV, O_RDWR);
	if (fd < 0) 
	{
		perror("Failed to open I2C device");
		return -1;
	}

	// 2. 设置I2C从设备地址
	if (ioctl(fd, I2C_SLAVE, I2C_SLAVE_ADDR) < 0) 
	{
		perror("Failed to set I2C slave address");
		close(fd);
		return -1;
	}

	// reg2写入 0 unmut, 1 mute
	if (i2c_write_reg(fd, 0x02, status) != 0) 
	{
		close(fd);
		return -1;
	}

	if (i2c_read_reg(fd, 0x02, &read_data) != 0) 
	{
		close(fd);
		return -1;
	}
	divoom_print("Read success: reg 0x02 = 0x%02X\n", read_data);

	// 5. 关闭设备文件
	close(fd);
	return 0;
}

//vol 最大16
int divoom_acm8625_set_vol(uint8 vol)
{
	int fd;
    unsigned char read_data;

	// 1. 打开I2C设备文件
	fd = open(I2C_DEV, O_RDWR);
	if (fd < 0) 
	{
		perror("Failed to open I2C device");
		return -1;
	}

	// 2. 设置I2C从设备地址
	if (ioctl(fd, I2C_SLAVE, I2C_SLAVE_ADDR) < 0) 
	{
		perror("Failed to set I2C slave address");
		close(fd);
		return -1;
	}

	if (vol > 16)
		vol = 16;

	// reg1写入音量值
	if (i2c_write_reg(fd, 0x01, vol) != 0) 
	{
		close(fd);
		return -1;
	}

	if (i2c_read_reg(fd, 0x01, &read_data) != 0) 
	{
		close(fd);
		return -1;
	}
	divoom_print("Read success: reg 0x01 = 0x%02X\n", read_data);

	// 5. 关闭设备文件
	close(fd);
	return 0;
}

int divoom_acm8625_init_all_reg(void)
{
	int fd;
    unsigned char read_data;

	// 1. 打开I2C设备文件
	fd = open(I2C_DEV, O_RDWR);
	if (fd < 0) 
	{
		perror("Failed to open I2C device");
		return -1;
	}

	// 2. 设置I2C从设备地址
	if (ioctl(fd, I2C_SLAVE, I2C_SLAVE_ADDR) < 0) 
	{
		perror("Failed to set I2C slave address");
		close(fd);
		return -1;
	}

	// reg1写入音量值
	if (i2c_write_reg(fd, 0x01, 8) != 0) 
	{
		close(fd);
		return -1;
	}

	if (i2c_read_reg(fd, 0x01, &read_data) != 0) 
	{
		close(fd);
		return -1;
	}
	divoom_print("Read success: reg 0x01 = 0x%02X\n", read_data);

	// reg2写入 0 unmut, 1 mute
	if (i2c_write_reg(fd, 0x02, 0) != 0) 
	{
		close(fd);
		return -1;
	}

	if (i2c_read_reg(fd, 0x02, &read_data) != 0) 
	{
		close(fd);
		return -1;
	}
	divoom_print("Read success: reg 0x02 = 0x%02X\n", read_data);

	// reg3写入 0 music mode, 1 gaming mode
	if (i2c_write_reg(fd, 0x03, 0) != 0) 
	{
		close(fd);
		return -1;
	}

	if (i2c_read_reg(fd, 0x03, &read_data) != 0) 
	{
		close(fd);
		return -1;
	}
	divoom_print("Read success: reg 0x03 = 0x%02X\n", read_data);

	// 5. 关闭设备文件
	close(fd);
	return 0;
}


