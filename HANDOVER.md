# dbhx-player 工作交接文档

> 最后更新：2026-11-13
> 用途：回家在 **macOS 与 Windows** 上继续实施。本文档自包含，按 §5 配环境、§6 构建即可接手。
> 当前里程碑：**M0 已提交并推送；M1 代码完成（未提交，待验收）**。

---

## 1. 项目概览

- **目标**：跨平台视频播放器，支持 **DLNA 投屏**到电视（当贝投屏 / 乐播投播等标准 DLNA 渲染器）。
- **平台**：Windows / macOS × amd64 / arm64（四目标）。
- **核心需求**：
  1. UI 美观，且 **Windows / macOS 界面区分**（两套外壳皮肤）。
  2. 常规播放操作 + **右键菜单**。
  3. **DLNA 投屏**：直推 + **ffmpeg 转码兜底**（不接乐播私有 SDK）。

## 2. 技术栈（已锁定，勿轻易更改）

| 层 | 选型 | 备注 |
|---|---|---|
| 框架 | **Qt 6.12 LTS** | 支持到 2031；CMake ≥ 3.25、C++17 |
| UI | **Qt Quick / QML** | 无边框窗口 + 分平台自绘标题栏 |
| 播放内核 | **Qt Multimedia** | **Windows 用 WMF 后端**（见 §7 坑#1）；macOS 用默认后端 |
| DLNA | **Qt Network 自研** | QUdpSocket(SSDP) + QNetworkAccessManager(SOAP) + 本机 HTTP |
| 转码兜底 | **ffmpeg**（M3） | 实时转 H.264+AAC → MPEG-TS |
| 打包 | windeployqt / macdeployqt | M5 |

**关键决策记录**：
- 选 Qt 而非 Flutter：原生性能、桌面集成（原生右键/托盘）、Windows arm64 原生、LTS 到 2031。
- 播放内核选 Qt Multimedia 而非 libmpv：集成简单、自带全格式；若后期字幕/容错不够再升级 libmpv（`player/` 已做抽象）。
- **Windows 播放后端 = WMF**（`QT_MEDIA_BACKEND=windows`，已写入 `src/main.cpp`，可被环境变量覆盖）。原因见 §7 坑#1。

## 3. 路线图与当前进度

| 阶段 | 内容 | 状态 |
|---|---|---|
| **M0** | 工程骨架：CMake+Qt6、无边框窗口、分平台标题栏、AppContext、Theme | ✅ 完成，**已提交+推送** `1206ff8` |
| **M1** | 播放器核心（Track A）+ DLNA 设备发现（Track B） | ✅ 代码完成、本地播放已验证、WMF 稳定（20 次 0 崩溃）；**未提交**；DLNA 发现待真实电视验收 |
| **M2** | DLNA 直推投屏链路（本机 HTTP + SOAP + 投屏面板） | ⬜ 待做（**下一步**，见 §9） |
| **M3** | ffmpeg 转码兜底 | ⬜ 待做 |
| **M4** | 播放体验 + 分平台 UI 打磨（播放列表/字幕/倍速/截图/快捷键/托盘） | ⬜ 待做 |
| **M5** | 四目标打包、签名、公证、发布 | ⬜ 待做 |
| **M6** | 加固与优化（容错/性能/日志/崩溃上报/i18n/测试） | ⬜ 可选 |

## 4. 代码结构

```
dbhx-player/
├── CMakeLists.txt              # Qt6 工程；QML 模块扁平化；各 src 子目录 include
├── .gitignore / .clang-format
├── HANDOVER.md                 # 本文档
├── src/
│   ├── main.cpp                # 入口；Windows 默认 WMF 后端；命令行打开文件
│   ├── app/AppContext.{h,cpp}  # QML 单例：平台判断 + 应用元信息
│   ├── player/PlayerController.{h,cpp}  # 播放引擎封装（惰性创建 QMediaPlayer）
│   └── dlna/
│       ├── SsdpDiscovery.{h,cpp}    # SSDP 组播发现 MediaRenderer
│       ├── DeviceParser.{h,cpp}     # 解析设备描述 XML（friendlyName/控制地址）
│       └── DlnaDeviceModel.{h,cpp}  # 设备列表模型（QAbstractListModel，暴露 QML）
└── qml/
    ├── Main.qml                # 主窗口；PlayerController + DlnaDeviceModel 实例
    ├── theme/Theme.qml         # 单例：分平台设计令牌（颜色/字体/圆角）
    ├── chrome/{WindowsTitleBar,MacTitleBar}.qml  # 两套标题栏
    ├── player/{PlayerView,ControlBar}.qml        # 播放视图 + 控制条（右键菜单内联在 PlayerView）
    └── cast/DeviceList.qml     # 投屏设备面板（Scan + 列表）
```

