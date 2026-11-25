#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <linux/i2c-dev.h>

#include "divoom_acm8625p_amp.h"

#ifndef DIVOOM_ACM8625P_EQ_PRESET_T_DEFINED
typedef enum {
    ACM8625P_EQ_PRESET_FLAT = 0,
    ACM8625P_EQ_PRESET_BYPASS,
} divoom_acm8625p_eq_preset_t;
#endif

static uint8 g_acm8625p_i2c_addr_7bit = ACM8625P_AMP_I2C_ADDR_7BIT_DEFAULT;
static const int g_acm8625p_clk_fault_retry_count = 20;
static const int g_acm8625p_clk_fault_retry_delay_ms = 50;
static divoom_acm8625p_eq_preset_t g_acm8625p_eq_preset = ACM8625P_EQ_PRESET_FLAT;
static uint8 g_acm8625p_auto_eq_by_volume = 0;

#define ACM8625P_EQ_AUTO_SWITCH_VOL_REG 0xE8

static int acm8625p_gpio_export(int gpio)
{
    char path[64];
    char buf[8];
    int fd;
    int len;

    if (gpio < 0) {
        return 0;
    }

    snprintf(path, sizeof(path), "/sys/class/gpio/gpio%d", gpio);
    if (divoom_port_file_exists(path)) {
        return 0;
    }

    fd = open("/sys/class/gpio/export", O_WRONLY);
    if (fd < 0) {
        return -errno;
    }

    len = snprintf(buf, sizeof(buf), "%d", gpio);
    if (write(fd, buf, len) != len) {
        close(fd);
        return -errno;
    }

    close(fd);
    divoom_port_delay_ms((50000) / 1000U);
    return 0;
}

static int acm8625p_gpio_set_direction_out(int gpio)
{
    char path[64];
    int fd;

    if (gpio < 0) {
        return 0;
    }

    snprintf(path, sizeof(path), "/sys/class/gpio/gpio%d/direction", gpio);
    fd = open(path, O_WRONLY);
    if (fd < 0) {
        return -errno;
    }

    if (write(fd, "out", 3) != 3) {
        close(fd);
        return -errno;
    }

    close(fd);
    return 0;
}

static int acm8625p_gpio_set_value(int gpio, int value)
{
    char path[64];
    int fd;

    if (gpio < 0) {
        return 0;
    }

    snprintf(path, sizeof(path), "/sys/class/gpio/gpio%d/value", gpio);
    fd = open(path, O_WRONLY);
    if (fd < 0) {
        return -errno;
    }

    if (write(fd, value ? "1" : "0", 1) != 1) {
        close(fd);
        return -errno;
    }

    close(fd);
    return 0;
}

static int acm8625p_i2c_open(void)
{
    int fd;

    fd = open(ACM8625P_AMP_I2C_DEV, O_RDWR);
    if (fd < 0) {
        perror("open ACM8625P i2c dev failed");
        return -1;
    }

    if (ioctl(fd, I2C_SLAVE, g_acm8625p_i2c_addr_7bit) < 0) {
        perror("set ACM8625P i2c addr failed");
        close(fd);
        return -1;
    }

    return fd;
}

static int acm8625p_i2c_write_reg_by_fd(int fd, uint8 reg, uint8 val)
{
    uint8 buf[2];

    buf[0] = reg;
    buf[1] = val;
    if (write(fd, buf, 2) != 2) {
        perror("ACM8625P write reg failed");
        return -1;
    }

    return 0;
}

static int acm8625p_i2c_read_reg_by_fd(int fd, uint8 reg, uint8 *val)
{
    if (val == NULL) {
        return -1;
    }

    if (write(fd, &reg, 1) != 1) {
        perror("ACM8625P set reg addr failed");
        return -1;
    }

    if (read(fd, val, 1) != 1) {
        perror("ACM8625P read reg failed");
        return -1;
    }

    return 0;
}

