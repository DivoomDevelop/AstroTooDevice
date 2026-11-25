/* Copyright (c) 2019-2025 Allwinner Technology Co., Ltd. ALL rights reserved.

 * Allwinner is a trademark of Allwinner Technology Co.,Ltd., registered in
 * the the People's Republic of China and other countries.
 * All Allwinner Technology Co.,Ltd. trademarks are used with permission.

 * DISCLAIMER
 * THIRD PARTY LICENCES MAY BE REQUIRED TO IMPLEMENT THE SOLUTION/PRODUCT.
 * IF YOU NEED TO INTEGRATE THIRD PARTY’S TECHNOLOGY (SONY, DTS, DOLBY, AVS OR MPEGLA, ETC.)
 * IN ALLWINNERS’SDK OR PRODUCTS, YOU SHALL BE SOLELY RESPONSIBLE TO OBTAIN
 * ALL APPROPRIATELY REQUIRED THIRD PARTY LICENCES.
 * ALLWINNER SHALL HAVE NO WARRANTY, INDEMNITY OR OTHER OBLIGATIONS WITH RESPECT TO MATTERS
 * COVERED UNDER ANY REQUIRED THIRD PARTY LICENSE.
 * YOU ARE SOLELY RESPONSIBLE FOR YOUR USAGE OF THIRD PARTY’S TECHNOLOGY.


 * THIS SOFTWARE IS PROVIDED BY ALLWINNER"AS IS" AND TO THE MAXIMUM EXTENT
 * PERMITTED BY LAW, ALLWINNER EXPRESSLY DISCLAIMS ALL WARRANTIES OF ANY KIND,
 * WHETHER EXPRESS, IMPLIED OR STATUTORY, INCLUDING WITHOUT LIMITATION REGARDING
 * THE TITLE, NON-INFRINGEMENT, ACCURACY, CONDITION, COMPLETENESS, PERFORMANCE
 * OR MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 * IN NO EVENT SHALL ALLWINNER BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS, OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _CE_REG_0_6_H_
#define _CE_REG_0_6_H_

#include "platform_ce.h"

/* CE: Crypto Engine*/
/* Reg Offset */
#define CE_REG_CTL			0x00
#define CE_REG_KEY_BASE		0x04
#define CE_REG_KEY(n)		(CE_REG_KEY_BASE + 4 * n)
#define CE_REG_IV_BASE		0x24
#define CE_REG_IV(n)		(CE_REG_IV_BASE + 4 * n)
#define CE_REG_CNT_BASE		0x34
#define CE_REG_CNT(n)		(CE_REG_CNT_BASE + 4 * n)
#define CE_REG_FCSR			0x44
#define CE_REG_ICSR			0x48
#define CE_REG_MD_BASE		0x4C
#define CE_REG_MD(n)		((n < 5) ? (CE_REG_MD_BASE + 4 * n) : (CE_REG_MD_BASE + 0x40 + 4 * n))
#define CE_REG_CTS_LEN		0x60
#define CE_REG_RXFIFO		0x200
#define CE_REG_TXFIFO		0x204

/* Bit Offset */
#define CE_REG_FCSR_TXFIFO_TRIG_LEVEL_SHIFT 0
#define CE_REG_FCSR_TXFIFO_TRIG_LEVEL_MASK	(0x1F << CE_REG_FCSR_TXFIFO_TRIG_LEVEL_SHIFT)

#define CE_REG_FCSR_RXFIFO_TRIG_LEVEL_SHIFT 8
#define CE_REG_FCSR_RXFIFO_TRIG_LEVEL_MASK	(0x1F << CE_REG_FCSR_RXFIFO_TRIG_LEVEL_SHIFT)

#define CE_REG_CTL_KEY_SELECT_SHIFT	24
#define CE_REG_CTL_KEY_SELECT_MASK	(0x4 << CE_REG_CTL_KEY_SELECT_SHIFT)

#define CE_REG_CTL_KEY_SIZE_SHIFT	8
#define CE_REG_CTL_KEY_SIZE_MASK	(0x3 << CE_REG_CTL_KEY_SIZE_SHIFT)

#define CE_REG_CTL_IV_MODE_SHIFT	14
#define CE_REG_CTL_IV_MODE_MASK		(1 << CE_REG_CTL_IV_MODE_SHIFT)

#define CE_REG_CTL_OP_DIR_SHIFT		7
#define CE_REG_CTL_OP_DIR_MASK		(0x1 << CE_REG_CTL_OP_DIR_SHIFT)

#define CE_REG_CTL_METHOD_SHIFT		4
#define CE_REG_CTL_METHOD_MASK		(0x7 << CE_REG_CTL_METHOD_SHIFT)

#define CE_REG_CTL_OP_MODE_SHIFT	12
#define CE_REG_CTL_OP_MODE_MASK		(0x3 << CE_REG_CTL_OP_MODE_SHIFT)

#define CE_REG_CTL_PRNG_MODE_SHIFT	15
#define CE_REG_CTL_PRNG_MODE_MASK	(1 << CE_REG_CTL_PRNG_MODE_SHIFT)

#define CE_REG_CTL_CTR_SIZE_SHIFT	10
#define CE_REG_CTL_CTR_SIZE_MASK	(0x3 << CE_REG_CTL_CTR_SIZE_SHIFT)

