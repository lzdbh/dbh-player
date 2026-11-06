# dbhx-player · 方案 v2（重新梳理版）

| 项 | 内容 |
|---|---|
| 生成日期 | 2026-11-06 |
| 依据 | `HANDOVER.md`（交接 / 证据链）、`.workbuddy/imported-tasks/2026-11-06-cast-player/TASK.md`（需求原文）、`player-ui-mockup.html`（UI 示意图） |
| 定位 | **决策用方案主文档**：只写「做什么、为什么、什么时候必须定」。调研证据、版本核实、参考链接仍以 `HANDOVER.md` 为准，本文不重复搬运 |
| 状态 | **方案已确认**（2026-11-06 13:40：A 开源 / P0 先行）；`DESIGN.md` 工程设计待最后确认；仍未建工程、未写业务代码、未提交 git |

> 文档分工：`HANDOVER.md` 是给接手者看的档案，`PLAN.md` 是给用户拍板看的方案。后续阶段状态回写 HANDOVER 第 6/7/9 节，本文只维护决策基线。

---

## 0. 一页结论

做一个 **PC 端（macOS / Windows / Linux）IINA 式桌面播放器**：`libmpv` 原生渲染层 + Vue 透明 Overlay 分层，WebView 不解码；投屏走 **DLNA/UPnP 零安装通道**，PC 起一个支持 **HTTP 206 Range** 的本地媒体服务，把 token 化 URL 推给电视，由电视自己拉流；Rust 侧复用 `vuio-core`，协议层先抽 `CastTransport`，为日后自建 Receiver 留口子。

**这次重新梳理的三个实质变化（不是复述原文）：**

1. **授权风险重新定性**：IINA 只是视觉 / 交互参考，不复制其 Swift 源码，GPLv3 通常不构成传染；真正要拍板的是 **libmpv 的构建与链接方式**（GPL 构建 vs LGPL 构建 + 动态链接）。原方案把「IINA 是 GPLv3」列为一号风险，定位偏了。
2. **决策阻塞从 7 项收敛到 1 项**：原 7 项里，3 项属于 P3/P4 后期裁剪，2 项有安全默认值可延后，1 项本来就是 P0 的验证内容（不需要提前拍板）。只剩授权路线是硬阻塞。
3. **分期加门禁与「最晚决定时点」**，并把「投屏最小闭环」提前到 P1 末尾集成，避免投屏风险拖到最后才暴露。

---

## 1. 范围（已锁定，不再讨论）

| # | 约束 | 说明 |
|---|---|---|
| 1 | 桌面视频播放器 | 体验参考 macOS IINA |
| 2 | 投屏到安卓电视 | Android TV / 国产智能电视 / 盒子 |
| 3 | 只做 PC 端跨平台 | macOS / Windows / Linux；**不做 iOS / Android 移动端** |
| 4 | 技术栈已指定 | Rust + Tauri 2.12 + Vue 3 + Ant Design Vue + Pinia |
| 5 | TV 端原生 | 内置 DLNA 接收端为零安装主通道；自建 Receiver 若做则 Kotlin + Media3，不跟 PC 跨端 |

**明确不做**（防范围蔓延）：移动端 App、Google Cast 主通道（GMS 闭源、国内基本没有）、Miracast 镜像、P0 阶段的实时转码 / 插件系统 / 缩略图进度条 / 原生 PiP / Touch Bar / Live Text。

---

## 2. 架构：两条主链路

```
┌─ Tauri 2.12 (PC 三端) ──────────────────────────────────────┐
│  Vue 3 + Ant Design Vue + Pinia  ← 透明 Overlay，只做 UI      │
│        ⇅ Commands (UI→Rust) / Events (Rust→UI) → Pinia       │
│  Rust Core                                                  │
│    · MpvEngine        libmpv 0.41 原生 surface               │
│    · CastController   SSDP / mDNS / DLNA（vuio-core）         │
│    · MediaServer      axum，HTTP 206 Range，token 化 URL       │
│    · Store            redb / SQLite，列表 / 历史 / 断点续播     │
└───────────────┬─────────────────────────────────────────────┘
                │ 局域网：控制指令 / 状态回传 / 媒体流
┌───────────────▼─────────────────────────────────────────────┐
│  Android TV：内置 DLNA 接收端（零安装）                        │
│             可选 P3：Kotlin + Media3 + Compose for TV         │
└─────────────────────────────────────────────────────────────┘
```

