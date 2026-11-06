# P0 Spike 验收报告

| 项 | 内容 |
|---|---|
| 报告时间 | 2026-11-06 14:05 |
| 方案基线 | `PLAN.md`（v2）、`DESIGN.md`（v1）、决策 A（GPL 开源）+ P0 先行 |
| 远端 | `https://github.com/lzdbh/dbh-player.git` |
| 结论 | **协议链路成立，P0 未全过**：S3 与模拟投屏闭环实测通过；真机 DLNA、原生 surface 画面叠加、干净机器启动仍未验收，Go/No-Go 不能判「全通过」 |

---

## 1. Go/No-Go 判定（`PLAN.md` §4）

| # | 判定项 | 结论 | 证据 / 缺口 |
|---|---|---|---|
| 1 | 三平台各自跑通 Tauri 2.12 + libmpv 原生 surface + 透明 Overlay 可点按 | ⚠️ **部分** | 工程骨架已建；本机 Windows GNU 链编译耗时过长，三平台编译交 CI 判定；libmpv DLL 探针交 Windows runner；**GUI 画面叠加与点按仍需人工看** |
| 2 | 真机 Android TV 发现 PC 并拉 206 本地流、正常播放 | ⚠️ **模拟通过 / 真机未验** | 模拟 Renderer 发现 → 推 URL → 拉 206 全通过；**SSDP 多播与真实电视未测** |
| 3 | 拖动进度后电视从新位置续播 | ✅ **协议层通过** | 模拟 Renderer Seek 30s 后从 `bytes=262144-262243` 重新拉到 206；真机待验 |
| 4 | 安装包能在干净机器（无预装 mpv / ffmpeg）启动 | ❌ **未验证** | 尚未打包分发 |

> **架构判断**：投屏侧「推 URL + 206 Range」这条命门已被端到端证明成立；播放侧「原生 surface + 透明 Overlay」仍是最大未验证风险。P0 不具备整体放行条件，但足以继续做 P1 的工程化准备。

---

## 2. 已实测通过（本机，可复现）

### 2.1 S3 · HTTP 206 Range 媒体服务

`spike/media-server/`（axum 0.8.9 + tokio 1.54.1），编译通过，512 KB 样本：

| 用例 | 结果 |
|---|---|
| `Range: bytes=100-199` | `206` + `content-range: bytes 100-199/524288` + `content-length: 100` + `accept-ranges: bytes`，body 实测 100 字节 |
| 后缀 `bytes=-10` | `206` + `bytes 524278-524287/524288` + `content-length: 10` |
| 越界 `bytes=99999999-` | `416` + `bytes */524288` |
| 无 Range | `200` + `content-length: 524288` + `accept-ranges: bytes` |
| 错 token / 未知 id / health | `403` / `404` / `200 ok` |

实现约束已落地：`:id` 为 store 映射 ID（不接受原始路径）、token 绑定会话、单段 Range、MIME 按扩展名映射。

### 2.2 模拟 DLNA 投屏闭环（`spike/cast-loop/`）

最小 UPnP AVTransport MediaRenderer 代替电视，**真的向 PC 媒体服务发 Range GET**：

```
discover description.xml + controlURL   PASS
fetch SCPD action list                  PASS
SetAVTransportURI accepted              PASS
Play -> TV GET Range returns 206        PASS
Play -> Content-Range present           PASS
GetPositionInfo returns RelTime         PASS
Seek -> new offset 206                  PASS
Seek -> new Range differs from initial PASS
Pause state                             PASS
Renderer action sequence                PASS
=== 10/10 passed ===

pull @ 0s   bytes=0-99          -> 206 bytes 0-99/524288        (100 bytes)
pull @ 30s  bytes=262144-262243 -> 206 bytes 262144-262243/524288 (100 bytes)
```

**明确不覆盖**：SSDP 多播发现（本机 UDP 1900 被 Windows SSDP 服务占用，走已知地址 = 手动 IP 兜底）、厂商 SOAP 差异、遥控器 / 10-foot UX、解码与 DRM。因此这**不等于 S4 真机验收**。

### 2.3 仓库与合规

| 项 | 状态 |
|---|---|
| `.gitignore` | 排除 `.workbuddy/`、`**/target/`、`node_modules/`、`spike/fixtures/`、DLL / 7z 二进制 |
| `LICENSE` | GPLv3 全文 674 行；项目许可证 `GPL-3.0-or-later` |
| `README.md` / `THIRD_PARTY_NOTICES.md` | 许可声明与依赖基线、合规清单 |
| `docs/BUILD.md` | 三平台 libmpv 来源、环境坑、实测记录、CI 说明 |

---

## 3. 交由 GitHub Actions 判定

