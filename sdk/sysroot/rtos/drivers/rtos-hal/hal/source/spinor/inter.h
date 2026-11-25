#ifndef _SPINOR_INTER_H_
#define _SPINOR_INTER_H_

#include <sunxi_hal_common.h>
#include <sunxi_hal_spi.h>
#include <sunxi_hal_spinor.h>
#include <sunxi_hal_spi_mem.h>
#include <hal_log.h>
#include <hal_mutex.h>

#ifdef CONFIG_DRIVERS_SPIF
#include <sunxi_hal_spif.h>
#endif

#define SPINOR_FMT(fmt) "spinor: "fmt
#define SPINOR_ERR(fmt, arg...) hal_log_err(SPINOR_FMT(fmt), ##arg)
#define SPINOR_WARN(fmt, arg...) hal_log_warn(SPINOR_FMT(fmt), ##arg)
#define SPINOR_DEBUG(fmt, arg...) hal_log_debug(SPINOR_FMT(fmt), ##arg)
#define SPINOR_INFO(fmt, arg...) hal_log_info(SPINOR_FMT(fmt), ##arg)

#define DIV_ROUND_UP(n,d) (((n) + (d) - 1) / (d))
#define BITS_PER_BYTE 8
#define BITS_PER_LONG (sizeof(long) * BITS_PER_BYTE)

#ifndef BITS_TO_LONGS
#define BITS_TO_LONGS(nr) DIV_ROUND_UP(nr, BITS_PER_BYTE * sizeof(long))
#endif

#define SZ_4K       (4 * 1024)
#define SZ_32K      (32 * 1024)
#define SZ_64K      (64 * 1024)
#define SZ_128K     (128 * 1024)
#define SZ_256K     (256 * 1024)
#define SZ_512K     (512 * 1024)
#define SZ_1M       (1 * 1024 * 1024)
#define SZ_2M       (2 * 1024 * 1024)
#define SZ_4M       (4 * 1024 * 1024)
#define SZ_8M       (8 * 1024 * 1024)
#define SZ_12M      (12 * 1024 * 1024)
#define SZ_14M      (14 * 1024 * 1024)
#define SZ_15M      (15 * 1024 * 1024)
#define SZ_15872K   (15872 * 1024)
#define SZ_16128K   (16128 * 1024)
#define SZ_16M      (16 * 1024 * 1024)
#define SZ_32M      (32 * 1024 * 1024)
#define SZ_64M      (64 * 1024 * 1024)

#define MAX_ID_LEN 3
#define MAX_WAIT_LOOP (((unsigned int)(-1))/2)
#define NOR_PAGE_SIZE 256
#define NOR_HALF_BLK_SIZE (SZ_32K)
#define NOR_BLK_SIZE (SZ_64K)

#define FACTORY_ZETTA       0xBA
#define FACTORY_ZETTA_ZB    0x5E
#define FACTORY_PUYA        0x85
#define FACTORY_MXIC        0xC2
#define FACTORY_GD          0xC8
#define FACTORY_WINBOND     0xEF
#define FACTORY_FM          0xA1
#define FACTORY_ESMT        0x1C
#define FACTORY_XTX         0x0B
#define FACTORY_XMC         0x20
#define FACTORY_BOYA        0x68
#define FACTORY_KH          0xC2
#define FACTORY_ISSI        0x9D

/* Used for secutity rigister */
#define SR_OP_PROGRAM   0x42
#define SR_OP_ERASE     0x44
#define SR_OP_READ      0x48
#define LB_OFS          2