static void acm8625p_fault_append(char *buf, uint32 buf_len, uint32 *pos, const char *tag)
{
    int n;

    if ((buf == NULL) || (pos == NULL) || (tag == NULL) || (buf_len == 0)) {
        return;
    }
    if (*pos >= (buf_len - 1)) {
        return;
    }

    if (*pos > 0) {
        n = snprintf(buf + *pos, buf_len - *pos, "|%s", tag);
    } else {
        n = snprintf(buf + *pos, buf_len - *pos, "%s", tag);
    }

    if (n > 0) {
        *pos += (uint32)n;
        if (*pos >= buf_len) {
            *pos = buf_len - 1;
        }
    }
}

int divoom_acm8625p_amp_power_on(void)
{
    int ret;

    ret = acm8625p_gpio_export(ACM8625P_AMP_GPIO_PDN_PE8);
    if (ret) {
        return ret;
    }
    ret = acm8625p_gpio_set_direction_out(ACM8625P_AMP_GPIO_PDN_PE8);
    if (ret) {
        return ret;
    }
    ret = acm8625p_gpio_export(ACM8625P_AMP_GPIO_POWER_PE9);
    if (ret) {
        return ret;
    }
    ret = acm8625p_gpio_set_direction_out(ACM8625P_AMP_GPIO_POWER_PE9);
    if (ret) {
        return ret;
    }

    ret = acm8625p_gpio_set_value(ACM8625P_AMP_GPIO_POWER_PE9, 1);
    if (ret) {
        return ret;
    }
    divoom_port_delay_ms((10000) / 1000U);
    ret = acm8625p_gpio_set_value(ACM8625P_AMP_GPIO_PDN_PE8, 1);
    if (ret) {
        return ret;
    }

    /* 规格书 10.9：PDN 拉高后建议等待 >=1ms 再做寄存器初始化 */
    divoom_port_delay_ms((1000) / 1000U);
    return 0;
}

int divoom_acm8625p_amp_power_off(void)
{
    acm8625p_gpio_set_value(ACM8625P_AMP_GPIO_PDN_PE8, 0);
    acm8625p_gpio_set_value(ACM8625P_AMP_GPIO_POWER_PE9, 0);
    return 0;
}

int divoom_acm8625p_amp_write_reg(uint8 reg, uint8 val)
{
    int fd;
    int ret;

    fd = acm8625p_i2c_open();
    if (fd < 0) {
        return -1;
    }

    ret = acm8625p_i2c_write_reg_by_fd(fd, reg, val);
    close(fd);
    return ret;
}

int divoom_acm8625p_amp_read_reg(uint8 reg, uint8 *val)
{
    int fd;
    int ret;

    fd = acm8625p_i2c_open();
    if (fd < 0) {
        return -1;
    }

    ret = acm8625p_i2c_read_reg_by_fd(fd, reg, val);
    close(fd);
    return ret;
}

int divoom_acm8625p_amp_set_i2c_addr(uint8 addr_7bit)
{
    if ((addr_7bit < 0x03) || (addr_7bit > 0x77)) {
        return -1;
    }
    g_acm8625p_i2c_addr_7bit = addr_7bit;
    return 0;
}

int divoom_acm8625p_amp_set_i2c_addr_by_adr(divoom_acm8625p_adr_cfg_t adr_cfg)
{
    switch (adr_cfg) {
    case ACM8625P_ADR_PULL_4K7:
        return divoom_acm8625p_amp_set_i2c_addr(ACM8625P_AMP_I2C_ADDR_7BIT_4K7);
    case ACM8625P_ADR_PULL_15K:
        return divoom_acm8625p_amp_set_i2c_addr(ACM8625P_AMP_I2C_ADDR_7BIT_15K);
    case ACM8625P_ADR_PULL_47K:
        return divoom_acm8625p_amp_set_i2c_addr(ACM8625P_AMP_I2C_ADDR_7BIT_47K);
    case ACM8625P_ADR_PULL_120K:
        return divoom_acm8625p_amp_set_i2c_addr(ACM8625P_AMP_I2C_ADDR_7BIT_120K);
    default:
        return -1;
    }
}