workflow：`.github/workflows/p0-spike.yml`，远端 `lzdbh/dbh-player`，触发 `push master/main`、PR、`workflow_dispatch`。

| Job | 平台 | 判定内容 |
|---|---|---|
| `s3-media-server` | ubuntu / macos / windows | `cargo build --locked`；Linux 复验 206 / 416 |
| `s1-tauri-overlay` | ubuntu / macos / windows | WebKitGTK 4.1 依赖 → `npm ci` → `tauri build --no-bundle` |
| `cast-loop` | ubuntu | 模拟 DLNA 闭环 10 项 |
| `libmpv-probe` | windows | 从 GitHub releases 下载 `mpv-dev-x86_64-20261106-*.7z`，ctypes 调 `mpv_client_api_version` / `mpv_create` / `mpv_terminate_destroy` |

本机直连 GitHub release 仅 ~18 KB/s（30 MB 包约 28 分钟），故 dev 包下载与 DLL 探针放到 CI；仓库不提交二进制。

---

## 4. 环境坑记录（已解决 / 已绕过）

| 现象 | 根因 | 处理 |
|---|---|---|
| `failed to create directory target / 拒绝访问 (os error 5)` | 瞬时目录创建失败，权限探测正常 | 重试通过 |
| `windows-sys 0.61`: `dlltool.exe program not found` | 默认 `stable-x86_64-pc-windows-gnu`，`dlltool.exe`/`gcc` 只在 `/c/msys64/mingw64/bin` 且不在 PATH | 编译前 `export PATH="/c/msys64/mingw64/bin:$PATH"`，未改全局配置 |
| GitHub release 下载 18 KB/s | 本机直连限速 | 移到 CI Windows runner |
| UDP 1900 被占用 | Windows SSDP Discovery 服务（PID 6208） | 模拟闭环走已知地址（手动 IP 兜底） |
| Tauri 在 GNU 链编译慢 / 官方主推 MSVC | toolchain 差异 | 三平台编译交 CI（Windows runner 自带 MSVC） |

---

## 5. 仍未验证 / 后续

| 项 | 需要什么 |
|---|---|
| 原生 libmpv surface 渲染到透明窗口、Overlay 可点按 | 有 GUI 的机器 + libmpv DLL，人工看画面叠加与 z-order |
| 真机 DLNA（发现、播放、Seek、厂商 SOAP 差异） | 一台同网段 Android TV / 盒子，建立设备兼容表 |
| 干净机器启动 | 打出三平台安装包后，在无 mpv / ffmpeg 的机器验证 |
| S2 `MpvEngine` 最小封装 | 依赖 S1b |
| 正式工程 `src-tauri/` / `src/` | P0 放行后把可复用核心从 `spike/` 搬入 |

> 约定：方案确认后才实施；`spike/` 是可弃验证代码，验证通过只搬可复用核心，不把 Spike 骨架当正式架构。

---

## 6. GitHub Actions 首轮结果与修复

首轮 run `42172489258`（8 个 job，约 3 分钟）：

| Job | 结果 |
|---|---|
| S3 · HTTP 206（ubuntu / macos / windows） | ✅ 3/3 success |
| 模拟 DLNA 闭环（Linux runner） | ✅ success（10/10） |
| S1b · libmpv DLL probe（Windows） | ✅ success |
| S1 · 透明窗口工程（三平台） | ❌ 3/3 failure（均在 `Build Tauri app` 步骤） |

**根因**：`tauri-build` 找不到默认图标。本机复现得到确切报错：

```
`...\spike\tauri-overlay\icons/icon.ico` not found;
required for generating a Windows Resource file during tauri-build
```

匿名账号无法下载 job 日志（403 需 admin），因此改用「本机 CLI 复现 + check-run annotations 定位步骤」交叉定位；三平台同一步失败与图标缺失一致。

**修复**：

1. 生成最小占位图标集（`32x32.png` / `128x128.png` / `128x128@2x.png` / `icon.ico` / `icon.icns`，纯标准库、总计 < 2 KB），并在 `tauri.conf.json` 的 `bundle.icon` 显式声明。
2. CI 的 S1 判定收敛为 `cargo build --locked`（Spike 阶段只需验证编译链）+ `tauri info` 校验配置；失败时把日志尾部回写成 `::error::` annotation，便于后续匿名诊断。
3. **本机复验通过**：`npx tauri build --no-bundle --debug` → `exit 0`，产出 `target/debug/tauri-overlay-spike.exe`（46s）。

> 修复提交已落地。若远端代理 `CONNECT tunnel 502` 导致重推延迟，CI 复验结果以 Actions 页面最新 run 为准；本地构建与协议验证结论不受影响。
