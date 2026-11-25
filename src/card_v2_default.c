#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <aw_common.h>
#ifndef CONFIG_KERNEL_FREERTOS
#include <init.h>
#endif
#include <script.h>
#include <hal_cfg.h>

#include "sunxi_machine.h"
#include "sunxi_sound_common.h"

#include <sound_v2/sound_adf_ksound.h>

static int sound_get_card_name(const char* prefix, const char* card, char* card_name)
{
	char key_name[32];
	char val_name[32];
	char get_card_name[32];
	int ret;

	snprintf(key_name, sizeof(key_name), "%s_mach", prefix);
	if (card)
		snprintf(val_name, sizeof(val_name), "%s-name", card);
	else
		snprintf(val_name, sizeof(val_name), "%s", "name");

	ret = hal_cfg_get_keyvalue(key_name, val_name, (int32_t *)get_card_name, sizeof(get_card_name) / sizeof(int));
	if (ret) {
		sound_debug("soundcards:%s name miss.\n", prefix);
	} else {
		snprintf(card_name, sizeof(get_card_name), "%s", get_card_name);
	}

	return ret;
}

int ksound_card_initialize(void)
{
	int ret = 0;
	char card_name[32];

	//maybe unused for compile warning.
	UNUSED(ret);
	UNUSED(card_name);

#ifdef CONFIG_KERNEL_FREERTOS
	sunxi_sound_card_register_component();
#endif

/* ------------------------------------------------------------------------- */
/* AUDIOCODEC */
/* ------------------------------------------------------------------------- */
#ifdef CONFIG_SOUND_PLATFORM_AAUDIO

	ret = sound_get_card_name("audiocodec", NULL, card_name);
	if (ret != 0) {
		sound_info("soundcards: sound_get_card_name failed, using default name\n");
		snprintf(card_name, sizeof(card_name), "%s", "audiocodec");
	}

	/* register audiocodec sound card */
	ret = sound_mach_probe(card_name, SOUND_PLATFORM_TYPE_AAUDIO);
	if (ret == 0) {
		sound_debug("soundcards: audiocodec register success!\n");
	} else {
		sound_err("soundcards: audiocodec register failed!\n");
	}
#endif

/* ------------------------------------------------------------------------- */
/* i2s */
/* ------------------------------------------------------------------------- */
#ifdef CONFIG_SOUND_PLATFORM_I2S0

	ret = sound_get_card_name("i2s0", NULL, card_name);
	if (ret != 0) {
		sound_info("soundcards: sound_get_card_name failed, using default name\n");
		snprintf(card_name, sizeof(card_name), "%s", "soundi2s0");
	}

	/* register i2s0 sound card */
	ret = sound_mach_probe(card_name, SOUND_PLATFORM_TYPE_I2S0);
	if (ret == 0) {
		sound_debug("soundcards: %s register success!\n", card_name);
	} else {
		sound_err("soundcards: %s register failed!\n", card_name);
	}
#endif

/* ------------------------------------------------------------------------- */
/* DMIC */
/* ------------------------------------------------------------------------- */
#ifdef CONFIG_SOUND_PLATFORM_DMIC

	ret = sound_get_card_name("dmic", NULL, card_name);
	if (ret != 0) {
		sound_info("soundcards: sound_get_card_name failed, using default name\n");
		snprintf(card_name, sizeof(card_name), "%s", "sounddmic");
	}

	/* register dmic sound card */
	ret = sound_mach_probe(card_name, SOUND_PLATFORM_TYPE_DMIC);
	if (ret == 0) {
		sound_debug("soundcards: %s register success!\n", card_name);
	} else {
		sound_err("soundcards: %s register failed!\n", card_name);
	}
#endif

/* ------------------------------------------------------------------------- */
/* OWA */
/* ------------------------------------------------------------------------- */
#ifdef CONFIG_SOUND_PLATFORM_OWA

	ret = sound_get_card_name("owa", NULL, card_name);
	if (ret != 0) {
		sound_info("soundcards: sound_get_card_name failed, using default name\n");
		snprintf(card_name, sizeof(card_name), "%s", "soundowa");
	}

	/* register owa sound card */
	ret = sound_mach_probe(card_name, SOUND_PLATFORM_TYPE_OWA);
	if (ret == 0) {
		sound_debug("soundcards: %s register success!\n", card_name);
	} else {
		sound_err("soundcards: %s register failed!\n", card_name);
	}
#endif

	/* Sound cards list */
	//sound_card_list();

	return 0;
}

int ksound_card_deinitialize(void)
{
	int ret = 0;
	char card_name[32];

	//maybe unused for compile warning.
	UNUSED(ret);
	UNUSED(card_name);
/* ------------------------------------------------------------------------- */
/* AUDIOCODEC */
/* ------------------------------------------------------------------------- */
#ifdef CONFIG_SOUND_PLATFORM_AAUDIO
	ret = sound_get_card_name("audiocodec", NULL, card_name);
	if (ret != 0) {
		sound_info("soundcards: sound_get_card_name failed, using default name\n");
		snprintf(card_name, sizeof(card_name), "%s", "audiocodec");
	}

	/* register audiocodec sound card */
	sound_mach_remove(card_name);
#endif

/* ------------------------------------------------------------------------- */
/* i2s */
/* ------------------------------------------------------------------------- */
#ifdef CONFIG_SOUND_PLATFORM_I2S0
	ret = sound_get_card_name("i2s0", NULL, card_name);
	if (ret != 0) {
		sound_info("soundcards: sound_get_card_name failed, using default name\n");
		snprintf(card_name, sizeof(card_name), "%s", "soundi2s0");
	}

	/* register i2s0 sound card */
	sound_mach_remove("soundi2s0");
#endif

/* ------------------------------------------------------------------------- */
/* DMIC */
/* ------------------------------------------------------------------------- */
#ifdef CONFIG_SOUND_PLATFORM_DMIC
	ret = sound_get_card_name("dmic", NULL, card_name);
	if (ret != 0) {
		sound_info("soundcards: sound_get_card_name failed, using default name\n");
		snprintf(card_name, sizeof(card_name), "%s", "sounddmic");
	}

	/* register dmic sound card */
	sound_mach_remove(card_name);
#endif

/* ------------------------------------------------------------------------- */
/* OWA */
/* ------------------------------------------------------------------------- */
#ifdef CONFIG_SOUND_PLATFORM_OWA
	ret = sound_get_card_name("owa", NULL, card_name);
	if (ret != 0) {
		sound_info("soundcards: sound_get_card_name failed, using default name\n");
		snprintf(card_name, sizeof(card_name), "%s", "soundowa");
	}

	/* register owa sound card */
	sound_mach_remove(card_name);
#endif

#ifdef CONFIG_KERNEL_FREERTOS
	sunxi_sound_card_unregister_component();
#endif

	return 0;
}

#ifndef CONFIG_KERNEL_FREERTOS
late_initcall(ksound_card_initialize);
#endif