int divoom_acm8625p_amp_apply_cfg(const divoom_acm8625p_reg_cfg_t *cfg, uint32 cfg_num)
{
    uint32 i;
    int fd;
    int ret;

    if ((cfg == NULL) || (cfg_num == 0)) {
        return -1;
    }

    fd = acm8625p_i2c_open();
    if (fd < 0) {
        return -1;
    }

    for (i = 0; i < cfg_num; i++) {
        ret = acm8625p_i2c_write_reg_by_fd(fd, cfg[i].reg, cfg[i].val);
        if (ret) {
            close(fd);
            return ret;
        }

        if (cfg[i].delay_ms > 0) {
            divoom_port_delay_ms((cfg[i].delay_ms * 1000) / 1000U);
        }
    }

    close(fd);
    return 0;
}

int divoom_acm8625p_amp_apply_default_cfg(void)
{
    /*
     * 仅 Page0 保守配置（与 64fs I2S 已验证可用）。
     * 参考驱动整表 + 程序化 EQ 需与 DSP 脚本完全一致，否则易无声；需要时再单独接入。
     */
    static const divoom_acm8625p_reg_cfg_t default_cfg[] = {
        {ACM8625P_REG_AMP_CTRL1, 0x04, 0},
        {ACM8625P_REG_AMP_CTRL2, 0x00, 0},
        {ACM8625P_REG_AMP_CTRL3, 0x05, 0},
        {ACM8625P_REG_PROCESSING_CTRL1, 0x12, 0},
        {ACM8625P_REG_PROCESSING_CTRL2, 0xF0, 0},
        {ACM8625P_REG_I2S_DATA_FORMAT1, 0x02, 0},
        {ACM8625P_REG_I2S_DATA_FORMAT2, 0x00, 0},
        {ACM8625P_REG_SS_CTRL, 0x00, 0},
        {ACM8625P_REG_VOLUME_CTRL_L, 0xC0, 0},
        {ACM8625P_REG_VOLUME_CTRL_R, 0xC0, 0},
        {ACM8625P_REG_MISC_CTRL, 0x03, 0},
        {ACM8625P_REG_STATE_CTRL, ACM8625P_STATE_DRIVER_OFF, 0},
    };

    g_acm8625p_eq_preset = ACM8625P_EQ_PRESET_FLAT;
    return divoom_acm8625p_amp_apply_cfg(default_cfg, sizeof(default_cfg) / sizeof(default_cfg[0]));
}

int divoom_acm8625p_amp_set_volume(uint8 vol_reg)
{
    int ret;
    divoom_acm8625p_eq_preset_t target_eq;

    if (vol_reg < ACM8625P_VOL_REG_MIN) {
        vol_reg = ACM8625P_VOL_REG_MIN;
    }
    if (vol_reg > ACM8625P_VOL_REG_MAX) {
        vol_reg = ACM8625P_VOL_REG_MAX;
    }

    ret = divoom_acm8625p_amp_write_reg(ACM8625P_REG_VOLUME_CTRL_L, vol_reg);
    if (ret) {
        return ret;
    }
    ret = divoom_acm8625p_amp_write_reg(ACM8625P_REG_VOLUME_CTRL_R, vol_reg);
    if (ret) {
        return ret;
    }

    if (g_acm8625p_auto_eq_by_volume) {
        target_eq = (vol_reg >= ACM8625P_EQ_AUTO_SWITCH_VOL_REG) ?
            ACM8625P_EQ_PRESET_BYPASS : ACM8625P_EQ_PRESET_FLAT;
        if (target_eq != g_acm8625p_eq_preset) {
            return divoom_acm8625p_amp_set_eq_preset(target_eq);
        }
    }

    return 0;
}

int divoom_acm8625p_amp_set_volume_db(int db_x2)
{
    int reg_val;

    /* db_x2 = dB * 2，范围 [-208, +48]，每步 0.5dB */
    if (db_x2 < -208) {
        db_x2 = -208;
    }
    if (db_x2 > 48) {
        db_x2 = 48;
    }

    reg_val = ACM8625P_VOL_REG_0DB + db_x2;
    return divoom_acm8625p_amp_set_volume((uint8)reg_val);
}

