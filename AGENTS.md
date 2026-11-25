# AstroToo_open：开发者与 AI 工具规则

开始工作前阅读 [DEVELOPMENT.md](DEVELOPMENT.md)。它说明应用与固定 SDK 的边界、构建命令、接口规范及验收方法。本文规则适用于本目录及其子目录。

## 产测与烧录链路禁止修改

**不得修改、删除、裁剪、停编或设为空实现 `src/product_test/` 的产测程序。否则可能导致设备无法进入烧录模式，失去正常恢复能力。**

同时保护以下链路，不能在普通业务修改中间接破坏：

- `src/app/divoom_btcore_uart_cmd.c` 及相关头文件中 `DM_BTCORE_ENTER_BOOT_MODE` 的编号、接收与分发。
- `src/product_test/divoom_product_test.c` 的 `DIVOOM_PT_MSG_ENTER_EFEX` 和 `divoom_product_test_enter_efex_boot()`。
- `src/product_test/divoom_lvgl_product_test_menu.c` 中现有进入烧录的入口。
- `src/lvgl/divoom_lvgl_init.c` 和 `src/app/divoom_onoff.c` 的产测/恢复启动选择与状态交接。
- UART 通信、RTC/FEL 标志、看门狗、LCD 收尾/复位和固定 Bootloader 的现有配合次序。

不能把“编译通过”当作“仍能进入烧录”。需要变更产测或恢复机制时，交由维护者在内部完整工程独立处理并做硬件验证；普通开源应用任务不得顺手修改。

## 修改边界与并发协作

- 目标是本工程应用层，日常改动限于业务 SRC、必要应用头文件和 `app_sources.txt`。不修改内部 AstroToo、TimesGate 或其它项目。
- 不修改 `sdk/`、`toolchain/`、`sdk.lock`、板型、分区、固定资源、Bootloader、链接脚本或校验值来绕过失败。
- SDK 公共 ABI 不得单独改变；同一头文件不留多份副本，应用不得引用私有 LIB 目录。
- 工作区由用户共同编辑。每次写入前立即重读目标区域，保留任何外部变化，仅改任务必需的行；不得从 Git 基线、备份或候选树整文件回填用户内容。
- 保留工作线程数量。使用合理的阻塞同步，检查重入、锁顺序、失败清理和等待依赖；不能用全局串行化或忙等处理局部问题。
- 如存在 `.codegraph/`，定位和理解代码先用 CodeGraph；否则直接搜索，不擅自索引。

## 开源限制与安全

- 保持 DEVELOPMENT.md 第 7 节的限制：指定上传函数空实现、SLS/Musixmatch 禁用、共享 AES KEY/IV 与实现仅在 LIB（保持加解密行为，SRC 无 KEY 副本）、OSS 实现/凭据移除、YouTube 固定代理且不带 Google KEY、Google 日历/Outlook 功能保留、百炼/LyricFind 无功能且无凭据、`Device/UploadLog` 禁用、LAN 分享移除、时钟上传命令拒绝、天气回调不向 Divoom 发指定回退请求、MQTT 主机规则。
- `DIVOOM_OPENSOURCE_SWITCH=1` 只由维护者用于编译 LIB，不能扩散到应用编译。
- 不通过通用 HTTP/MQTT 或数字命令绕过被禁功能。保留公共声明不代表接口仍有功能。
- 当前设备收到的 OSS 照片任务应发送 Device/DelPhoto 原图/预览删除通知并跳过下载；保留该服务器命令，不清空 Bucket、不删除本地相册，不把入队成功当作云端删除完成。
- 不新增或输出真实密钥、token、认证头、生产密码和含凭据 URL；不访问真实服务试验凭据。静态库不能保证秘密不可提取。
- 安全扫描发现问题时记录脱敏位置，不擅自删除产测代码或其它未授权功能。
- `verification/`、`build/`、`out/` 可能含历史密钥或私有实现，不能因被 Git 忽略就随整目录公开。

## 构建与验收

```bash
bash build.sh --verify-only
bash build.sh --jobs 8
# 需要清理时：
bash build.sh --clean
bash build.sh --jobs 8
```

使用 Linux/WSL2 与附带工具链。新增 `src/app/my_feature.c` 时，将 `app/my_feature.c` 加入 `app_sources.txt`。不能同时执行构建、清理和 SDK 同步，不手工删除操作锁。

对代码变更完成实际构建与必要业务验证；纯文档变更核对路径、命令和内容即可。维护者同步 SDK 还需候选/正式打包、产物校验与开源策略行为验收。未上板就明确标注未验证硬件及再次进入烧录。

完成任务时说明修改内容、验证结果、SDK ID（涉及构建时）和剩余限制。不自动推送、公开发布、上传固件/OTA 或烧写设备。
