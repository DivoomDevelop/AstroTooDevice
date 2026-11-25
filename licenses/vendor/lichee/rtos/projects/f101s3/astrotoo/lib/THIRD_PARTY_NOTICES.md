# 第三方组件声明

本清单记录当前源码携带的声明，并不将第三方代码重新许可为 Divoom 私有代码。实际编入的 AAC 后端和 SQLite 开关见 `prebuilt/manifest.json`。

| 组件 | 本地声明与材料 |
| --- | --- |
| Zstandard 1.5.6 | 源码声明 BSD/GPL 双许可；保留 BSD 文本于 `licenses/zstd-LICENSE.txt`，来源为对应上游版本。 |
| WebP | 源码声明 BSD 风格许可；`licenses/webp-COPYING.txt` 保存上游 BSD 文本，不据此推断本地库版本。 |
| SQLite | 原始版权放弃声明保存在 `licenses/sqlite-NOTICE.txt`。 |
| minimp3 | 原始公共领域/CC0 声明保存在 `licenses/minimp3-NOTICE.txt`；导出头文件亦保留声明。默认不编入（`CONFIG_DIVOOM_MP3_MINIMP3` 时启用）；Helix Layer I/II 参照其码表与解析流程重写。 |
| FAAD2 | 原始 `COPYING` 全文保存在 `licenses/faad2-COPYING.txt`，包括其商业授权说明；是否编入由配置决定。 |
| minilzo | 原始源文件声明保存在 `licenses/minilzo-NOTICE.txt`，其引用的 GPL v2 文本保存在 `licenses/GPL-2.0.txt`。 |
| Helix AAC / Helix MP3 | 原始 RPSL/RCSL 声明保存在 `licenses/helix-NOTICE.txt`；本地改动见 `src/thirdparty/helix_*/ORIGIN.txt`；需根据已有授权确认适用许可及发布材料。 |

上游文本来源：

- [Zstandard v1.5.6 LICENSE](https://github.com/facebook/zstd/blob/v1.5.6/LICENSE)
- [libwebp COPYING](https://github.com/webmproject/libwebp/blob/v1.3.2/COPYING)
- [minimp3](https://github.com/lieff/minimp3)

打包命令保留这些声明，并去除实现源码和调试信息。正式发布前，需要针对实际启用的第三方组件确认授权以及对应源码/修改记录的交付方式；本二进制包不包含对应源码材料。
