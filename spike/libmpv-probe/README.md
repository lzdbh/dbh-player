# libmpv DLL 探针（S1b 前置）

验证 Windows 版 `libmpv-2.dll` 能被加载、核心 C API 可调用。由 GitHub Actions 的 `libmpv-probe` job 执行——CI 的 Windows runner 下载 mpv dev 包快，且自带 MSVC 运行库。

```bash
python probe.py --dll /path/to/libmpv-2.dll
```

检查项：`mpv_client_api_version` 返回非零版本号、`mpv_create` 返回有效句柄、`mpv_terminate_destroy` 正常销毁。

## dev 包来源（GPL 构建，选 A）

- 仓库：`shinchiro/mpv-winbuild-cmake` releases
- 本次记录版本：tag `20261106`，资产 `mpv-dev-x86_64-20261106-git-9028187538.7z`（31.8 MB）
- 解压后取 `libmpv-2.dll` 放入 `spike/tauri-overlay/lib/`（该目录已在 `.gitignore` 中排除）

## 不覆盖

- 原生 surface 渲染到透明窗口（需要 GUI 手动验收）。
- mpv 渲染上下文 / `wid` HWND 绑定、VO 输出测试画面。
- 三平台动态库打包（dylib / so / AppImage）。

> 这些属于 S1b / S2，需在装有 libmpv 的 GUI 环境里继续验证。