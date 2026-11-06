# dbhx-player

PC 端跨平台（macOS / Windows / Linux）视频播放器，参考 macOS IINA 的体验，支持投屏到安卓电视。

> 当前阶段：P0 Spike 进行中。正式工程尚未建立，`spike/` 下为可弃的验证代码；进度见 `HANDOVER.md` §11。

## 文档

| 文件 | 内容 |
|---|---|
| `PLAN.md` | 方案 v2：范围、架构、分期、决策基线 |
| `DESIGN.md` | 工程结构与接口设计：Commands / Events、`CastTransport`、MediaServer、P0 分解 |
| `HANDOVER.md` | 交接文档：调研证据链、参考链接、仓库现状、工作约定 |
| `player-ui-mockup.html` | 可交互 UI 示意图，浏览器直接打开即可 |
| `docs/BUILD.md` | 三平台构建前置与 libmpv 来源 |
| `THIRD_PARTY_NOTICES.md` | 第三方许可声明与 GPL 合规清单 |

## 技术栈

Rust + Tauri 2.12 + Vue 3 + Ant Design Vue + Pinia；播放内核 libmpv 0.41 原生 surface，WebView 只做透明 Overlay（不解码）；投屏走 DLNA/UPnP，PC 侧提供 HTTP 206 Range 媒体服务，Rust 侧复用 `vuio-core`。

投屏推的是播放 URL 而非画面：电视自行拉流，因此本地媒体服务必须正确实现 `Range` / `206` / `Content-Range`，否则进度条一拖就断。

## 许可证

**GPL-3.0-or-later**，详见 `LICENSE` 与 `THIRD_PARTY_NOTICES.md`。二进制分发时随附「对应源码」获取方式（本仓库 URL + 固定 commit / tag）。

本项目**仅参考 IINA 的视觉与交互**，不含其源码、图标或素材；IINA 是独立的 GPLv3 项目，与本仓库无代码关联。

## 状态

P0 Spike 正在验证两条最高风险链路：① 三平台 libmpv 透明窗口 + Overlay；② 真机 DLNA 推本地 206 流。验收标准见 `PLAN.md` §4 与 `DESIGN.md` §8。
