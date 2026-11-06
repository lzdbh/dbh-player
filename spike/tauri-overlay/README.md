# S1 · 透明窗口 + libmpv + Overlay

分成两级，避免被缺失的 libmpv 动态库把整条链路卡死。

| 级 | 目标 | 依赖 | 当前状态 |
|---|---|---|---|
| **S1a** | Tauri 2.12 透明窗口 + Overlay 可点按 + IPC 往返 | Tauri CLI、WebView2 | 代码已落，**待装 CLI 后 `cargo run` 验证** |
| **S1b** | 接入 libmpv 0.41 原生 surface 输出测试画面 | Windows `libmpv-2.dll` | **阻塞**：本机未安装 libmpv |

## 跑 S1a

```bash
cd spike/tauri-overlay
npm i -D @tauri-apps/cli@^2
npx tauri dev          # 或 cargo tauri dev
```

也可以用 `cargo run` 直接跑（frontendDist 已指向同目录 `static/`，无 Vite 依赖）。

**验收**：窗口透明无黑底；Overlay 控制条能点；「点我验证 IPC」返回 `pong: overlay`；「查看 surface 状态」返回 `libmpv: not-linked ...`。

## S1b 接入要点（待动态库到位）

1. 把 mpv 官方 dev 包（GPL 构建）的 `libmpv-2.dll` 放到 `lib/`，`tauri.conf.json` 的 `resources: ["lib/**/*"]` 随包携带。
2. 在 `src/main.rs` 所在进程创建原生 HWND surface，把 `mpv_create` / `mpv_set_option` / `mpv_initialize` / `mpv_render_context_create` 接到该 surface；Vue/HTML Overlay 保持独立透明层，不参与解码。
3. 验证输出测试画面时 Overlay 仍可点按、不出现黑底或 z-order 反转。
4. 核查 `tauri-plugin-libmpv` / `libmpv-wrapper` 自身许可并写回 `THIRD_PARTY_NOTICES.md`。

> Spike 阶段刻意用静态 HTML 验证窗口分层，Vue 3 集成在 P1 正式工程落地；`src/app/bridge.ts` 单一订阅入口的设计见 `DESIGN.md` §4。

## CI 构建

本机默认 toolchain 是 `stable-x86_64-pc-windows-gnu`，Tauri 官方主要支持 MSVC，GNU 链上可能编译失败。因此三平台编译交给 `.github/workflows/p0-spike.yml` 的 `s1-tauri-overlay` job：Windows runner 自带 MSVC、macOS runner 自带 Xcode、Linux runner 会先装 WebKitGTK 4.1 依赖，再执行 `npm ci` + `tauri build --no-bundle`。

`package-lock.json` 必须入库（CI 用 `npm ci`），`node_modules/` 与 `target/` 由 `.gitignore` 排除。