int divoom_acm8625p_amp_set_eq_preset(divoom_acm8625p_eq_preset_t preset)
{
    int ret;

    if ((preset != ACM8625P_EQ_PRESET_FLAT) && (preset != ACM8625P_EQ_PRESET_BYPASS)) {
        return -1;
    }

    /*
     * 当前默认路径未加载完整 DSP/EQ 表，仅做与规格书默认一致的 Page0 处理控制。
     * 参考驱动里 PostEQ 用 0x05=0x20/0x28，与纯 Page0 的 0x12 不兼容，勿混用。
     */
    ret = divoom_acm8625p_amp_write_reg(ACM8625P_REG_PROCESSING_CTRL1, 0x12);
    if (ret) {
        return ret;
    }
    ret = divoom_acm8625p_amp_write_reg(ACM8625P_REG_PROCESSING_CTRL2, 0xF0);
    if (ret) {
        return ret;
    }

    if (preset == ACM8625P_EQ_PRESET_BYPASS) {
        /* 占位：完整参考表接入后再写 0x20/0x28 或专用 bypass */
        (void)0;
    }

    g_acm8625p_eq_preset = preset;
    return 0;
}

int divoom_acm8625p_amp_set_auto_eq_by_volume(uint8 enable)
{
    g_acm8625p_auto_eq_by_volume = (enable != 0) ? 1 : 0;
    return 0;
}

int divoom_acm8625p_amp_set_mute(uint8 mute)
{
    uint8 reg = 0;
    int ret;

    ret = divoom_acm8625p_amp_read_reg(ACM8625P_REG_STATE_CTRL, &reg);
    if (ret) {
        return ret;
    }

    if (mute == ACM8625P_MUTE_ON) {
        reg |= (1U << 3);   /* MUTE_L */
        reg |= (1U << 2);   /* MUTE_R */
    } else {
        reg &= (uint8)(~(1U << 3));
        reg &= (uint8)(~(1U << 2));
    }

    return divoom_acm8625p_amp_write_reg(ACM8625P_REG_STATE_CTRL, reg);
}

int divoom_acm8625p_amp_set_state(uint8 state)
{
    uint8 reg = 0;
    int ret;

    if (state > ACM8625P_STATE_PLAY) {
        return -1;
    }

    ret = divoom_acm8625p_amp_read_reg(ACM8625P_REG_STATE_CTRL, &reg);
    if (ret) {
        return ret;
    }

    reg &= (uint8)(~0x03U);
    reg |= (state & 0x03U);
    return divoom_acm8625p_amp_write_reg(ACM8625P_REG_STATE_CTRL, reg);
}

int divoom_acm8625p_amp_fault_clear(void)
{
    uint8 reg = 0;
    int ret;

    ret = divoom_acm8625p_amp_read_reg(ACM8625P_REG_AMP_CTRL1, &reg);
    if (ret) {
        return ret;
    }

    reg |= (1U << 7);
    return divoom_acm8625p_amp_write_reg(ACM8625P_REG_AMP_CTRL1, reg);
}

int divoom_acm8625p_amp_get_fault(uint8 *fault1, uint8 *fault2, uint8 *fault3)
{
    int ret;

    if ((fault1 == NULL) || (fault2 == NULL) || (fault3 == NULL)) {
        return -1;
    }

    ret = divoom_acm8625p_amp_read_reg(ACM8625P_REG_FAULT_RPT1, fault1);
    if (ret) {
        return ret;
    }
    ret = divoom_acm8625p_amp_read_reg(ACM8625P_REG_FAULT_RPT2, fault2);
    if (ret) {
        return ret;
    }
    return divoom_acm8625p_amp_read_reg(ACM8625P_REG_FAULT_RPT3, fault3);
}

