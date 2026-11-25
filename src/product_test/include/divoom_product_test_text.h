#ifndef DIVOOM_PRODUCT_TEST_TEXT_H
#define DIVOOM_PRODUCT_TEST_TEXT_H

/* Fixed Chinese caption when the factory font-28 snapshot is usable. */
#define PT_FACTORY_AGING_TEXT "老化模式"
#define PT_FACTORY_NEXT_TEXT "产品测试"

/* NEXT flow only: also used to snapshot every required glyph from font 28. */
#define PT_NEXT_TEXTS(X) \
    X(FONT_ERROR, "字体文件错误") \
    X(BACKLIGHT, "背光测试\n+ 调亮 / - 调暗\n按确认键继续") \
    X(SPEAKER, "扬声器测试") \
    X(MIC, "麦克风噪声测试\n%d 分贝") \
    X(KEY_FIRST, "按键测试\n模式键") \
    X(KEY, "按键测试\n%s") \
    X(KEY_MODE, "模式键") \
    X(KEY_LEFT, "左键") \
    X(KEY_UP, "上键") \
    X(KEY_RIGHT, "右键") \
    X(KEY_DOWN, "下键") \
    X(KEY_BACK, "返回键") \
    X(KEY_PLAY, "播放 / 暂停键") \
    X(LINE_IN, "音频输入") \
    X(USB, "通用串行接口") \
    X(TF, "存储卡") \
    X(UNKNOWN, "未知接口") \
    X(INSERT_PASS, "%s插拔测试\n#00FF00 测试通过#") \
    X(INSERT_REMOVE, "%s插拔测试\n请拔出%s") \
    X(INSERT_ADD, "%s插拔测试\n请插入%s") \
    X(CHECK_TITLE, "系统文件检测") \
    X(CHECK_ERRORS, "错误文件：%d") \
    X(CHECK_RUNNING, "正在校验文件…") \
    X(CHECK_READING, "正在读取校验清单…") \
    X(WAIT, "请稍候…") \
    X(CHECK_DONE, "文件检测完成") \
    X(CHECK_PASS, "系统文件检测通过") \
    X(CHECK_NEXT, "即将继续测试") \
    X(CHECK_FAILED, "文件检测失败（%d）") \
    X(CHECK_STOPPED, "检测未通过，已停止") \
    X(WIFI_OK, "无线网络已连接") \
    X(WIFI_CONNECTING, "正在连接无线网络…") \
    X(ALL_DONE, "全部完成") \
    X(RECEIVING, "正在接收文件（%d/%d）") \
    X(CHECKING, "正在检查…") \
    X(WAITING, "正在等待…") \
    X(DOWNLOADING, "正在下载固件 %d%%") \
    X(EXTRACTING, "正在解压固件 %d%%") \
    X(INSTALLING, "正在安装固件 %d%%") \
    X(JL_UPDATING, "正在更新蓝牙固件 %d%%") \
    X(GUIDE_FAILED, "资源清单获取失败") \
    X(FW_READY, "固件已就绪") \
    X(DOWNLOAD_FAILED, "下载失败") \
    X(URL_FAILED, "下载地址错误") \
    X(RESOURCE_FAILED, "资源更新失败") \
    X(FW_LATEST, "固件已是最新版本") \
    X(DOWNLOAD_DONE, "下载完成") \
    X(SERVER, "服务器") \
    X(UPDATE, "更新")

#define PT_NEXT_DEFINE(name, text) static const char PT_TEXT_##name[] = text;
PT_NEXT_TEXTS(PT_NEXT_DEFINE)
#undef PT_NEXT_DEFINE

#endif