**链路 A · 本地播放**：libmpv 输出原生 video surface → Tauri 透明窗口 → Vue Overlay 悬浮控制条。硬前提是透明窗口；Tauri 2.12.1+ 在 macOS 已免 `macos-private-api`，门槛降低。

**链路 B · 投屏会话**：发现（SSDP M-SEARCH / mDNS）→ 取设备描述与 SCPD → `SetAVTransportURI(http://PC:port/media/id?token=…)` → 电视 `HTTP GET Range` → PC 回 `206 Partial Content` → SOAP Play/Pause/Seek → `GetPositionInfo` 约 1s 轮询取进度与状态。

**三条命门**：

| 机制 | 若缺失的后果 | 落地方式 |
|---|---|---|
| 透明窗口 | 视频层无法与 UI 叠加，架构不成立 | `transparent: true`，P0 三平台各验一次 |
| HTTP 206 Range | 进度条能显示但一拖就断 | `vuio-core` 已内置；`Accept-Ranges` / `Content-Range` / MIME 必须正确 |
| `CastTransport` 抽象 | 日后接自建 Receiver 要返工 | `discover` / `load` / `control` / `stateStream`；P2 只落 DLNA 实现 |

示意图 `player-ui-mockup.html` 已把这两条链路的产品形态表达清楚：三态投屏（本地 / 发现设备 / 投屏中）、手动 IP 兜底、投屏后 PC 转为遥控器。

---

## 3. 选型定稿（不变，补「为什么」）

| 层 | 选型 | 为什么是它 |
|---|---|---|
| UI | Vue 3 + Ant Design Vue + Pinia | 用户指定；Overlay 只画控件，暗色 token 对齐 IINA |
| 应用壳 | Tauri 2.12.x | 用户指定；Commands/Events 与 Pinia 天然对接，2.12.1+ 解决 macOS 透明窗口 |
| 播放内核 | libmpv 0.41 原生 surface | 系统 WebView 覆盖不了 MKV / HEVC / AV1 / 4K HDR / FLAC；IINA 本身就是这个架构 |
| Rust core | tokio + axum + `vuio-core` + redb/SQLite | Apache-2.0 的 `vuio-core` 自带 SSDP、mDNS、ContentDirectory、Range 流式、renderer 控制，砍掉大半投屏工作量 |
| 媒体服务 | axum HTTP 206 Range | 本地文件 / 外挂字幕 / token 化 URL，防局域网嗅探 |
| TV 端 | 内置 DLNA 接收端 → 可选 Kotlin + Media3 1.11.1 + Compose for TV | 遥控器焦点、10-foot UX、MediaSession、Widevine 必须原生 |

---

## 4. 三平台差异 → P0 必验清单

| 项 | macOS | Windows | Linux |
|---|---|---|---|
| WebView | WKWebView | WebView2 | WebKitGTK 4.1 |
| libmpv 渲染 | NSOpenGLView / Metal；Homebrew 的 mpv 为 Vulkan-only，**嵌入需源码编译**（mpv 0.40/0.41 与 ffmpeg 8 不兼容，需 ffmpeg@7） | D3D11 / 原生 HWND；可用预编译 `mpv-dev-lgpl` | EGL + X11 子窗口 / Wayland subsurface |
| 透明窗口 | 2.12.1+ 免 private API ✅ | `transparent: true` | 依赖 compositor |
| 动态库打包 | `dylibbundler` + 签名公证 | DLL 随 `resources: lib/**/*` + MSVC 运行库 | AppImage `patchelf` / 系统 `libmpv-dev` |
| 系统门槛 | macOS 12+ | Win10+（2.12 起弃 Win7） | 视发行版 |