int divoom_acm8625p_amp_fault_to_string(uint8 fault1, uint8 fault2, uint8 fault3, char *buf, uint32 buf_len)
{
    uint32 pos = 0;

    if ((buf == NULL) || (buf_len == 0)) {
        return -1;
    }
    buf[0] = '\0';

    /* FAULT_RPT1: OTSD, PVDD_OV, PVDD_UV, CH2_DC, CH1_DC, CH2_OC, CH1_OC */
    if (fault1 & (1U << 6)) { acm8625p_fault_append(buf, buf_len, &pos, "OTSD"); }
    if (fault1 & (1U << 5)) { acm8625p_fault_append(buf, buf_len, &pos, "PVDD_OV"); }
    if (fault1 & (1U << 4)) { acm8625p_fault_append(buf, buf_len, &pos, "PVDD_UV"); }
    if (fault1 & (1U << 3)) { acm8625p_fault_append(buf, buf_len, &pos, "CH2_DC"); }
    if (fault1 & (1U << 2)) { acm8625p_fault_append(buf, buf_len, &pos, "CH1_DC"); }
    if (fault1 & (1U << 1)) { acm8625p_fault_append(buf, buf_len, &pos, "CH2_OC"); }
    if (fault1 & (1U << 0)) { acm8625p_fault_append(buf, buf_len, &pos, "CH1_OC"); }

    /* FAULT_RPT2: CLK_FAULT */
    if (fault2 & (1U << 2)) { acm8625p_fault_append(buf, buf_len, &pos, "CLK_FAULT"); }

    /* FAULT_RPT3: CH2_CLIP, CH1_CLIP, OTW */
    if (fault3 & (1U << 2)) { acm8625p_fault_append(buf, buf_len, &pos, "CH2_CLIP"); }
    if (fault3 & (1U << 1)) { acm8625p_fault_append(buf, buf_len, &pos, "CH1_CLIP"); }
    if (fault3 & (1U << 0)) { acm8625p_fault_append(buf, buf_len, &pos, "OTW"); }

    if (pos == 0) {
        (void)snprintf(buf, buf_len, "NONE");
    }

    return 0;
}

divoom_acm8625p_fault_level_t divoom_acm8625p_amp_get_fault_level(uint8 fault1, uint8 fault2, uint8 fault3)
{
    /* fatal: 保护类故障，建议立即停播/静音并做恢复流程 */
    if ((fault1 & (1U << 6)) ||  /* OTSD */
        (fault1 & (1U << 5)) ||  /* PVDD_OV */
        (fault1 & (1U << 4)) ||  /* PVDD_UV */
        (fault1 & (1U << 3)) ||  /* CH2_DC */
        (fault1 & (1U << 2)) ||  /* CH1_DC */
        (fault1 & (1U << 1)) ||  /* CH2_OC */
        (fault1 & (1U << 0))) {  /* CH1_OC */
        return ACM8625P_FAULT_LEVEL_FATAL;
    }

    /* warn: 音质/时钟风险，建议降级处理并持续观察 */
    if ((fault2 & (1U << 2)) ||  /* CLK_FAULT */
        (fault3 & (1U << 0)) ||  /* OTW */
        (fault3 & (1U << 2)) ||  /* CH2_CLIP */
        (fault3 & (1U << 1))) {  /* CH1_CLIP */
        return ACM8625P_FAULT_LEVEL_WARN;
    }

    return ACM8625P_FAULT_LEVEL_INFO;
}

const char *divoom_acm8625p_amp_fault_level_to_string(divoom_acm8625p_fault_level_t level)
{
    switch (level) {
    case ACM8625P_FAULT_LEVEL_FATAL:
        return "FATAL";
    case ACM8625P_FAULT_LEVEL_WARN:
        return "WARN";
    case ACM8625P_FAULT_LEVEL_INFO:
    default:
        return "INFO";
    }
}

int divoom_acm8625p_amp_self_check(uint8 *die_id, uint8 *state, uint8 *fault1, uint8 *fault2, uint8 *fault3)
{
    uint8 die_id_local = 0;
    uint8 state_local = 0;
    uint8 fault1_local = 0;
    uint8 fault2_local = 0;
    uint8 fault3_local = 0;
    divoom_acm8625p_fault_level_t level;
    char fault_text[160];
    int ret;

    ret = divoom_acm8625p_amp_read_reg(ACM8625P_REG_DIEID_RPT, &die_id_local);
    if (ret) {
        return ret;
    }
    ret = divoom_acm8625p_amp_read_reg(ACM8625P_REG_STATE_RPT, &state_local);
    if (ret) {
        return ret;
    }
    ret = divoom_acm8625p_amp_get_fault(&fault1_local, &fault2_local, &fault3_local);
    if (ret) {
        return ret;
    }

    if (die_id != NULL) {
        *die_id = die_id_local;
    }
    if (state != NULL) {
        *state = state_local;
    }
    if (fault1 != NULL) {
        *fault1 = fault1_local;
    }
    if (fault2 != NULL) {
        *fault2 = fault2_local;
    }
    if (fault3 != NULL) {
        *fault3 = fault3_local;
    }

    divoom_acm8625p_amp_fault_to_string(fault1_local, fault2_local, fault3_local, fault_text, sizeof(fault_text));
    level = divoom_acm8625p_amp_get_fault_level(fault1_local, fault2_local, fault3_local);
    printf("acm8625p self-check: i2c=0x%02X die=0x%02X state=0x%02X fault=[0x%02X 0x%02X 0x%02X] decode=%s level=%s\n",
           g_acm8625p_i2c_addr_7bit,
           die_id_local,
           state_local,
           fault1_local,
           fault2_local,
           fault3_local,
           fault_text,
           divoom_acm8625p_amp_fault_level_to_string(level));
    return 0;
}

