# dbhx-player · 工程结构与接口设计（v1）

| 项 | 内容 |
|---|---|
| 生成日期 | 2026-11-06 13:40 |
| 上游 | `PLAN.md`（方案 v2）、`HANDOVER.md`（证据链） |
| 状态 | **决策已确认，设计待确认**；确认前不建工程、不写业务代码 |
| 本文定位 | 实施前最后一份设计文档：目录结构、IPC 契约、`CastTransport`、MediaServer 路由、P0 任务分解 |

---

## 0. 决策基线（用户已书面确认）

| 决策 | 结论 | 对设计的直接影响 |
|---|---|---|
| 授权路线 | **A · 直接用 GPL 构建、项目开源** | 不再要求 LGPL 构建 + 动态链接；可用各平台默认/预编译 libmpv，三平台构建链大幅简化 |
| P0 先行 | **允许** | 授权技术核对不再是 P0 前置条件，两条最高风险链路可立即验证 |
| 其余 5 项 | 按 `PLAN.md` §6.2 默认值 | 只落 DLNA 但保留 `CastTransport`；按 TV 不能装 APK 设计；本地文件优先、网络流预留；IINA 核心子集；不做实时转码 |
| libmpv 接入 | 由 P0 回答 | 先试 `tauri-plugin-libmpv`，不通再自研薄封装 |

**许可证版本建议：`GPL-3.0-or-later`**。理由：mpv 是 GPLv2-or-later，可升到 GPLv3；而 `vuio-core`（Apache-2.0）、Tauri（MIT/Apache-2.0 双许可）与 **GPLv3 兼容、与 GPLv2-only 不兼容**。选 GPLv3+ 可让整条依赖链一次性合规，不必逐个排除 Apache-2.0 组件。

---

## 1. 选 A 之后新增的合规动作（P0 顺手做掉）

| # | 动作 | 落点 |
|---|---|---|
| 1 | 根目录 `LICENSE`（GPL-3.0-or-later）+ `README` 声明 | 首次 commit 时 |
| 2 | `THIRD_PARTY_NOTICES.md`：依赖与许可证清单，用 `cargo license` / `cargo-deny` 生成 | 首次打包前 |
| 3 | 随二进制提供「对应源码」获取方式：仓库 URL + 固定 commit / tag | 打包脚本 |
| 4 | 三平台 libmpv 构建来源与版本写进 `docs/BUILD.md` | P0 结束 |
| 5 | 核查 `tauri-plugin-libmpv` / `libmpv-wrapper` 自身许可 | P0 S2 |
| 6 | **不复制** IINA 源码、图标、素材；只参考交互与视觉 | 全程 |

> 开源路线下，原本的「授权核查」从阻塞项降级为 6 条清单动作，其中 2 条在 P0 内即可完成。

---

## 2. 仓库与工程结构

```
dbhx-player/
├─ src/                          # Vue 3 + Vite
│  ├─ app/bridge.ts              # 唯一 IPC 与事件订阅入口
│  ├─ stores/                    # Pinia
│  ├─ features/player/           # OSC 控制条、轨道菜单、快捷键
│  ├─ features/cast/             # 设备列表、三态面板、手动 IP、遥控器
│  ├─ features/playlist/
│  └─ theme/                     # 暗色 token，对齐示意图
├─ src-tauri/                    # Rust（Tauri 2.12）
│  ├─ src/
│  │  ├─ lib.rs / main.rs
│  │  ├─ commands/               # IPC 命令（薄适配层）
│  │  ├─ mpv/                    # MpvEngine：surface + property
│  │  ├─ cast/                   # transport.rs (trait) + dlna.rs
│  │  ├─ media/                  # axum MediaServer
│  │  ├─ store/                  # redb：列表 / 历史 / 断点续播
│  │  ├─ platform/               # #[cfg] 三平台 surface 与打包
│  │  └─ error.rs                # AppError → {code, message, detail}
│  ├─ capabilities/default.json
│  ├─ tauri.conf.json            # transparent: true, resources: lib/**/*
│  └─ Cargo.toml
├─ spike/                        # P0 验证代码，可弃；验证通过再搬可复用部分
├─ docs/BUILD.md  THIRD_PARTY_NOTICES.md  LICENSE
├─ PLAN.md  HANDOVER.md  DESIGN.md  player-ui-mockup.html
└─ .gitignore                    # 含 .workbuddy/、target/、node_modules/、dist/
```

**TV 端工程 P0~P2 不建目录**：`CastTransport` 已隔离协议层，P3 立项时再以 `tv-receiver/` 加入同一 monorepo，不会返工；提前建空目录只会造成"看起来做了"的错觉。

### Rust crate 划分：P0 用单 crate 多 module