**P0 Go / No-Go 判定项**（必须全部为「是」才进入 P1）：

1. 三平台各自能否跑通「Tauri 2.12 + libmpv 原生 surface + 透明 Overlay 可点按」？
2. 真机 Android TV 能否发现 PC 并拉取 `206` 本地流、正常播放？
3. 拖动进度（Seek）后电视能否从新位置续播？
4. 安装包能否在干净机器（无预装 mpv / ffmpeg）上启动？

---

## 5. 分期与门禁（重排）

| 阶段 | 目标 | 验收 / 门禁 | 最晚决定时点 |
|---|---|---|---|
| **P0 Spike** | 双链路打通：① 三平台透明窗口 + libmpv + Overlay；② 真机 DLNA 推本地 206 URL 可播可拖 | 第 4 节 4 项全通过；任一不通则调整架构而非硬做 | 开始前：授权路线（仅影响用哪个构建） |
| **P1 播放器 MVP** | IINA 核心子集：播放列表、字幕/音轨、倍速/逐帧、快捷键、历史、深色主题 | 桌面三端可日常使用 | — |
| **P1.5 投屏最小闭环（新增）** | 一个「投屏」按钮 + 一台真机：发现 → 推 URL → 播放 → 暂停 → 拖动 | 零安装投本地文件成功；暴露 UI 状态机与 Rust 层的集成问题 | 自建 Receiver 路线最晚在此定 |
| **P2 投屏 MVP** | 完整发现、播控、状态轮询、遥控器面板、手动 IP 兜底、设备兼容表 | 至少 2 个不同品牌电视可用 | TV 端能否装 APK 最晚在此定 |
| **P3 自建 Receiver（可选）** | Kotlin + Media3 + Compose for TV：双向状态、字幕、PIN 配对 | 取决于是否产品化 | — |
| **P4 深化** | 插件系统、缩略图、在线字幕、实时转码、PiP | 按优先级裁剪 | 特性范围 / 转码 / 缩略图最晚在此定 |

**与原分期的差别**：原方案 P1（播放器）→ P2（投屏）顺序不变，但插入 P1.5 把投屏最小闭环提前集成。理由：投屏是本项目的差异化价值，也是 DLNA 厂商碎片化风险的来源，拖到 P2 才发现集成问题代价最大；而 P0 已经把底层链路验完，P1.5 只是接线，增量很小。

---

## 6. 决策重构：7 项 → 1 项必须现在定

| # | 原决策 | 新归类 | 默认值 | 最晚决定时点 | 变更代价 |
|---|---|---|---|---|---|
| 1 | 开源 or 闭源商用 | **🔴 唯一硬阻塞** | 见 6.1，建议 B | **P0 开始前** | 高且部分不可逆：选 GPL 后再转闭源须替换内核 / 重做构建链 |
| 2 | 投屏主通道（纯 DLNA / 自建 / 双通道） | 🟡 可延后 | 双通道设计，**但只落 DLNA**；`CastTransport` 保抽象 | P1.5 前 | 低，抽象层已隔离 |
| 3 | TV 端能否装 APK | 🟡 可延后 | 按「不能」设计（零安装优先） | P2 前 | 低，只影响 P3 是否立项 |
| 4 | 媒体源（本地 / 网络流 / 两者） | 🟡 可延后 | 两者，P0/P1 先落本地文件 | P2 前 | 低，MediaServer 路由预留即可 |
| 5 | IINA 特性范围（全量 / 核心子集） | 🟢 后期裁剪 | 核心子集（P1 那批），其余全部推 P4 | P4 前 | 无，缩略图 / PiP 会引入 ffmpeg 与平台原生代码，本就该晚做 |
| 6 | 是否要实时转码 | 🟢 后期裁剪 | 不要，P4 再评估 | P4 前 | 无，叠加 ffmpeg 依赖与授权复杂度 |
| 7 | libmpv 接入（现成插件 / 自研 plugin） | 🔵 属 P0 验证内容 | 先试 `tauri-plugin-libmpv`，不通再自研 | P0 出结论 | — 这本来就是 Spike 要回答的问题，不该让用户提前猜 |