/* spinor cmd */
#define NOR_CMD_READ			(0x03)
#define NOR_CMD_FAST_READ		(0x0B)
#define NOR_CMD_DUAL_READ		(0x3B)
#define NOR_CMD_QUAD_READ		(0x6B)
#define NOR_CMD_DUAL_IO_READ		(0xBB)
#define NOR_CMD_QUAD_IO_READ		(0xEB)
#define NOR_CMD_PROG			(0x02)
#define NOR_CMD_QUAD_PROG		(0x32)
#define NOR_CMD_QUAD_IO_PROG		(0x38)
#define NOR_CMD_ERASE_BLK4K		(0x20)
#define NOR_CMD_ERASE_BLK32K		(0x52)
#define NOR_CMD_ERASE_BLK64K		(0xD8)
#define NOR_CMD_ERASE_CHIP		(0x60)
#define NOR_CMD_WREN			(0x06)
#define NOR_CMD_WRDI			(0x04)
#define NOR_CMD_READ_SR			(0x05)  /* Read status register 1 */
#define NOR_CMD_WRITE_SR		(0x01)  /* Write status register 1 */
#define NOR_CMD_READ_SR2		(0x35)  /* Read status register 2 */
#define NOR_CMD_WRITE_SR2		(0x31)	/* Write status register 2 */
#define NOR_CMD_READ_SCUR		(0x2B)
#define NOR_CMD_RESET_EN		(0x66)
#define NOR_CMD_RESET			(0x99)
#define NOR_CMD_RDID			(0x9F)
#define NOR_CMD_READ_1_2_2		(0xbb)  /* Read data bytes (Dual I/O SPI) */
#define NOR_CMD_READ_1_4_4		(0xeb)  /* Read data bytes (Quad I/O SPI) */
#define NOR_CMD_READ_1_8_8		(0xcb)  /* Read data bytes (Octal I/O SPI) */
#define NOR_CMD_READ_1_1_1_DTR		(0x0d)
#define NOR_CMD_READ_1_2_2_DTR		(0xbd)
#define NOR_CMD_READ_1_4_4_DTR		(0xed)

#define cmd_4bytes(cmd) (nor->addr_width == 4 ? cmd##4B : cmd)
#define NOR_CMD_EN4B			(0xB7)
#define NOR_CMD_EX4B			(0xE9)
#define NOR_CMD_MXC_EX4B		(0x29)
#define NOR_CMD_READ4B			(0x13)
#define NOR_CMD_FAST_READ4B		(0x0C)
#define NOR_CMD_DUAL_READ4B		(0x3C)
#define NOR_CMD_QUAD_READ4B		(0x6C)
#define NOR_CMD_DUAL_IO_READ4B		(0xBC)
#define NOR_CMD_QUAD_IO_READ4B		(0xEC)
#define NOR_CMD_PROG4B			(0x12)
#define NOR_CMD_QUAD_PROG4B		(0x34)
#define NOR_CMD_QUAD_IO_PROG4B		(0x3E)
#define NOR_CMD_ERASE_BLK4K4B		(0x21)
#define NOR_CMD_ERASE_BLK32K4B		(0x5C)
#define NOR_CMD_ERASE_BLK64K4B		(0xDC)
#define NOR_CMD_READ_1_2_2_4B		(0xbc)  /* Read data bytes (Dual I/O SPI) */
#define NOR_CMD_READ_1_4_4_4B		(0xec)  /* Read data bytes (Quad I/O SPI) */
#define NOR_CMD_READ_1_8_8_4B		(0xcc)  /* Read data bytes (Octal I/O SPI) */
#define NOR_CMD_READ_1_1_1_DTR_4B	(0x0e)
#define NOR_CMD_READ_1_2_2_DTR_4B	(0xbe)
#define NOR_CMD_READ_1_4_4_DTR_4B	(0xee)

#define CHECK_DTR_OP(opcode)	((opcode == NOR_CMD_READ_1_1_1_DTR || \
				opcode == NOR_CMD_READ_1_2_2_DTR || \
				opcode == NOR_CMD_READ_1_4_4_DTR || \
				opcode == NOR_CMD_READ_1_1_1_DTR_4B || \
				opcode == NOR_CMD_READ_1_2_2_DTR_4B || \
				opcode == NOR_CMD_READ_1_4_4_DTR_4B) \
				? 1 : 0)

#define CHECK_IO_OP(opcode)	(CHECK_DTR_OP(opcode) || \
				((opcode == NOR_CMD_READ_1_2_2 || \
				opcode == NOR_CMD_READ_1_4_4 || \
				opcode == NOR_CMD_READ_1_8_8 || \
				opcode == NOR_CMD_READ_1_2_2_4B || \
				opcode == NOR_CMD_READ_1_4_4_4B || \
				opcode == NOR_CMD_READ_1_8_8_4B) \
				? 1 : 0))

#define CHECK_4B_READ_OP(opcode)    ((opcode == NOR_CMD_READ4B || \
				opcode == NOR_CMD_DUAL_READ4B || \
				opcode == NOR_CMD_QUAD_READ4B || \
				opcode == NOR_CMD_FAST_READ4B || \
				opcode == NOR_CMD_READ_1_4_4_DTR_4B) \
				? 1 : 0)

#define CHECK_4B_OP(opcode)    ((CHECK_4B_READ_OP(opcode) || \
				opcode == NOR_CMD_QUAD_PROG4B || \
				opcode == NOR_CMD_READ_1_4_4_DTR_4B) \
								? 1 : 0)

