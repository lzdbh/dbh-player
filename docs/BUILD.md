# 构建说明

> 面向 P0 Spike。正式工程尚未建立；`spike/` 下为可弃验证代码，验证通过后只把可复用核心搬进 `src-tauri/`。

## 1. 前置与本机探测（2026-11-06 13:45）

| 项 | 要求 | 本机状态 |
|---|---|---|
| Rust / cargo | ≥ 1.90（Tauri 2.12 MSRV） | 1.99.1 ✅ |
| rustup | — | 1.29.2 ✅ |
| Node / npm | 20+ | v22.22.2 / 10.9.7 ✅ |
| WebView2 Runtime（Windows） | 随 Edge | 已安装 ✅ |
| Tauri CLI | 2.12.x | **未安装** ⛔ |
| libmpv 动态库 | 0.41 | **未安装** ⛔ |
| 同网段 Android TV | DLNA 接收端 | **不具备** ⛔（S4 阻塞） |
| git 身份 | — | `lzdbh <lzdbh@163.com>` ✅ |

补齐前置：

```bash
# Tauri CLI（推荐走 npm，不必 cargo install 全量编译）
npm i -D @tauri-apps/cli@^2

# libmpv：见第 2 节，Windows 放 src-tauri/lib/ 并在 tauri.conf.json 配 resources
```

## 2. 三平台 libmpv 来源（选 A：GPL 构建即可）

| 平台 | 来源 | 渲染 surface | 打包 |
|---|---|---|---|
| Windows | mpv 官方 dev 预编译包（GPL 构建），取 `libmpv-2.dll` / `mpv.def` + 头文件 | D3D11 + 原生 HWND | DLL 随 `resources: lib/**/*`，附 MSVC 运行库 |
| macOS | **源码编译**：Homebrew 版为 Vulkan-only，不适合嵌入；mpv 0.41 需配 **ffmpeg@7**（与 ffmpeg 8 不兼容） | `NSOpenGLView` / Metal | `dylibbundler` 处理 dylib + rpath；签名公证在 P1 |
| Linux | 发行版 `libmpv-dev`，或自行编译后随包携带 `.so` | EGL + X11 子窗口 / Wayland subsurface | AppImage + `patchelf`（P1） |

> 必须记录**实际采用的二进制版本号与下载来源**，写回 `THIRD_PARTY_NOTICES.md`。Tauri 2.12.1+ 在 macOS 使用透明窗口已免 `macos-private-api`。

## 3. 关键配置

| 文件 | 要点 |
|---|---|
| `src-tauri/tauri.conf.json` | `transparent: true`；`resources: ["lib/**/*"]`；`frontendDist` 指向 Vite 产物 |
| `src-tauri/capabilities/default.json` | 最小权限白名单，不开放任意文件读取 |
| `src/app/bridge.ts` | 唯一 IPC / 事件订阅入口（正式工程） |

## 4. S3 · MediaServer 206 Range 验证

`spike/media-server/` 是独立的 axum 小 crate，不依赖 Tauri / libmpv，可单独编译验证：

```bash
cd spike/media-server
cargo build
./target/debug/cast-media-spike <测试文件绝对路径> dev-token 18765

# 另开终端
curl -i -H "Range: bytes=100-199" "http://127.0.0.1:18765/media/sample?token=dev-token"
curl -i -H "Range: bytes=99999999-" "http://127.0.0.1:18765/media/sample?token=dev-token"   # 期望 416
curl -i "http://127.0.0.1:18765/media/sample?token=wrong"                                   # 期望 403
curl -i "http://127.0.0.1:18765/health?token=dev-token"
```

判定：206 响应带 `Content-Range`、`Accept-Ranges: bytes`、`Content-Length` 与正确 MIME；越界 416；token 错 403；URL 只认映射 ID、不接受原始路径。

## 5. 当前 P0 状态（2026-11-06 13:50）

