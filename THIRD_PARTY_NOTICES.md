# 第三方许可声明

本项目以 **GPL-3.0-or-later** 开源。根目录 `LICENSE` 为许可证全文。下表是截至 2026-11-06 的已知依赖许可，**最终清单在首次打包前用工具重新生成**，避免遗漏。

## 清单生成方式

| 生态 | 命令 |
|---|---|
| Rust | `cargo install cargo-license && cargo license --json > target/licenses-rust.json` |
| npm | `npx license-checker --json > target/licenses-node.json` |
| 动态库 | 三平台 libmpv / ffmpeg 构建来源、版本与许可记录在 `docs/BUILD.md` |

> 生成结果不入库，只把汇总写回本表。打包产物必须附带 `LICENSE`、本文件与源码获取方式。

## 已知依赖

| 组件 | 许可 | 用途 |
|---|---|---|
| Tauri 2.12 | MIT / Apache-2.0 双许可 | 应用壳 |
| Vue 3 | MIT | UI |
| Ant Design Vue | MIT | 组件库 |
| Pinia | MIT | 状态 |
| tokio | MIT | 异步运行时 |
| axum | MIT | 本地媒体服务 |
| `vuio-core` | Apache-2.0 | SSDP / mDNS / DLNA / Range 流式 |
| redb | MIT | 本地存储 |
| libmpv / mpv 0.41 | GPL-2.0-or-later（LGPL 构建可选） | 播放内核 |
| ffmpeg（随构建） | LGPL-2.1+ 或 GPL-2.0+ | 解码依赖，取决于启用的组件 |
| `tauri-plugin-libmpv` / `libmpv-wrapper` | **待核查（P0 S2）** | libmpv 接入层 |

> 许可证版本选择理由：mpv 为 GPLv2-or-later 可升到 v3；`vuio-core`（Apache-2.0）与 Tauri（MIT/Apache-2.0）**与 GPLv3 兼容、与 GPLv2-only 不兼容**。定为 GPL-3.0-or-later 可让整条依赖链一次合规。

## GPL 合规清单

- [x] 根目录 `LICENSE`（GPL-3.0-or-later 全文）
- [x] `README.md` 中的许可证声明与 IINA 关系说明
- [x] 本文件（依赖许可基线 + 生成方式）
- [x] `docs/BUILD.md`（构建前置与三平台 libmpv 来源）
- [ ] 打包产物附带源码获取方式（仓库 URL + 固定 commit / tag）
- [ ] 首次打包前刷新完整依赖许可清单
- [ ] 记录实际采用的 libmpv 二进制版本与构建来源
- [ ] 核查 `tauri-plugin-libmpv` / `libmpv-wrapper` 自身许可（P0 S2）
- [ ] 不复制 IINA 源码、图标、素材（全程约束）