#define CE_REG_CTL_SHA_END_SHIFT	2
#define CE_REG_CTL_SHA_END_MASK		(0x1 << CE_REG_CTL_SHA_END_SHIFT)

#define CE_CTL_START			1
#define CE_REG_CTL_START_MASK		0x1

#define CE_REG_ICSR_RXFIFO_EMP_PENDING	BIT(10)
#define CE_REG_ICSR_TXFIFO_AVA_PENDING	BIT(8)
#define CE_REG_FCSR_RXFIFO_STATUS		BIT(30)
#define CE_REG_FCSR_TXFIFO_STATUS		BIT(22)
#define CE_REG_ICSR_DRA_ENABLE			BIT(4)

/* Bit-Val */
#define CE_AES_KEY_SIZE_128		0
#define CE_AES_KEY_SIZE_192		1
#define CE_AES_KEY_SIZE_256		2

#define CE_KEY_SELECT_INPUT		0
#define CE_KEY_SELECT_SSK		1
#define CE_KEY_SELECT_INTERNAL_0	3
#define CE_KEY_SELECT_INTERNAL_1	4
#define CE_KEY_SELECT_INTERNAL_2	5
#define CE_KEY_SELECT_INTERNAL_3	6
#define CE_KEY_SELECT_INTERNAL_4	7
#define CE_KEY_SELECT_INTERNAL_5	8
#define CE_KEY_SELECT_INTERNAL_6	9
#define CE_KEY_SELECT_INTERNAL_7	10

#define CE_CTR_SIZE_16				16
#define CE_CTR_SIZE_32				32
#define CE_CTR_SIZE_64				64
#define CE_CTR_SIZE_128				128
#define CE_CNT_WIDTH_16				0
#define CE_CNT_WIDTH_32				1
#define CE_CNT_WIDTH_64				2
#define CE_CNT_WIDTH_128			3

#define CE_IV_MODE_CONSTANT			0

#define CE_DIR_ENCRYPT				0
#define CE_DIR_DECRYPT				1

#define CE_METHOD_AES				0
#define CE_METHOD_DES				1
#define CE_METHOD_3DES				2
#define CE_METHOD_SHA1				3
#define CE_METHOD_MD5				4
#define CE_METHOD_PRNG				5
#define CE_METHOD_SHA256			6

#define CE_AES_MODE_ECB				0
#define CE_AES_MODE_CBC				1
#define CE_AES_MODE_CTR				2
#define CE_AES_MODE_CTS				3

/* The identification string to indicate the key source. */
#define CE_KS_SSK			"KEY_SEL_SSK"
#define CE_KS_HUK			"KEY_SEL_HUK"
#define CE_KS_INTERNAL_0	"KEY_SEL_INTRA_0"
#define CE_KS_INTERNAL_1	"KEY_SEL_INTRA_1"
#define CE_KS_INTERNAL_2	"KEY_SEL_INTRA_2"
#define CE_KS_INTERNAL_3	"KEY_SEL_INTRA_3"
#define CE_KS_INTERNAL_4	"KEY_SEL_INTRA_4"
#define CE_KS_INTERNAL_5	"KEY_SEL_INTRA_5"
#define CE_KS_INTERNAL_6	"KEY_SEL_INTRA_6"
#define CE_KS_INTERNAL_7	"KEY_SEL_INTRA_7"

#define CE_PRNG_START			2
#define CE_RNG_MODE_ONESHOT			0
#define CE_PRNG_MAX_RAND_LEN		20 /* 5 word = 20 bytes */
#define CE_PRNG_SEED_NUM		24 /* 6 word = 24 bytes */

#ifdef __cplusplus
extern "C" {
#endif

void hal_ce_clock_init(void);
void hal_ce_fifo_init(void);
uint32_t hal_ce_reg_rd(uint32_t offset);
void hal_ce_reg_wr(uint32_t offset, uint32_t val);

void hal_ce_key_set(char *key, int size);

int hal_ce_pending_get(void);
void hal_ce_pending_clear(void);
void hal_ce_irq_enable(int flow);
void hal_ce_irq_disable(int flow);

void hal_ce_iv_set(unsigned char *iv, int size);
void hal_ce_iv_mode_set(int mode);

void hal_ce_cnt_set(unsigned char *cnt, int size);
void hal_ce_cnt_get(int flow, char *cnt, int size);

void hal_ce_cts_size_set(int size);

void hal_ce_md_get(char *dst, char *src, int size);
void hal_ce_sha_final(void);
void hal_ce_check_sha_end(void);

void hal_ce_method_set(int dir, int type);
void hal_ce_aes_mode_set(int mode);
void hal_ce_rng_mode_set(int mode);

int hal_ce_flow_err(int flow);

int hal_ce_reg_print(char *buf, int len);
void hal_ce_keyselect_set(int select);
void hal_ce_keysize_set(int size);
void hal_ce_ctrl_start(void);
void hal_ce_ctrl_stop(void);

void hal_ce_wait_finish(uint32_t flow);

#ifdef __cplusplus
}
#endif

#endif /* end of _CE_REG_0_6_H_ */