int divoom_acm8625p_amp_dump_key_regs(void)
{
    uint8 r01 = 0;
    uint8 r02 = 0;
    uint8 r03 = 0;
    uint8 r04 = 0;
    uint8 r07 = 0;
    uint8 r0f = 0;
    uint8 r10 = 0;
    uint8 r16 = 0;
    uint8 r17 = 0;
    uint8 r18 = 0;
    uint8 r19 = 0;
    char fault_text[160];
    int ret;

    ret = divoom_acm8625p_amp_read_reg(ACM8625P_REG_AMP_CTRL1, &r01);
    if (ret) return ret;
    ret = divoom_acm8625p_amp_read_reg(ACM8625P_REG_AMP_CTRL2, &r02);
    if (ret) return ret;
    ret = divoom_acm8625p_amp_read_reg(ACM8625P_REG_AMP_CTRL3, &r03);
    if (ret) return ret;
    ret = divoom_acm8625p_amp_read_reg(ACM8625P_REG_STATE_CTRL, &r04);
    if (ret) return ret;
    ret = divoom_acm8625p_amp_read_reg(ACM8625P_REG_I2S_DATA_FORMAT1, &r07);
    if (ret) return ret;
    ret = divoom_acm8625p_amp_read_reg(ACM8625P_REG_VOLUME_CTRL_L, &r0f);
    if (ret) return ret;
    ret = divoom_acm8625p_amp_read_reg(ACM8625P_REG_VOLUME_CTRL_R, &r10);
    if (ret) return ret;
    ret = divoom_acm8625p_amp_read_reg(ACM8625P_REG_STATE_RPT, &r16);
    if (ret) return ret;
    ret = divoom_acm8625p_amp_read_reg(ACM8625P_REG_FAULT_RPT1, &r17);
    if (ret) return ret;
    ret = divoom_acm8625p_amp_read_reg(ACM8625P_REG_FAULT_RPT2, &r18);
    if (ret) return ret;
    ret = divoom_acm8625p_amp_read_reg(ACM8625P_REG_FAULT_RPT3, &r19);
    if (ret) return ret;

    divoom_acm8625p_amp_fault_to_string(r17, r18, r19, fault_text, sizeof(fault_text));

    printf("acm8625p reg dump: "
           "r01=0x%02X r02=0x%02X r03=0x%02X r04=0x%02X r07=0x%02X r0f=0x%02X r10=0x%02X "
           "r16=0x%02X r17=0x%02X r18=0x%02X r19=0x%02X fault=%s\n",
           r01, r02, r03, r04, r07, r0f, r10, r16, r17, r18, r19, fault_text);

    printf("acm8625p check: PBTL=%u CH_L_HIZ=%u CH_R_HIZ=%u MUTE_L=%u MUTE_R=%u STATE=%u "
           "I2S_FMT=%u WORD_LEN=%u VOL_L=0x%02X VOL_R=0x%02X\n",
           (unsigned)(r01 & 0x01),
           (unsigned)((r04 >> 5) & 0x01),
           (unsigned)((r04 >> 4) & 0x01),
           (unsigned)((r04 >> 3) & 0x01),
           (unsigned)((r04 >> 2) & 0x01),
           (unsigned)(r04 & 0x03),
           (unsigned)((r07 >> 4) & 0x03),
           (unsigned)(r07 & 0x03),
           r0f, r10);

    return 0;
}