| 任务 | 状态 |
|---|---|
| S1a 透明窗口 + Overlay + IPC | 工程骨架已建（`spike/tauri-overlay/`），**待装 Tauri CLI 后运行验证** |
| S1b libmpv 原生 surface | **阻塞：本机未安装 libmpv 动态库** |
| S2 MpvEngine 最小封装 | 待 S1b |
| **S3 206 Range 服务** | ✅ **已编译并 curl 实测通过**（见下表） |
| S4 真机 DLNA 推流与 Seek | **阻塞：无同网段 Android TV** |
| S5 干净机器启动检查 | 待打包后 |

### 环境修复记录

默认 toolchain 是 `stable-x86_64-pc-windows-gnu`，`windows-sys 0.61` 构建时调用 `dlltool.exe`，而该文件只在 `/c/msys64/mingw64/bin/`、不在 PATH，导致 `error calling dlltool: program not found`。**编译前临时加 PATH 即可，不改全局配置**：

```bash
export PATH="/c/msys64/mingw64/bin:$PATH"   # 提供 dlltool + gcc（GNU 链接器）
cd spike/media-server && cargo build
```

首次 `cargo build` 还出现过一次 `failed to create directory target / 拒绝访问 (os error 5)`，重试即通过（瞬时目录创建失败，非权限策略问题）。

### S3 实测结果（512 KB 样本，端口 18765）

| 用例 | 结果 |
|---|---|
| `Range: bytes=100-199` | `206` + `content-range: bytes 100-199/524288`、`content-length: 100`、`accept-ranges: bytes`，body 实测 100 字节 ✅ |
| `Range: bytes=-10` | `206` + `bytes 524278-524287/524288`、`content-length: 10` ✅ |
| `Range: bytes=99999999-` | `416` + `content-range: bytes */524288` ✅ |
| 无 Range | `200` + `content-length: 524288` + `accept-ranges: bytes` ✅ |
| token 错误 / 未知 media id / health | `403` / `404` / `200 ok` ✅ |

> 样本为 `.bin`，故 MIME 为 `application/octet-stream`；MKV / MP4 / TS / 字幕的 MIME 映射已在代码，正式接入时用真实文件复验。Spike 绑定 `127.0.0.1`，正式版需绑局域网地址并按已配对设备 IP 过滤。

> 当前开发机为 Windows，S1 只能本机验 Windows；macOS / Linux 计划走 CI 构建矩阵，透明窗口的视觉验收仍需真机。

## 6. GitHub Actions 三平台构建

远端：`https://github.com/lzdbh/dbh-player.git`（空仓起步）。workflow：`.github/workflows/p0-spike.yml`，触发条件为 push `master` / `main`、PR、`workflow_dispatch`。

| Job | 平台 | 验证内容 |
|---|---|---|
| `s3-media-server` | ubuntu / macos / windows | `cargo build --locked`；Linux 上另跑 206 / 416 curl 复验 |
| `s1-tauri-overlay` | ubuntu / macos / windows | 装 Linux WebKitGTK 4.1 依赖 → `npm ci` → `tauri build --no-bundle` |
| `cast-loop` | ubuntu | 启动 S3 服务，跑 `spike/cast-loop/cast_loop_test.py` 模拟 DLNA 闭环 |
| `libmpv-probe` | windows | 从 GitHub releases 下载 `mpv-dev-x86_64-*.7z`，7z 解压后用 ctypes 调 `mpv_client_api_version` / `mpv_create` / `mpv_terminate_destroy` |

本机直连 GitHub release 仅 ~18 KB/s（30 MB 包约需 28 分钟），因此 dev 包的下载与 DLL 探针放到 CI 的 Windows runner 执行；仓库**不提交任何 DLL / 7z 二进制**（见 `.gitignore`）。

```bash
git remote add origin https://github.com/lzdbh/dbh-player.git
git push -u origin master
# 或在 GitHub 仓库页 Actions → P0 Spike → Run workflow 手动触发
```

> CI 通过只证明「可编译 + 协议层闭环」，**不等于** Go/No-Go 全过：真机 DLNA、原生 surface 画面叠加与干净机器启动仍需人工验收。