> 约定：`player/` 是播放抽象层，`dlna/` 是投屏层，二者通过 QML（Main.qml）组合。M2 的 CastManager 将放在 `dlna/`。

## 5. 环境搭建

### 5.1 Windows（本机已配好；新机器按此复现）

本机工具链（MSYS2 MinGW64，`C:\msys64`）：
- Qt **6.12.0**：`qt6-base`、`qt6-declarative`、`qt6-multimedia`、`qt6-multimedia-wmf`
- CMake 4.4.5、Ninja 1.13.2、g++ 16.2.0、gdb、ffmpeg 9.0.2、zlib 1.4

**新 Windows 机器复现步骤**：
1. 安装 [MSYS2](https://www.msys2.org/)。
2. **换国内镜像**（默认源慢/503）。编辑 `C:\msys64\etc\pacman.d\mirrorlist.mingw` 与 `mirrorlist.msys`，把阿里云提到最前：
   ```
   Server = https://mirrors.aliyun.com/msys2/mingw/$repo/      # mirrorlist.mingw
   Server = https://mirrors.ustc.edu.cn/msys2/mingw/$repo/
   Server = https://mirror.nju.edu.cn/msys2/mingw/$repo/
   Server = https://mirrors.cloud.tencent.com/msys2/mingw/$repo/
   ```
   ```
   Server = https://mirrors.aliyun.com/msys2/msys/$arch/       # mirrorlist.msys
   Server = https://mirrors.ustc.edu.cn/msys2/msys/$arch/
   Server = https://mirror.nju.edu.cn/msys2/msys/$arch/
   Server = https://mirrors.cloud.tencent.com/msys2/msys/$arch/
   ```
   （本机原文件已备份为 `*.bak`。）
3. 在 MSYS2 shell 安装：
   ```bash
   pacman -Sy
   pacman -S --needed mingw-w64-x86_64-qt6-base mingw-w64-x86_64-qt6-declarative \
     mingw-w64-x86_64-qt6-multimedia mingw-w64-x86_64-qt6-multimedia-wmf \
     mingw-w64-x86_64-cmake mingw-w64-x86_64-ninja mingw-w64-x86_64-gdb \
     mingw-w64-x86_64-ffmpeg
   ```
   > 若 ffmpeg 报缺 `libz-1.dll`：`pacman -S mingw-w64-x86_64-zlib`（需 1.4+）。

### 5.2 macOS（回家新配）

1. **Xcode Command Line Tools**（clang）：`xcode-select --install`
2. **Qt 6.12 LTS**（二选一）：
   - **官方在线安装器**（推荐，精确 6.12 LTS，macOS universal 含 arm64+x86_64）：qt.io 下载，登录 Qt 账号，勾选 `Qt 6.12 LTS → macOS` + `Qt Multimedia` + Developer Tools（CMake/Ninja）。
   - 或 Homebrew：`brew install qt cmake ninja`（版本随 brew，可能非 6.12）。
3. **后端说明**：macOS 默认用 FFmpeg 后端（官方构建，**无 Windows 那个崩溃问题**）。`main.cpp` 的 WMF 设置仅在 `Q_OS_WIN` 生效，Mac 不受影响。若 Mac 遇问题可试 `QT_MEDIA_BACKEND=darwin`（AVFoundation）。
4. arm64 / x86_64：Qt macOS kit 是 universal；构建时用 `CMAKE_OSX_ARCHITECTURES="arm64;x86_64"` 出通用二进制。

## 6. 构建与运行

### Windows（MSYS2 MINGW64 shell）
```bash
cd /e/my-git/dbhx-player
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build
./build/dbhx-player.exe                 # 运行
./build/dbhx-player.exe /path/video.mp4 # 命令行打开文件
```
> 在 Git Bash 里也可：`MSYSTEM=MINGW64 C:/msys64/usr/bin/bash.exe -lc "cd /e/my-git/dbhx-player && cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release && cmake --build build"`
> 运行时需 `/mingw64/bin` 在 PATH（MINGW64 shell 自动满足）。

### macOS
```bash
cd ~/path/to/dbhx-player
# Qt 官方安装器路径示例：~/Qt/6.12.0/macos；brew 则用 $(brew --prefix qt)
cmake -S . -B build -G Ninja \
  -DCMAKE_PREFIX_PATH=~/Qt/6.12.0/macos \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_OSX_ARCHITECTURES="arm64;x86_64"
cmake --build build
open build/dbhx-player.app
```

### 生成测试视频（任意平台，需 ffmpeg）
```bash
ffmpeg -y -f lavfi -i testsrc=duration=5:size=640x360:rate=30 \
  -f lavfi -i sine=frequency=440:duration=5 \
  -c:v libx264 -pix_fmt yuv420p -c:a aac -shortest test.mp4
```

## 7. 关键坑与约定（必读，都是踩过的）

1. **Windows 勿用 FFmpeg 后端**：MSYS2 的 Qt6.12 FFmpeg 后端（gcc16 + FFmpeg9）有**间歇性启动段错误**（调用固定非法地址，初始化竞态；gdb 下不复现、对内存布局极敏感）。已改用 **WMF 后端**（`main.cpp` 里 `qputenv("QT_MEDIA_BACKEND","windows")`）。**不要在 Windows 改回 ffmpeg 后端**。WMF 直放常见格式，冷门格式由 M3 转码兜底。
2. **QML 模块扁平化**：CMake 用 `QT_RESOURCE_ALIAS` 把 `qml/子目录/*.qml` 映射到模块根，配合 `engine.loadFromModule("Dbhx","Main")`。新增 QML 文件要在 CMake 加 alias + QML_FILES。
3. **QML 类型注册的 include**：生成的注册代码用 `__has_include(<Xxx.h>)` 引头文件，CMake 必须把对应 `src/子目录` 加进 `target_include_directories`，否则报 "Xxx not declared"。
4. **VideoOutput 连接方式**：Qt6.12 的 `VideoOutput` **没有** `mediaPlayer` 属性；正确做法是 `mediaPlayer.videoOutput = videoSurface`（见 PlayerView 的 `onMediaPlayerChanged`）。
5. **右键 Menu 必须内联**：以 `Menu` 为根的独立 QML 组件作为 Item 子对象会报 "Type cannot be created"。右键菜单直接内联在 `PlayerView.qml`。
6. **播放器惰性创建**：`PlayerController::ensurePlayer()` 首次 open/play 才创建 QMediaPlayer，避开启动期初始化竞态。属性 getter 均处理 null。
7. **ffmpeg 依赖 zlib 1.4**（`libz-1.dll`）；旧 zlib 1.3 只有 `zlib1.dll`，需升级。
8. **分平台 UI**：`AppContext.isMac/isWindows` 判断；`Theme.qml` 提供令牌；标题栏在 `chrome/`。新增 UI 记得两套皮肤都顾及。

## 8. DLNA 投屏架构

**原理**（App 作为控制点 DMC）：
1. **SSDP 发现**：组播 `M-SEARCH` 到 `239.255.255.250:1900`，收集 `MediaRenderer` 的 `LOCATION`。
2. **设备解析**：GET 设备描述 XML，取 `friendlyName` + `AVTransport`/`RenderingControl` 的 `controlURL`。
3. **本机 HTTP 服务**：把本地视频以 `http://<PC_IP>:<port>/media` 暴露，**必须支持 HTTP Range**（电视拖动进度）。
4. **SOAP 控制**：调电视 `AVTransport.SetAVTransportURI + Play`，让**电视自己拉流**；`RenderingControl` 控音量。
5. **转码兜底**（M3）：电视不兼容的格式（MKV/HEVC/DTS）用 ffmpeg 实时转 H.264+AAC 的 MPEG-TS 再喂。

**已实现（M1）**：步骤 1、2 + 设备列表 UI（`dlna/` + `cast/DeviceList.qml`）。
**待实现（M2）**：步骤 3、4 + 投屏编排 + 远程播控面板。

## 9. 下一步：M2 详细任务（直推投屏链路）

在 `src/dlna/` 新增：
- [ ] **MediaHttpServer**：`QTcpServer`（或 QtHttpServer）本机 HTTP，**支持 Range**，暴露当前播放文件；探测本机局域网 IP。
- [ ] **AvTransportClient**：SOAP 客户端（QNetworkAccessManager POST XML），实现 `SetAVTransportURI / Play / Pause / Stop / Seek / GetPositionInfo`。
- [ ] **RenderingControlClient**：SOAP `SetVolume / GetVolume / SetMute`。
- [ ] **CastManager**：编排「选设备 → 起 HTTP → SetAVTransportURI → Play」，暴露 QML；管理投屏状态。
- [ ] **投屏面板 UI**：设备点击投屏、投屏中远程播控（播放/暂停/进度/音量/停止）。
- [ ] CMake 加新源码 + include；QML 接入。

**M2 验收**：MP4/H.264 能直推电视播放，PC 端可远程播放/暂停/进度/音量。
**提示**：SOAP action 头 `SOAPAction: "urn:schemas-upnp-org:service:AVTransport:1#Play"`；controlURL 已在 DeviceParser 解析好（`avTransport` role）。

## 10. Git 状态与跨机同步

- **remote**：`origin = https://github.com/lzdbh/dbh-player.git`，`main` 跟踪 `origin/main`。
- **M0**：已提交并推送（`1206ff8`）。
- **M1**：**未提交**（工作区改动：`CMakeLists.txt`、`Main.qml`、`main.cpp` + 新增 `player/`、`dlna/`、`cast/`）。
- **回家前**：验收 M1 后 `git add` 相关文件 → `git commit` → `git push`。
- **回家后**：`git pull` → 按 §5 配环境 → §6 构建。
- ⚠️ **网络**：github.com 在你的网络下需 **Steam++ 加速**（并加 revoke 豁免）才通；push/pull 前确认加速开启。
- **提交约定**：`feat:`/`fix:` 前缀；**UI 改动先验收再提交**。

## 11. 测试与验证

- **稳定性**（Windows 改后端/播放相关后必跑）：循环启动 N 次数崩溃：
  ```bash
  c=0; for i in $(seq 1 20); do timeout 4 ./build/dbhx-player.exe >/dev/null 2>&1; [ $? -eq 139 ] && c=$((c+1)); done; echo "crashes=$c/20"
  ```
- **播放确证**：命令行传测试视频，开 `QT_FORCE_STDERR_LOGGING=1`，确认 duration/position 推进（WMF 不打 FFmpeg 那种日志，可临时在 QML 加 `onPositionChanged` 打印）。
- **DLNA 发现**：电视与电脑同 Wi-Fi 并开启 → 控制条 **📺 → Scan** → 应列出设备。

## 12. 许可证注意（M5 发布前处理）

- **Qt**：LGPLv3 / 商业双授权。闭源产品用 LGPL 需**动态链接**（部署工具默认即是）。
- **ffmpeg**：MSYS2 的 ffmpeg 是 **GPL**（含 x264/x265）。**产品发布需换 LGPL 构建**（去掉 GPL 编码器）或整体 GPL。M3 集成转码时就要选定 LGPL 构建。
- H.264 等专利授权与 Qt/ffmpeg 许可证无关，另行评估。

---

## 附：当前未提交改动清单（M1）

```
 M CMakeLists.txt        # +Multimedia/Network、player/dlna 源码、DeviceList.qml、include
 M qml/Main.qml          # +PlayerController/DlnaDeviceModel、RowLayout 播放器+设备侧栏
 M src/main.cpp          # +Windows WMF 默认后端、命令行打开文件
?? qml/player/           # PlayerView.qml, ControlBar.qml
?? qml/cast/             # DeviceList.qml
?? src/player/           # PlayerController.{h,cpp}
?? src/dlna/             # SsdpDiscovery/DeviceParser/DlnaDeviceModel.{h,cpp}
```