#define CHECK_READ_OP(opcode)  ((CHECK_4B_READ_OP(opcode) || \
				opcode == NOR_CMD_READ || \
				opcode == NOR_CMD_DUAL_READ || \
				opcode == NOR_CMD_QUAD_READ || \
				opcode == NOR_CMD_FAST_READ || \
				opcode == NOR_CMD_READ_1_4_4_DTR) \
				? 1 : 0)

#define CHECK_PP_SE_OP(opcode) ((opcode == NOR_CMD_PROG || \
				opcode == NOR_CMD_PROG4B || \
				opcode == NOR_CMD_QUAD_PROG ||\
				opcode == NOR_CMD_QUAD_PROG4B || \
				opcode == NOR_CMD_ERASE_BLK4K || \
				opcode == NOR_CMD_ERASE_BLK32K || \
				opcode == NOR_CMD_ERASE_BLK64K || \
				opcode == NOR_CMD_ERASE_BLK4K4B || \
				opcode == NOR_CMD_ERASE_BLK32K4B || \
				opcode == NOR_CMD_ERASE_BLK64K4B) \
				? 1 : 0)

#define CHECK_DUMMY_OP(opcode) ((opcode == NOR_CMD_READ || \
				opcode == NOR_CMD_READ4B) \
				? 0 : 1)


/**
 * struct spi_nor_otp_organization - Structure to describe the SPI NOR OTP regions
 * @len:	size of one OTP region in bytes.
 * @base:	start address of the OTP area.
 * @offset:	offset between consecutive OTP regions if there are more
 *              than one.
 * @n_regions:	number of individual OTP regions.
 */
struct spi_nor_otp_organization {
    unsigned int len;
    unsigned int base;
    unsigned int offset;
    unsigned int n_regions;
};

struct nor_info
{
    char *model;
    unsigned char id[MAX_ID_LEN];
    unsigned int total_size;

    unsigned int flag;
#define SUPPORT_4K_ERASE_BLK        BIT(0)
#define SUPPORT_32K_ERASE_BLK       BIT(1)
#define SUPPORT_64K_ERASE_BLK       BIT(2)
#define SUPPORT_DUAL_READ           BIT(3)
#define SUPPORT_QUAD_READ           BIT(4)
#define SUPPORT_QUAD_WRITE          BIT(5)
#define SUPPORT_INDIVIDUAL_PROTECT  BIT(6)
#define SUPPORT_WITHOUT_PROTECT  	BIT(7)
#define SUPPORT_ALL_ERASE_BLK       (SUPPORT_4K_ERASE_BLK | \
                                     SUPPORT_32K_ERASE_BLK | \
                                     SUPPORT_64K_ERASE_BLK)
#define SUPPORT_GENERAL             (SUPPORT_ALL_ERASE_BLK | \
                                     SUPPORT_QUAD_WRITE | \
                                     SUPPORT_QUAD_READ | \
                                     SUPPORT_DUAL_READ)
#define USE_4K_ERASE                BIT(20)
#define USE_IO_PROG_X4              BIT(21)
#define USE_IO_READ_X2              BIT(22)
#define USE_IO_READ_X4              BIT(23)
#define USE_RX_DTR		    BIT(24)
#define USE_TX_DTR		    BIT(25)
    int freq;

    const struct spi_nor_otp_organization otp_org;
};

#define OTP_INFO(_len, _n_regions, _base, _offset)        \
            .otp_org.len = (_len),                        \
            .otp_org.n_regions = (_n_regions),            \
            .otp_org.base = (_base),                      \
            .otp_org.offset = (_offset),                  \

struct nor_spi_master
{
    int port;
    hal_spi_master_config_t cfg;
};

#ifdef CONFIG_DRIVERS_SPINOR
struct nor_flash
{
    unsigned char cmd_read;
    unsigned char cmd_write;

    unsigned int r_cmd_slen: 3;
    unsigned int w_cmd_slen: 3;
    unsigned int total_size;
    unsigned int blk_size;
    unsigned int page_size;
    unsigned int addr_width;
    unsigned int read_dummy;
    enum spi_nor_protocol read_proto;
    enum spi_nor_protocol write_proto;

#ifdef CONFIG_DRIVERS_SPIF
    struct sunxi_spif sspi;
#else
    struct nor_spi_master spim;
#endif
    struct nor_info *info;
    struct nor_factory *factory;

    hal_mutex_t hal_mutex;
    int (*transfer_one)(int, void *, int, void *, int);
    unsigned char io_mode;
};
#else
struct nor_flash
{
    unsigned char cmd_read;
    unsigned char cmd_write;