int divoom_acm8625p_amp_monitor_once_loop(uint32 duration_ms, uint32 interval_ms)
{
    uint32 i;
    uint32 loops;
    uint32 clk_fault_cnt = 0;
    uint32 fs_det_zero_cnt = 0;
    uint32 play_state_cnt = 0;
    uint8 fault1 = 0;
    uint8 fault2 = 0;
    uint8 fault3 = 0;
    uint8 state_rpt = 0;
    uint8 clk_fmt1 = 0;
    uint8 clk_fmt2 = 0;
    uint16 bclk_ratio = 0;
    uint8 fs_det = 0;
    int ret;

    if (interval_ms == 0) {
        interval_ms = 200;
    }
    if (duration_ms == 0) {
        duration_ms = 5000;
    }
    loops = duration_ms / interval_ms;
    if (loops == 0) {
        loops = 1;
    }
	loops = 1;

    printf("acm8625p monitor start: duration=%u ms interval=%u ms loops=%u\n",
           duration_ms, interval_ms, loops);

    /* 清一次历史锁存故障，后续监测更聚焦“当前窗口内”事件 */
    ret = divoom_acm8625p_amp_fault_clear();
    if (ret) {
        return ret;
    }

    for (i = 0; i < loops; i++) {
        ret = divoom_acm8625p_amp_get_fault(&fault1, &fault2, &fault3);
        if (ret) {
            return ret;
        }
        ret = divoom_acm8625p_amp_read_reg(ACM8625P_REG_STATE_RPT, &state_rpt);
        if (ret) {
            return ret;
        }
        ret = divoom_acm8625p_amp_read_reg(0x12, &clk_fmt1); /* I2S_CLK_FORMAT_RPT1 */
        if (ret) {
            return ret;
        }
        ret = divoom_acm8625p_amp_read_reg(0x13, &clk_fmt2); /* I2S_CLK_FORMAT_RPT2 */
        if (ret) {
            return ret;
        }

        fs_det = (uint8)(clk_fmt1 & 0x0F);
        bclk_ratio = (uint16)((((clk_fmt1 >> 4) & 0x03) << 8) | clk_fmt2);

        if (fault2 & (1U << 2)) {
            clk_fault_cnt++;
        }
        if (fs_det == 0) {
            fs_det_zero_cnt++;
        }
        if ((state_rpt & 0x03) == ACM8625P_STATE_PLAY) {
            play_state_cnt++;
        }

        printf("acm8625p monitor %u/%u: state_rpt=0x%02X fault=[0x%02X 0x%02X 0x%02X] fs_det=0x%X bclk_ratio=%u\n",
               i + 1,
               loops,
               state_rpt,
               fault1,
               fault2,
               fault3,
               fs_det,
               bclk_ratio);

        /* 每轮读完即清，下一轮只统计新出现故障 */
        ret = divoom_acm8625p_amp_fault_clear();
        if (ret) {
            return ret;
        }

        if (i + 1 < loops) {
            divoom_port_delay_ms(interval_ms);
        }
    }

    printf("acm8625p monitor summary: clk_fault=%u/%u fs_det_zero=%u/%u play_state=%u/%u\n",
           clk_fault_cnt, loops, fs_det_zero_cnt, loops, play_state_cnt, loops);

    return 0;
}

