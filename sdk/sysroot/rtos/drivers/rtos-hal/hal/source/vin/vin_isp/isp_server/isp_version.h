/*
 * linux-4.9/drivers/media/platform/sunxi-vin/vin-isp/isp_server/isp_version.h
 *
 * Copyright (C) 2018 zhao wei.
 * Copyright (C) 2022 zheng zequn
 *
 * Authors: zhengzequn <zequnzheng@allwinnertech.com>
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#ifndef _ISP_VERSION_H_
#define _ISP_VERSION_H_

#include "include/isp_debug.h"

#if defined CONFIG_ARCH_SUN20IW3 //V851,V853
#define ISP_VERSION 600
#define ISP_SMALL_VERSION 100
#define REPO_TAG "isp-600-600-v1.00"

#define REPO_BRANCH "libisp-dev"
#define REPO_COMMIT "72981ac9c5c7523c2bfa456600f5ba78f4ccb149"
#define REPO_DATE "Thu May 16 15:23:07 2024 +0800"
#define RELEASE_AUTHOR "<mayifei@allwinnertech.com>"

#define FROM_REPO_BRANCH "libisp-dev"
#define FROM_REPO_COMMIT "72981ac9c5c7523c2bfa456600f5ba78f4ccb149"
#define FROM_REPO_DATE "Thu May 16 15:23:07 2024 +0800"
#elif defined CONFIG_ARCH_SUN55IW3 //AI985
#define ISP_VERSION 601
#define ISP_SMALL_VERSION 100
#define REPO_TAG "isp-601-600-v1.00"

#define REPO_BRANCH "libisp-dev"
#define REPO_COMMIT "28d384c0e77cce45d9ba8b62d489799898dab49e"
#define REPO_DATE "Thu Apr 13 15:42:52 2023 +0800"
#define RELEASE_AUTHOR "<mayifei@allwinnertech.com>"

#define FROM_REPO_BRANCH "libisp-dev"
#define FROM_REPO_COMMIT "6f800a4a53f3761c68f34785d8043f161e3e1906"
#define FROM_REPO_DATE "Thu Apr 13 14:44:39 2023 +0800"
#elif defined CONFIG_ARCH_SUN300IW1 //V821
#define ISP_VERSION 603
#define ISP_SMALL_VERSION 100
#define REPO_TAG "isp-603-600-v1.00"

#define REPO_BRANCH "libisp-dev"
#define REPO_COMMIT "72981ac9c5c7523c2bfa456600f5ba78f4ccb149"
#define REPO_DATE "Thu May 16 15:23:07 2024 +0800"
#define RELEASE_AUTHOR "<mayifei@allwinnertech.com>"

#define FROM_REPO_BRANCH "libisp-dev"
#define FROM_REPO_COMMIT "a68885726281bbfcffc43ec2c7ca5aaaf3d97c06"
#define FROM_REPO_DATE "Tue May 28 10:10:20 2024 +0800"
#endif

static inline void isp_version_info(void)
{
	ISP_PRINT(">>>>>>>>>>>> ISP VERSION INFO >>>>>>>>>>>\n"
		"IPCORE: ISP%d\n"
		"branch: %s\n"
		"commit: %s\n"
		"date  : %s\n"
		"author: %s\n"
		"-------------------------------------------\n\n",
		ISP_VERSION, REPO_BRANCH, REPO_COMMIT, REPO_DATE, RELEASE_AUTHOR);
}

#endif
