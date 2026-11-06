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

## 5. 当前 P0 状态

| 任务 | 状态 |
|---|---|
| S1 透明窗口 + libmpv + Overlay | 骨架已建，**编译验证阻塞于 Tauri CLI / libmpv 动态库** |
| S2 MpvEngine 最小封装 | 待 S1 |
| S3 206 Range 服务 | 代码已落，`cargo build` + curl 验证结果见 `HANDOVER.md` §11 |
| S4 真机 DLNA 推流与 Seek | **阻塞：无同网段 Android TV** |
| S5 干净机器启动检查 | 待打包后 |

> 当前开发机为 Windows，S1 只能本机验 Windows；macOS / Linux 计划走 CI 构建矩阵，透明窗口的视觉验收仍需真机。