static int acm8625p_recover_from_clock_fault(void)
{
    int i;
    int ret;
    uint8 fault1 = 0;
    uint8 fault2 = 0;
    uint8 fault3 = 0;
    uint8 state_rpt = 0;
    uint8 clk_fmt1 = 0;
    uint8 clk_fmt2 = 0;
    uint16 bclk_ratio = 0;
    uint8 fs_det = 0;
    char fault_text[160];

    for (i = 0; i < g_acm8625p_clk_fault_retry_count; i++) {
        divoom_port_delay_ms(g_acm8625p_clk_fault_retry_delay_ms);

        ret = divoom_acm8625p_amp_get_fault(&fault1, &fault2, &fault3);
        if (ret) {
            return ret;
        }
        ret = divoom_acm8625p_amp_read_reg(ACM8625P_REG_STATE_RPT, &state_rpt);
        if (ret) {
            return ret;
        }
        ret = divoom_acm8625p_amp_read_reg(0x12, &clk_fmt1); /* I2S_CLK_FORMAT_RPT1 */
        if (ret) {
            return ret;
        }
        ret = divoom_acm8625p_amp_read_reg(0x13, &clk_fmt2); /* I2S_CLK_FORMAT_RPT2 */
        if (ret) {
            return ret;
        }

        fs_det = (uint8)(clk_fmt1 & 0x0F);
        bclk_ratio = (uint16)((((clk_fmt1 >> 4) & 0x03) << 8) | clk_fmt2);

        if ((fault2 & (1U << 2)) == 0) {
            printf("acm8625p clk-fault recovered at try %d/%d, state_rpt=0x%02X fs_det=0x%X bclk_ratio=%u\n",
                   i + 1,
                   g_acm8625p_clk_fault_retry_count,
                   state_rpt,
                   fs_det,
                   bclk_ratio);
            return 0;
        }

        ret = divoom_acm8625p_amp_fault_clear();
        if (ret) {
            return ret;
        }
        ret = divoom_acm8625p_amp_set_state(ACM8625P_STATE_PLAY);
        if (ret) {
            return ret;
        }

        divoom_acm8625p_amp_fault_to_string(fault1, fault2, fault3, fault_text, sizeof(fault_text));
        printf("acm8625p clk-fault recover try %d/%d, fault=[0x%02X 0x%02X 0x%02X] decode=%s "
               "state_rpt=0x%02X fs_det=0x%X bclk_ratio=%u\n",
               i + 1,
               g_acm8625p_clk_fault_retry_count,
               fault1,
               fault2,
               fault3,
               fault_text,
               state_rpt,
               fs_det,
               bclk_ratio);
    }

    return -1;
}

int divoom_acm8625p_amp_start_play(void)
{
    int ret;
    uint8 die_id = 0;
    uint8 state = 0;
    uint8 fault1 = 0;
    uint8 fault2 = 0;
    uint8 fault3 = 0;

    ret = divoom_acm8625p_amp_fault_clear();
    if (ret) {
        return ret;
    }

    ret = divoom_acm8625p_amp_set_state(ACM8625P_STATE_PLAY);
    if (ret) {
        return ret;
    }

    ret = divoom_acm8625p_amp_self_check(&die_id, &state, &fault1, &fault2, &fault3);
    if (ret) {
        return ret;
    }

    if (fault2 & (1U << 2)) {
        ret = acm8625p_recover_from_clock_fault();
        if (ret) {
            printf("acm8625p start_play clk-fault recovery failed\n");
            return ret;
        }

        ret = divoom_acm8625p_amp_self_check(&die_id, &state, &fault1, &fault2, &fault3);
        if (ret) {
            return ret;
        }
    }

    return divoom_acm8625p_amp_dump_key_regs();
}

int divoom_acm8625p_amp_init(void)
{
    int ret;

    /* 板级 ADR=15k → 7bit 地址 0x2D */
    ret = divoom_acm8625p_amp_set_i2c_addr_by_adr(ACM8625P_ADR_PULL_15K);
    if (ret) {
        return ret;
    }

    ret = divoom_acm8625p_amp_power_on();
    if (ret) {
        return ret;
    }

    ret = divoom_acm8625p_amp_apply_default_cfg();
    if (ret) {
        return ret;
    }

    /* 单阶段启动：init 内直接进入 PLAY 并完成检查/恢复 */
    return divoom_acm8625p_amp_start_play();
}

int divoom_acm8625p_amp_deinit(void)
{
    /*
     * 规格书 10.10：关电前需先关闭输出并等待至少 6ms。
     * 这里走寄存器进入 Digital Off，再拉低 PDN/POWER。
     */
    divoom_acm8625p_amp_set_mute(ACM8625P_MUTE_ON);
    divoom_acm8625p_amp_set_state(ACM8625P_STATE_DIGITAL_OFF);
    divoom_port_delay_ms((6000) / 1000U);
    return divoom_acm8625p_amp_power_off();
}
