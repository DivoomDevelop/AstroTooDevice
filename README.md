# AstroToo 应用工程与固定 SDK

本工程用于修改应用源码、与预编译底层链接并生成完整烧录包。原 AstroToo 工程是导出时的只读输入，日常构建不需要它，也不会调用它的 Makefile、配置或打包脚本。

开发者与 AI 工具请先阅读 [开发说明与规范](DEVELOPMENT.md) 和 [AI 执行规则](AGENTS.md)。当前工程结构与规则以开发说明和现行 SDK 为准；下方按日期记录的旧导出信息保留作历史参考。

> **产测程序及进入烧录模式的链路禁止修改。否则可能导致设备无法进入烧录，失去正常恢复能力。** 保护范围和入口见开发说明第 2 节。

## 环境与一键构建

### 1. 准备 Linux 环境

使用 x86_64 Linux 和 Python 3.8 或以上。Windows 用户可在 x86_64 电脑上通过管理员 PowerShell 执行 `wsl --install -d Ubuntu`，按提示重启后进入 WSL2 Ubuntu。用 `wsl -l -v` 查看 Ubuntu 的 WSL 版本。后续命令都在 Linux 终端执行，建议把仓库克隆到 Linux 家目录（如 `~/AstroTooDevice`）。

在 Ubuntu 中安装依赖。后两个包提供全志打包程序 `dragon` 所需的 32 位运行库。Windows 上的 WSL 安装步骤见 [Microsoft WSL 安装文档](https://learn.microsoft.com/windows/wsl/install)。

```bash
sudo apt update
sudo apt install -y git git-lfs python3 make libc6-i386 lib32stdc++6
```

### 2. 获取源码和固定工具链

```bash
git lfs install
cd ~
git clone https://github.com/DivoomDevelop/AstroTooDevice.git
cd AstroTooDevice
git lfs pull
```

Git LFS 用于下载固定 SDK 的两个资源镜像。仓库自带 Linux x86_64 版玄铁 Xuantie V3.3.0 工具链，GCC 版本为 14.3.0。工具名以 `riscv64-unknown-elf-` 开头，构建脚本按板型参数生成 F101-S3 的 C907 RV32 固件。

在工程根目录运行以下命令，确认交叉编译器可执行。构建脚本按 `sdk.lock` 对应的清单直接定位工具链，无需设置 `PATH` 或 `CROSS_COMPILE`。

```bash
TC="$PWD/toolchain/Xuantie-900-gcc-elf-newlib-x86_64-V3.3.0/bin"
"$TC/riscv64-unknown-elf-gcc" --version
```

### 3. 校验并构建

```bash
bash build.sh --verify-only     # 检查固定 SDK 和工具链的完整性
bash build.sh --jobs 8          # 编译应用、链接、校验并打包
```

构建后可用同一套工具链检查 ELF 架构。

```bash
"$TC/riscv64-unknown-elf-readelf" -h out/rt_system.elf
```

其他命令：

```bash
bash build.sh --rebuild --jobs 8 # 全量重编应用，不编译底层
bash build.sh --no-pack         # 只生成 ELF/BIN
bash build.sh --pack-only       # 重新打包已有 out/rt_system.bin
bash build.sh --clean           # 仅删除本工程 build/ 和 out/
```

也可以使用 `make`、`make verify`、`make rebuild`、`make pack`、`make clean`。

## 目录与修改范围

| 目录／文件 | 内容及用途 |
|---|---|
| `src/` | 原 `lichee/rtos/projects/f101s3/astrotoo/src` 的应用源码快照，可修改 |
| `app_sources.txt` | 当前配置启用的应用编译单元清单，可添加新源文件 |
| `sdk/lib/` | 固定底层的普通静态库及原有厂商链接库，不引用原工程 |
| `sdk/sysroot/` | 应用编译需要的接口头文件，保留原目录层级 |
| `sdk/config/` | 固定板型、配置、每个编译单元的准确编译参数和链接参数 |
| `sdk/linker/` | 已预处理的固定链接脚本 |
| `sdk/pack/` | 固定 Bootloader、分区、资源镜像和打包工具 |
| `toolchain/` | 固定交叉编译工具链 |
| `sdk.lock`、`sdk/manifest.json` | SDK 版本及完整性校验，不属于防破解机制 |
| `build/` | 应用对象、依赖、逐文件编译日志和打包暂存 |
| `out/` | 新生成的固件和构建报告 |
| `verification/` | 本次导出和验证记录，不是用户编译依赖 |

SDK 与工具链在构建前逐文件校验。配置变化应由维护者发布新 SDK，而不是让用户修改 `defconfig`、`autoconf.h` 或链接脚本。`src` 内原有 Makefile 作为源码快照保留，新构建入口不执行它们，不提供 `menuconfig` 或 `mboot`。

修改现有 `.c/.cpp/.h` 后直接重新构建。头文件变化通过依赖文件触发相应应用单元重编；底层预编译对象保持不变。涉及跨应用／底层接口的结构体布局、编译宏或调用约定不能单独变更。

原应用二维码模块直接包含 LVGL 的 MIT QR 编码器源码。为避免用户构建读取底层源码树，这份实现和头文件被复制到 `src/thirdparty/qrcode/`；新副本仅调整了包含路径，原工程不变。

新增源文件后将相对 `src/` 的路径逐行加入 `app_sources.txt`。已有单元严格使用导出时参数；新增单元继承同语言、目录最接近的现有单元参数。不要把目录下所有第三方源文件批量加入，因为其中包含当前配置未启用的实现。

## 产物与刷机

- `out/rt_system.elf`、`rt_system.bin`、`rt_system.map`、`rt_system.syms`：新链接的 RTOS 和调试材料。
- `out/rtos_freertos_f101s3_uart0_480Mmmc.img`：当前固定 f101s3_astrotoo / 480Mmmc 板型的完整量产烧录包。
- `out/rtos_pkg_nor.fex`、`rtos_pkg_nor.zip`：当前原工程 RTOS 分区包格式及压缩包；名字沿用原 SDK，实际板型为 480Mmmc。
- `out/build-report.json`：SDK ID、构建耗时、重编数量、产物大小和 SHA-256。

Windows 烧录工具随工程提供。下载并解压 [PhoenixSuit 烧录工具](tools/phoenSuit.zip)，打开 PhoenixSuit 后选择上面列出的完整 `.img` 镜像，按设备现有烧录流程操作。

打包在新工程 `build/pack` 中进行：重新生成 RTOS 包，再恢复固定 U-Boot 包，最后由原全志工具生成完整 `.img`。不会简单拼接二进制或覆盖原工程烧录包。打包检查 RTOS 加载内存上限和 bootA/bootB 分区容量。

完整量产包包含固定资源分区，使用现有量产刷机流程；不要把它当成保证保留用户数据的应用增量更新包。RTOS 分区包仅能通过设备原本支持的升级流程使用。本工程没有自动烧写设备，也没有生成指向官方服务器的 OTA 描述或上传产物。硬件启动、显示、联网和恢复仍需要在开发设备实测。

> 2026-11-25 开源策略：OSS 完整只读实现仅编入 SDK .a，SRC 不带实现/KEY；百炼、LyricFind 仍禁用且不交付凭据，`Device/UploadLog` 禁用。新同步的候选和备份保存在工程外的 `AstroToo_open_maintenance/`；本次归档的旧验证材料见 `verification/private-archive-index.json`。以下历史记录不代表新策略的功能状态。

> 共享 AES 使用 SDK 中已有的私有 LIB 实现，公开 SRC 的旧 KEY/IV 副本已移除；加解密协议、接口和运行行为不变，产测及烧录恢复链路不改。

## 内部维护与发布

### 仅同步 SDK，保留当前应用源码

原工程完成底层编译和资源打包后，在本工程根目录执行：

```bash
bash sync_sdk.sh --jobs 8
# 默认读取同级 AstroToo；也可以指定构建机上的原工程路径：
bash sync_sdk.sh --source /home/qifi/esp/allwinner/AstroToo --jobs 8
# 只导出并验证候选 SDK，不替换当前 SDK：
bash sync_sdk.sh --check-only --jobs 8
# 原工程将原应用模块移入库等情况下，显式对齐编译清单，仍不修改 src/：
bash sync_sdk.sh --align-source-list --jobs 8
# 仅用于旧版未更新 SRC、缺少运行库锁补丁源码时的迁移：
bash sync_sdk.sh --align-source-list --freeze-app-source newlib_stdio_lock.c --jobs 8
```

也可以运行 `make sync-sdk JOBS=8`。命令需要 Linux/WSL 和完整镜像打包运行库，与正常构建相同。

同步包括 `sdk/`（静态库、接口头文件、配置、链接脚本、Bootloader、固定资源镜像和打包工具）、`toolchain/`、`sdk.lock`、厂商许可证及输入快照。**不会修改 `src/`；默认也不修改 `app_sources.txt`，不会在原工程中执行编译或打包。** 原工程的源文件改动必须先在原工程完成相应编译；`board/.../data` 的改动必须先完成资源打包，命令读取的是已有产物。

当前原工程已将 net、Zstandard、WebP、SQLite、音频解码器和 minilzo 移入 `astrotoo_bundle` 库。对于最初包含这些模块源码的应用清单，首次迁移需使用 `--align-source-list`：移除旧 SDK 编译、而新 SDK 已不再作为应用编译的单元，加入新 SDK 的应用单元，保留本地新增自定义单元和注释。变更明细记入同步报告，旧清单随旧 SDK 备份并参与回滚。物理源码文件继续保留，但移入库的单元不再参与应用编译；修改这些旧副本不会改变库。若新清单需要的应用文件在本地不存在，验证会失败，不自动从原工程同步源码。

可用 `--freeze-app-source 相对src的路径` 明确指定个别模块由原工程已编译的对象提供，例如本次新增的 `newlib_stdio_lock.c` 运行库补丁。导出器将对象纳入固定静态库，并保持它与应用对象的原始链接顺序，不复制源文件。模块名保存在 `sdk/config/frozen_application.json`，后续普通同步会沿用此设置；原工程不再提供该模块时会报错，需要维护者重新确定接口。它不会自动将所有缺失应用文件转为二进制。当前项目的两个 newlib stdio 锁包装链接参数也会随 SDK 导出。

本工程现已完成 SRC 更新，`newlib_stdio_lock.c` 已作为应用源码参与编译，不需要上述冻结参数；普通 SDK 同步命令仍保持 `src/` 不变。

流程是：在 `verification/sdk-sync/<运行编号>/candidate/` 导出候选 SDK，用当前应用源码的临时副本完成全量编译和打包；通过后备份旧 SDK 与 `build/out`，替换固定文件，再在正式路径重新编译、打包。这样不会沿用旧 SDK 的应用对象。当前应用与新接口不兼容时，候选验证失败并保留原 SDK。安装后构建失败会恢复旧 SDK 和旧产物；检测到并发 SDK 编辑时停止自动回滚，保留修改与备份供人工处理。

应用源码和清单在同步前后进行内容比较；发现并发修改会停止更新，不覆盖源码。构建、清理和同步使用同一进程锁，不能同时执行。锁随进程退出释放，不需要手动删除锁文件。

每次运行的日志及 `result.json`（失败时为 `failure.json`）位于对应的 `verification/sdk-sync/<运行编号>/`。旧版本在其 `backup/` 中保留，候选工程也保留供排查，每次可能占用数 GB；确认新版本正常后可自行归档清理相应运行目录。`verification/integration-results.json` 是首次导出的验证记录，新版本以本次同步报告和 `out/build-report.json` 为准。同步期间不要手工修改 SDK 或源工程的构建产物。

### 首次完整导出

`tools/export_sdk.py` 是内部维护工具，只读取一个已经构建完成的原工程，不运行原工程编译。它拒绝覆盖已有 `src/sdk/toolchain`，只适用于导出到新的空工程骨架：

```bash
python3 tools/export_sdk.py --source /path/to/AstroToo --output /path/to/new-project
```

输入必须包含完整对象、对应 `.cmd`、链接脚本、工具链以及匹配的 `out/f101s3/astrotoo/image` 打包暂存材料。当前导出器针对已验证的单 RTOS 镜像、无 XIP/PSRAM 分离镜像、无内核压缩、无 kallsyms 配置；不把不支持的配置静默降级。当前链接规则以现有 C907 RV32 配置为基准。

导出会检查读取的源码／对象在导出过程中是否变化，并留下 SHA-256 快照。原工程可以继续由其他人编辑；快照之后的变化不会自动进入这个 SDK。

## 首次导出验证记录

679 个应用编译单元已从源码编译，并与 1,247 个预编译底层对象成功链接，完整 480Mmmc 烧录包已生成。集成验证确认：

- 未修改应用时重编数量为 0；新增应用源码只重编新增单元，标记成功进入二进制，测试修改随后已清除。
- 修改固定 SDK 配置会使完整性校验失败。
- RTOS 包的校验和有效，其中 RTOS 负载与新链接的 BIN 完全一致；完整烧录包包含该 RTOS 包及固定 Bootloader。
- SDK 使用普通静态库，没有引用原工程的 thin archive；应用编译依赖不指向原源码树。
- 导出涉及的 7,838 个原工程输入文件 SHA-256 未改变。未烧写设备，硬件运行尚未验证。

详细记录见 `verification/integration-results.json`。可在没有同时编辑本工程源码时重跑集成验证；验证脚本会短暂修改一个 SDK 配置和添加一个应用探针，检查后恢复自身改动：

```bash
python3 tools/validate_project.py
```

## SDK 同步验证记录

此前已完成首次库结构迁移及一次默认 `bash sync_sdk.sh --jobs 8` 的完整同步。该次 SDK 为 `f101s3_astrotoo-20261013T075737Z`，538 个应用编译单元与更新后的预编译 SDK 完成全量编译、链接和打包。

- `src/` 内容保持不变；首次迁移将 141 个已移入库的单元移出应用清单，后续默认同步也保持清单不变。
- `newlib_stdio_lock.c` 由 SDK 对象提供，两个包装链接参数和对应符号均已核验。
- 10 项替换、回滚、并发保护、清单对齐和链接顺序测试通过。
- RTOS 包校验和、BIN 负载、完整镜像中的 Bootloader 及两个资源分区均通过内容核验；7 个 SDK 静态库均为普通 archive。
- 应用编译依赖没有指向原工程。原工程在导出后继续发生编译产物更新，差异已记录在检查报告中；本次 SDK 保留导出时的快照，后续更新在原工程编译、打包完成后再次同步。

报告见 `verification/sdk-sync/20261013T075627118113Z/result.json` 和同目录 `post-checks.json`。旧 SDK、产物及首次迁移的旧清单位于各次运行的 `backup/`。本次未烧写设备。

## SRC 与配套 SDK 更新记录

当前应用源码已按原工程的库拆分结构更新，SDK 为 `f101s3_astrotoo-20261013T081137Z`。更新前对比首次导出快照，确认本地没有独立源码修改；原工程只作为只读输入。

- 新增 4 个文件、更新 25 个文件，454 个已移入库的旧文件从应用树移除。应用树从 1,718 个文件更新为 1,268 个文件。
- net、WebP、Zstandard、SQLite、音频解码器、minilzo 等实现由 `sdk/lib/projects_fixed_1.a` 中的预编译 bundle 提供，公共头文件位于 `sdk/sysroot/rtos/projects/f101s3/astrotoo/lib/include/`。
- `newlib_stdio_lock.c` 恢复为源码编译，`sdk/config/frozen_application.json` 清单为空；应用编译清单按原工程当前配置更新为 539 个单元。
- 二维码模块继续保留独立工程需要的 include 路径适配。
- 候选目录及正式目录均已完成全量编译、链接和完整烧录包生成，尚未烧写设备。

更新报告见 `verification/src-update/20261013T081036954367Z/result.json`。旧 `src/`、编译清单、SDK 和烧录包均保存在该次运行的 `backup/` 中。

**这个目录是内部工程交付，不代表全志 SDK 已取得公开再分发许可，也不宣称只提供二进制就满足 GPL。公开推送前必须完成 `licenses/RELEASE_CHECKLIST.md`。**