| 方案 | 优点 | 代价 | 判断 |
|---|---|---|---|
| **单 crate + 清晰 module（推荐）** | DTO / `AppError` 直接共享，编译快，P0 迭代最省事 | 后期模块边界靠约定维持 | P0/P1 用这个 |
| workspace 多 crate（core / cast / media） | 边界强制、可独立测试 | 跨 crate 要抽公共 DTO crate，P0 阶段纯增成本 | 规模上来再拆，拆时按现有 module 边界切即可 |

---

## 3. Tauri IPC 契约

**原则**：Commands 只收可序列化 DTO，不把 libmpv 句柄 / surface 指针暴露给 WebView；UI 侧不直接读 mpv property，全部经 Events 推送。

### Commands（UI → Rust）

| 命令 | 入参 | 返回 | 说明 |
|---|---|---|---|
| `player_open` | `{ source: MediaSource }` | `MediaId` | 本地路径或 URL，路径由 Rust 侧校验并登记映射 |
| `player_control` | `{ cmd: PlayerCommand }` | `()` | `Play/Pause/Seek{pos}/SetSpeed/SetTrack{kind,id}/FrameStep/Snapshot` |
| `player_props` | `()` | `PlayerSnapshot` | 首次订阅时拉全量，之后走 Events |
| `playlist_list` / `playlist_add` / `playlist_remove` | DTO | 列表 | 历史与断点续播同 store |
| `cast_discover` | `{ timeout_ms }` | `()` | 结果走 `cast://devices` 事件，避免长轮询 |
| `cast_add_manual` | `{ ip, port? }` | `Device` | 组播被拦时的兜底 |
| `cast_connect` / `cast_disconnect` | `{ device_id }` | `SessionId` / `()` | 会话与 token 绑定 |
| `cast_load` | `{ session_id, media_id }` | `{ url, token }` | 内部向 MediaServer 登记映射 |
| `cast_control` | `{ session_id, cmd }` | `()` | Play/Pause/Seek/Stop |
| `config_get` / `config_set` | `{ key, value? }` | DTO | mpv.conf / 快捷键 / 主题透传 |

### Events（Rust → UI）

统一 envelope：`{ seq, ts, kind, payload }`（`seq` 用于 UI 去重与乱序丢弃，Pinia 只认更大的 seq）。

| 事件 | payload | 频率 |
|---|---|---|
| `player://state` | `{ position, duration, paused, speed, tracks, buffering }` | mpv property 变化 / 250ms 节流 |
| `player://error` | `AppError` | 按需 |
| `cast://devices` | `Vec<Device>` | 发现期间增量推送 |
| `cast://state` | `{ phase, device, position, duration, error }` | DLNA `GetPositionInfo` 约 1s 轮询 |
| `media://access` | `{ media_id, range, status }` | 调试开关，默认关闭 |

`AppError = { code: enum, message, detail? }`；UI 只按 `code` 分支，不解析 message 文本。

---

## 4. Pinia 与 bridge（防「忘记订阅」）

| store | 职责 |
|---|---|
| `usePlayerStore` | 播放状态、轨道、倍速、OSC 自动隐藏 |
| `usePlaylistStore` | 列表、历史、断点续播、音乐模式 |
| `useCastStore` | 三态（本地 / 发现中 / 投屏中）、设备表、手动 IP、遥控器指令 |
| `useSettingsStore` | 暗色主题、mpv.conf 透传、快捷键映射 |
| `useUiStore` | Overlay 可见性、侧栏折叠、全屏 |

**`src/app/bridge.ts` 是唯一入口**：应用启动时一次性注册全部 Events 订阅 + 恢复 `player_props` 全量快照，再挂载到各 store。store 内不写 `listen()`，避免新增事件时漏订阅。这个"自描述入口"比外部配置文件更不容易遗忘。

---

## 5. `CastTransport` 抽象

```rust
use futures::stream::BoxStream;

#[async_trait]
pub trait CastTransport: Send + Sync {
    async fn discover(&self) -> Result<Vec<Device>, AppError>;
    async fn connect(&self, device: &Device) -> Result<Session, AppError>;
    async fn load(&self, session: &Session, item: &MediaItem) -> Result<(), AppError>;
    async fn control(&self, session: &Session, cmd: CastCommand) -> Result<(), AppError>;
    fn state_stream(&self, session: &Session) -> BoxStream<'static, CastState>;
    async fn disconnect(&self, session: &Session) -> Result<(), AppError>;
}
```

- `state_stream` 返回 `BoxStream` 而不是 `impl Stream`：保持 trait **dyn-compatible**，将来可 `Box<dyn CastTransport>` 在实现间运行时切换；Rust 1.90 的 RPITIT 虽然可用，但会让 trait 失去 dyn 兼容。
- P2 只落 `DlnaTransport`（适配 `vuio-core`）；P3 若立项再加 `ReceiverTransport`（NSD + WebSocket），上层 `CastController` 不改。
- DTO：`Device{id,name,ip,port,manufacturer,capabilities}`、`MediaItem{media_id,url,mime,duration,subtitles}`、`CastState{phase,position,duration,error}`。

---

