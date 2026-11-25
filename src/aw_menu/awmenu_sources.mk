# AstroToo 真机 aw_menu 源文件清单（480×480 UI）
# Tina/Makefile 集成：仅编译 KEEP 列表 + AstroToo UI 源

KEEP_AW_MENU_SRC = \
	aw_menu/divoom_menu_control.c \
	aw_menu/divoom_menu_direct_ctrl.c \
	aw_menu/divoom_load_message.c \
	aw_menu/divoom_popup_menu.c \
	aw_menu/divoom_lvgl_font_manage.c \
	aw_menu/divoom_lvgl_message_box.c \
	aw_menu/divoom_lvgl_normal_menu_list.c \
	aw_menu/divoom_lvgl_movie_play.c \
	aw_menu/divoom_lvgl_poweron.c \
	aw_menu/divoom_astrootoo_config_wifi.c \
	aw_menu/divoom_astrootoo_oobe_config_wifi.c \
	aw_menu/divoom_astrootoo_oobe_welcome.c \
	aw_menu/divoom_lvgl_set_language.c

ASTROTOO_UI_SRC = \
	aw_menu/divoom_astrootoo_ui_base.c \
	aw_menu/divoom_astrootoo_watch.c \
	aw_menu/divoom_astrootoo_main_menu.c \
	aw_menu/divoom_astrootoo_language_route.c \
	aw_menu/divoom_astrootoo_settings.c \
	aw_menu/divoom_astrootoo_time_pick.c \
	aw_menu/divoom_astrootoo_settings_ex.c \
	aw_menu/divoom_astrootoo_settings_wifi.c \
	aw_menu/divoom_astrootoo_settings_tws.c \
	aw_menu/divoom_astrootoo_network_reset_confirm.c \
	aw_menu/divoom_astrootoo_alarm.c \
	aw_menu/divoom_astrootoo_tools.c \
	aw_menu/divoom_astrootoo_games.c \
	aw_menu/divoom_astrootoo_sleep.c \
	aw_menu/divoom_astrootoo_source.c \
	aw_menu/divoom_astrootoo_face_market.c \
	aw_menu/divoom_astrootoo_placeholder.c \
	aw_menu/divoom_astrootoo_menu_compat.c \
	aw_menu/divoom_astrootoo_menu_registry.c \
	aw_menu/divoom_buddy.c \
	aw_menu/divoom_buddy_config.c \
	aw_menu/divoom_fish_tank.c \
	aw_menu/divoom_astro_orrery.c \
	aw_menu/divoom_fire_breath.c \
	aw_menu/divoom_after_rain_bg.c \
	aw_menu/divoom_liquid_bubble.c \
	aw_menu/divoom_golden_hour_bg.c \
	aw_menu/divoom_horizon_indicator_bg.c \
	aw_menu/divoom_living_lyrics_bg.c \
	aw_menu/divoom_kinetic_lines_bg.c \
	aw_menu/divoom_meteor_star_bg.c \
	aw_menu/divoom_pendulum_bg.c \
	aw_menu/divoom_orbital_system_bg.c \
	aw_menu/divoom_particle_bloom_bg.c \
	aw_menu/divoom_retro_tv_bg.c \
	aw_menu/divoom_sand_waves_bg.c \
	aw_menu/divoom_sea_horizon_bg.c \
	aw_menu/divoom_star_field_bg.c \
	aw_menu/divoom_rain_trace_bg.c \
	aw_menu/divoom_seed_diffusion.c \
	aw_menu/divoom_silent_snow_bg.c \
	aw_menu/divoom_star_rain_horizon.c

GAME_RUNTIME_SRC = \
	middle/game_runtime/divoom_game_runtime_loader.c \
	middle/game_runtime/divoom_game_native_lua.c \
	middle/game_runtime/divoom_game_pack_install.c \
	middle/game_runtime/miniz.c \
	middle/game_runtime/miniz_tdef.c \
	middle/game_runtime/miniz_tinfl.c \
	middle/game_runtime/miniz_zip.c

# Lua 5.4：与模拟器同源，位于 src/lua-5.4（下小机代码，非 third_party）
LUA54_ROOT ?= $(abspath $(dir $(lastword $(MAKEFILE_LIST)))/../lua-5.4)
LUA54_SRC = \
	$(LUA54_ROOT)/src/lapi.c \
	$(LUA54_ROOT)/src/lauxlib.c \
	$(LUA54_ROOT)/src/lbaselib.c \
	$(LUA54_ROOT)/src/lcode.c \
	$(LUA54_ROOT)/src/lcorolib.c \
	$(LUA54_ROOT)/src/lctype.c \
	$(LUA54_ROOT)/src/ldblib.c \
	$(LUA54_ROOT)/src/ldebug.c \
	$(LUA54_ROOT)/src/ldo.c \
	$(LUA54_ROOT)/src/ldump.c \
	$(LUA54_ROOT)/src/lfunc.c \
	$(LUA54_ROOT)/src/lgc.c \
	$(LUA54_ROOT)/src/linit.c \
	$(LUA54_ROOT)/src/llex.c \
	$(LUA54_ROOT)/src/lmathlib.c \
	$(LUA54_ROOT)/src/lmem.c \
	$(LUA54_ROOT)/src/lobject.c \
	$(LUA54_ROOT)/src/lopcodes.c \
	$(LUA54_ROOT)/src/lparser.c \
	$(LUA54_ROOT)/src/lstate.c \
	$(LUA54_ROOT)/src/lstring.c \
	$(LUA54_ROOT)/src/lstrlib.c \
	$(LUA54_ROOT)/src/ltable.c \
	$(LUA54_ROOT)/src/ltablib.c \
	$(LUA54_ROOT)/src/ltm.c \
	$(LUA54_ROOT)/src/lundump.c \
	$(LUA54_ROOT)/src/lutf8lib.c \
	$(LUA54_ROOT)/src/lvm.c \
	$(LUA54_ROOT)/src/lzio.c

AW_MENU_SRC = $(KEEP_AW_MENU_SRC) $(ASTROTOO_UI_SRC) $(GAME_RUNTIME_SRC)
ifdef LUA54_ROOT
AW_MENU_SRC += $(LUA54_SRC)
CFLAGS += -I$(LUA54_ROOT)/src
endif

CFLAGS += -DDIVOOM_PRODUCT_ASTRO_TOO