### 6.1 授权路线 · 已确认（2026-11-06 13:40）

**结论：选 A —— 直接用 GPL 构建，项目开源。** 授权核对不再是 P0 前置条件。

| 路线 | 做法 | 判断 |
|---|---|---|
| **A. GPL 开源（已选）** | 直接用各平台默认 / 预编译 libmpv，项目按 GPL 开源 | 构建链最省事；后续若转闭源需替换内核与依赖链，基本不可逆 |
| B. 闭源兼容起步 | LGPL 构建 + 动态链接，三平台自控构建 | 未选；若日后商业化须重新评估 |
| C. 换内核 | 改用可商用内核 | 未选；会推翻架构 |

许可证版本定为 **`GPL-3.0-or-later`**：mpv 为 GPLv2-or-later 可升 v3，而 `vuio-core`（Apache-2.0）、Tauri（MIT/Apache-2.0）与 GPLv3 兼容、与 GPLv2-only 不兼容，选 v3+ 可让整条依赖链一次合规。**仍不复制** IINA 源码、图标与素材。开源义务的 6 条清单动作见 `DESIGN.md` §1。

### 6.2 其余决策已按默认值生效

- 投屏：双通道设计、P2 只落 DLNA，保留 `CastTransport`；按「TV 不能装 APK」设计。
- 媒体源：本地文件优先，网络流在 MediaServer 预留路由。
- 特性：IINA 核心子集；不做实时转码、缩略图、插件、PiP（推 P4）。
- libmpv 接入方式由 P0 Spike 给出结论（先试现成插件，不通再自研）。

---

## 7. 风险（精简）

| 风险 | 等级 | 缓解 |
|---|---|---|
| P0 双链路任一不通 | 高 | 最先做，设 Go / No-Go 门禁；不通就调架构，不硬做 |
| libmpv 授权与构建 | 高 → **中（已解除阻塞）** | 路线 A 已定，转为 6 条开源合规清单动作 |
| DLNA 厂商碎片化 | 中 | SOAP 差异做设备兼容表；SSDP 被防火墙 / AP 隔离拦时提供手动 IP 兜底 |
| 三平台打包分发 | 中 | dylib / DLL / so 随 bundle；macOS 签名公证、Windows SmartScreen、Linux AppImage |
| 运行时稳定性 | 中 | URL token 化；PC 睡眠 / 换网重连与断点续播；投屏期间媒体服务常驻 |

---

## 8. 确认记录与下一步

| 时间 | 事项 | 结论 |
|---|---|---|
| 2026-11-06 11:27 | 交接文档 `HANDOVER.md` 生成 | 7 项决策列为阻塞点 |
| 2026-11-06 11:35 | 方案 v2 `PLAN.md` | 决策收敛为 1 项硬阻塞 |
| **2026-11-06 13:40** | **用户书面拍板** | **A GPL 开源 + P0 先行；其余 5 项按默认值** |

**当前唯一待确认**：`DESIGN.md`（工程结构与接口设计 —— crate 划分、Commands/Events 契约、`CastTransport` trait、Pinia store 划分、MediaServer 路由、P0 的 S1~S5 分解）。按工作约定，确认后才建工程、写业务代码、执行 P0 Spike。

**未做的准备动作**（未经确认不擅自推进）：补 `.gitignore`（含 `.workbuddy/`）、把示意图与文档作为首次提交。仓库目前仍是 `No commits yet`。

---

## 9. 约定重申

先方案 / 对比表 → **书面确认** → 才实施；即使此前确认过的方案，重新生效前也需再次确认。本文为方案，**确认前不建工程、不写业务代码、不提交 git**。