    unsigned int r_cmd_slen: 3;
    unsigned int w_cmd_slen: 3;
    unsigned int total_size;
    unsigned int blk_size;
    unsigned int page_size;
    unsigned int addr_width;

    struct nor_info *info;
    struct nor_factory *factory;

    hal_mutex_t hal_mutex;
    int (*transfer_one)(int, void *, int, void *, int);
};

#endif

struct nor_factory {
    unsigned char factory;
    unsigned int idt_cnt;
    struct nor_info *idt;

    int (*init)(struct nor_flash *nor);
    void (*deinit)(struct nor_flash *nor);
    int (*init_lock)(struct nor_flash *nor);
    void (*deinit_lock)(struct nor_flash *nor);
    int (*lock)(struct nor_flash *nor, unsigned int addr, unsigned int len);
    int (*unlock)(struct nor_flash *nor, unsigned int addr, unsigned int len);
    bool (*islock)(struct nor_flash *nor, unsigned int addr, unsigned int len);
    int (*set_quad_mode)(struct nor_flash *nor);
    int (*set_4bytes_addr)(struct nor_flash *nor, int enable);
    int (*read_uuid)(struct nor_flash *nor, void* uuid);

    struct nor_factory *next;
};

int nor_register_factory(struct nor_factory *f);
int nor_register_factory_gd(void);
int nor_register_factory_mxic(void);
int nor_register_factory_winbond(void);
int nor_register_factory_xtx(void);
int nor_register_factory_esmt(void);
int nor_register_factory_fm(void);
int nor_register_factory_xmc(void);
int nor_register_factory_puya(void);
int nor_register_factory_zetta(void);
int nor_register_factory_zetta_zb(void);
int nor_register_factory_boya(void);
int nor_register_factory_kh(void);
int nor_register_factory_issi(void);

int nor_transfer(int single_len, void *tbuf, int tlen, void *rbuf, int rlen);
int nor_send_cmd(unsigned char cmd);
int nor_read_status(unsigned char *sr);
int nor_write_status(unsigned char *sr, unsigned int len);
int nor_wait_ready(int ms, int times);
int nor_write_enable(void);
int nor_read_uuid(void *uuid);

int nor_otp_read(u32 region, u32 len, u8 *buf);
int nor_otp_write(u32 region, u32 len, u8 *buf);

int nor_init(void);
int nor_deinit(void);
int nor_write(unsigned int addr, char *buf, unsigned int len);
int nor_read(unsigned int addr, char *buf, unsigned int len);
int nor_erase(unsigned int addr, unsigned int size);
int nor_ioctrl(int cmd, void *buf, unsigned int size);
struct nor_flash *get_nor_flash(void);

#ifdef CONFIG_DRIVERS_SPINOR_PANIC_WRITE
int nor_panic_write(unsigned int addr, char *buf, unsigned int len);
int nor_panic_read(unsigned int addr, char *buf, unsigned int len);
int nor_panic_erase(unsigned int addr, unsigned int size);
#endif

#ifdef CONFIG_DRIVERS_SPINOR_CACHE
int nor_cache_init(struct nor_flash *nor);
void nor_cache_exit(void);
int nor_cache_read(unsigned int addr, char *buf, unsigned int len);
int nor_cache_write(unsigned int addr, char *buf, unsigned int len);
int nor_cache_sync(void);
int nor_cache_erase(unsigned int addr, unsigned int len);
#endif /* CONFIG_DRIVERS_SPINOR_CACHE */

#ifdef CONFIG_DRIVERS_SPINOR_WRITE_LOCK
int nor_wr_lock_init(struct nor_flash *nor);
void nor_wr_lock_deinit(struct nor_flash *nor);
int nor_wr_lock(struct nor_flash *nor, unsigned int addr, unsigned int len);
int nor_wr_unlock(struct nor_flash *nor, unsigned int addr, unsigned int len);
bool nor_wr_islock(struct nor_flash *nor, unsigned int addr, unsigned int len);
int nor_wr_unlock_all(struct nor_flash *nor);
int nor_wr_lock_all(struct nor_flash *nor);
#endif /* CONFIG_DRIVERS_SPINOR_WRITE_LOCK */

#ifdef CONFIG_DRIVERS_SPINOR_WRITE_LOCK
#define is_wrlock_work(nor) (nor->info->flag & SUPPORT_INDIVIDUAL_PROTECT)
#else
#define is_wrlock_work(nor) (false)
#endif

#endif