## 6. MediaServer 路由与安全

| 路由 | 方法 | 行为 |
|---|---|---|
| `GET /media/:id?token=…` | GET / HEAD | `Accept-Ranges: bytes`；单段 Range → `206` + `Content-Range`；越界 → `416`；无 Range → `200` 全量 |
| `GET /media/:id/sub/:sid?token=…` | GET | 外挂字幕，SRT → `text/plain`、VTT → `text/vtt` |
| `GET /health?token=…` | GET | 连通性自检，供防火墙排查 |

**四条硬规则**：

1. `:id` 是 store 里的 **映射 ID**，绝不接受客户端传原始文件路径 → 杜绝路径穿越。
2. token 随 `cast_load` 生成，随机 128bit，**绑定 media_id + 会话 + 时效**，会话断开即失效。
3. 服务绑定局域网可达地址，但只接受来自已配对设备 IP 的请求；不对外网暴露。
4. MIME 按扩展名映射（MKV `video/x-matroska`、MP4 `video/mp4`、TS `video/mp2t`），厂商电视对错误 MIME 极敏感；只支持单段 Range，不做 multipart。

---

## 7. 三平台差异落地

| 平台 | surface | libmpv 来源 | 打包 |
|---|---|---|---|
| macOS | `NSOpenGLView` / Metal + 透明窗口（2.12.1+ 免 private API） | **源码编译**：Homebrew 版为 Vulkan-only，mpv 0.41 需配 ffmpeg@7 | `dylibbundler`、rpath、签名公证（P1） |
| Windows | D3D11 + 原生 HWND | 官方 dev 预编译包（GPL 构建即可） | DLL 随 `resources: lib/**/*` + MSVC 运行库 |
| Linux | EGL + X11 子窗口 / Wayland subsurface | 系统 `libmpv-dev` 或随包 so | AppImage + `patchelf`（P1） |

`platform/` 用 `#[cfg(target_os = …)]` 收敛差异，上层 `MpvEngine` 只见统一 trait。

**现实约束**：当前开发机是 Windows，S1 只能在本机完成 Windows 侧。macOS / Linux 默认走 CI 构建矩阵（编译 + 单元验证），透明窗口与画面叠加的视觉验收仍需真机手动确认；若你手上有 Mac / Linux 机器，可并行跑。

---

## 8. P0 Spike 任务分解与验收

| # | 任务 | 验收方式 | 对应 Go/No-Go |
|---|---|---|---|
| **S1** | Tauri 2.12 透明窗口 + libmpv 输出测试画面 + Vue Overlay 可点按（Windows 先行，CI 覆盖另两端构建） | 画面可见、Overlay 点击有响应、窗口透明无黑底 | 判定 1 |
| **S2** | `MpvEngine` 最小封装：open / play / pause / seek + property → `player://state` 回传 | 拖动、倍速、轨道切换在 UI 上同步 | — |
| **S3** | axum MediaServer：本地文件 206 Range + token + MIME | `curl -H "Range: bytes=100-"` 得 206 与正确 `Content-Range`；越界得 416 | 判定 3 的前置 |
| **S4** | 真机 Android TV：SSDP 发现 → `SetAVTransportURI` → 播放 → Seek 后续播 | 零安装播出本地文件，拖进度不中断 | 判定 2、3 |
| **S5** | 干净机器启动检查（无预装 mpv / ffmpeg） | 拷安装包到干净环境能启动 | 判定 4 |

**Spike 代码放 `spike/`，声明为可弃**；验证通过后只把 `MpvEngine` / `MediaServer` / `DlnaTransport` 三个可复用核心搬进 `src-tauri`，UI 骨架重写为正式 `src/`。

**P0 明确不做**：正式 UI 全套、Pinia 全量 store、播放列表持久化、实时转码、插件、缩略图、PiP、安装包签名公证、TV 端工程。这些留到 P1/P2/P4。

**环境前置**：Rust ≥ 1.90（Tauri 2.12 MSRV）、Node、Tauri CLI、各平台 libmpv 动态库、一台同网段 Android TV。

---

## 9. 最后一道确认

按约定，设计确认后才执行 P0。给你三个选项：

| 选项 | 内容 | 适用 |
|---|---|---|
| **A · 按本文全量执行 P0（推荐）** | S1→S5 顺序推进，S1 先出 Windows 结果再决定 CI 矩阵 | 想一次验完两条链路 |
| B · 先只跑 S1 + S2 | 先验播放链路，投屏链路延后 | 想先确认 libmpv 透明窗口这个最大风险点 |
| C · 调整设计 | 指出要改的章节，我改完再确认 | 目录结构 / 契约 / 路由有不同意见 |

> 也可以直接回「按此开工」= A。执行时按 S1→S5 小步推进，每步给结果，不做未经确认的不可逆操作；`.gitignore` 与首次 commit 会作为 P0 的第一步一并处理（含把 `.workbuddy/` 排除）。
