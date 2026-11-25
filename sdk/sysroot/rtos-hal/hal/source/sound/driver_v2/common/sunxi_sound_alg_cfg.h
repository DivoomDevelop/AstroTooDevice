/*
 * sound\snd_sunxi_rxsync.h
 * (C) Copyright 2025-2029
 * AllWinner Technology Co., Ltd. <www.allwinnertech.com>
 * xudongpdc <xudongpdc@allwinnertech.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 */
#ifndef __SUNXI_SOUND_ALG_CFG_H
#define __SUNXI_SOUND_ALG_CFG_H

#define ALG_CFG_PARM_LEN_MIN	14
#define ALG_CFG_PARM_LEN_MAX	24

enum SUNXI_ALG_CFG_DOMAIN {
	SUNXI_ALG_CFG_DOMAIN_MEQ = 0,
	SUNXI_ALG_CFG_DOMAIN_PEQ,
	SUNXI_ALG_CFG_DOMAIN_1BDRC,
	SUNXI_ALG_CFG_DOMAIN_3BDRC,
	SUNXI_ALG_CFG_DOMAIN_HPF,
};

struct alg_cfg_reg_domain {
	void *reg_base;
	unsigned int reg_min;
	unsigned int reg_max;
};

#if CONFIG_SUNXI_SOUND_COMMON_ALG_CFG
extern int sunxi_sound_alg_cfg_domain_set(struct alg_cfg_reg_domain *domain,
				    enum SUNXI_ALG_CFG_DOMAIN domain_sel);
extern int sunxi_sound_alg_cfg_domain_get(struct alg_cfg_reg_domain **domain,
				    enum SUNXI_ALG_CFG_DOMAIN domain_sel);
void sunxi_sound_alg_cfg_set(unsigned int reg, unsigned int reg_val,
			      struct alg_cfg_reg_domain *domain);

#else
static inline int sunxi_sound_alg_cfg_domain_set(struct alg_cfg_reg_domain *domain,
					   enum SUNXI_ALG_CFG_DOMAIN domain_sel)
{
	return 0;
}
static inline int sunxi_sound_alg_cfg_domain_get(struct alg_cfg_reg_domain **domain,
				    enum SUNXI_ALG_CFG_DOMAIN domain_sel)
{
	return 0;
}
static inline void sunxi_sound_alg_cfg_set(unsigned int reg, unsigned int reg_val,
				struct alg_cfg_reg_domain *domain)
{
}
#endif

#endif /* __SND_SUNXI_ALG_CFG_H */
