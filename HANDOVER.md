# dbhx-player 工作交接文档

> 最后更新：2026-11-14（macOS 夜间实施 M3–M6 完成）
> 用途：回家在 **macOS 与 Windows** 上继续实施。本文档自包含，按 §5 配环境、§6 构建即可接手。
> 当前里程碑：**M0–M6 全部完成（macOS 端）**；M2 经真实电视直推验收，M3–M6 经模拟渲染器/单元测试/UI 自动化验证。Windows 端待回归。

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
| **M1** | 播放器核心（Track A）+ DLNA 设备发现（Track B） | ✅ 完成，**已提交+推送** `61ed0eb`；macOS 播放/发现均已验证 |
| **M2** | DLNA 直推投屏链路（本机 HTTP + SOAP + 投屏面板） | ✅ 完成，**真实电视直推验收通过**（播放/进度/时长/音量/停止；暂停经模拟渲染器验证） |
| **M3** | ffmpeg 转码兜底 | ✅ 完成（ffprobe 探测 + videotoolbox/libx264 → MPEG-TS 直播流；模拟渲染器验证 TS 字节流） |
| **M4** | 播放体验 + 分平台 UI 打磨（播放列表/字幕/倍速/截图/快捷键/托盘） | ✅ 完成，**UI 全面改为 Ant Design v5 风格**（中文文案，英文翻译随系统语言切换） |
| **M5** | 四目标打包、签名、公证、发布 | ✅ macOS 未签名 universal .app/.dmg 已出并验证；Windows 脚本备好待跑；签名/公证留待有证书（用户决策：未签名包+本地验证） |
| **M6** | 加固与优化（容错/性能/日志/崩溃上报/i18n/测试） | ✅ 完成（日志轮转、QSettings 持久化、qsTr+i18n、11 项单元测试、轮询容错；崩溃上报未做） |

## 4. 代码结构

```
dbhx-player/
├── CMakeLists.txt              # Qt6 工程；QML 模块扁平化；各 src 子目录 include
├── .gitignore / .clang-format
├── HANDOVER.md                 # 本文档
├── src/
│   ├── main.cpp                # 入口（QApplication+托盘）；Windows 默认 WMF；多文件命令行；i18n 加载
│   ├── app/AppContext.{h,cpp}  # QML 单例：平台判断 + 应用元信息 + QSettings 读写
│   ├── app/Logging.{h,cpp}     # 日志分类 + 文件轮转（M6）
│   ├── player/PlayerController.{h,cpp}  # 播放引擎封装（惰性创建；音量/静音/倍速持久化友好）
│   ├── player/PlaylistController.{h,cpp} # 播放列表模型 + 自动连播（M4）
│   ├── player/SubtitleTrack.{h,cpp}     # SRT 解析/定位（UTF-8/UTF-16/GB18030）（M4）
│   └── dlna/
│       ├── SsdpDiscovery.{h,cpp}    # SSDP 组播发现 MediaRenderer
│       ├── DeviceParser.{h,cpp}     # 解析设备描述 XML（friendlyName/控制地址）
│       ├── DlnaDeviceModel.{h,cpp}  # 设备列表模型（QAbstractListModel，暴露 QML）
│       ├── SoapClient.{h,cpp}       # UPnP SOAP 通用客户端（M2）
│       ├── AvTransportClient.{h,cpp}    # SetAVTransportURI/Play/Pause/Stop/Seek/GetPositionInfo（M2）
│       ├── RenderingControlClient.{h,cpp} # SetVolume/GetVolume/SetMute（M2）
│       ├── MediaHttpServer.{h,cpp}  # 本机 HTTP：文件 Range 模式 + 转码直播流模式（M2/M3）
│       ├── MediaProbe.{h,cpp}       # ffprobe 探测 + 直推兼容性判断（M3）
│       ├── TranscodeSession.{h,cpp} # ffmpeg → H.264+AAC MPEG-TS 直播流（M3）
│       └── CastManager.{h,cpp}      # 投屏编排（探测→直推/转码）+ 状态轮询（M2/M3）
├── cmake/Info.plist.in         # bundle 模板（本地化声明/网络权限/文件关联，见坑#9/#14）
├── cmake/dbhx.icns             # 应用图标
├── scripts/package-macos.sh    # 一键打包 .app/.dmg（M5）
├── scripts/package-windows.bat # Windows windeployqt 打包（待跑）
├── tests/                      # Qt Test 单元测试（M6）
├── translations/dbhx_en.ts     # 英文翻译（86 条，M6）
└── qml/
    ├── Main.qml                # 主窗口；快捷键/设置持久化/面板切换/Toast
    ├── theme/Theme.qml         # 单例：Ant Design v5 设计令牌（分平台字体/标题栏）
    ├── controls/               # AntButton/AntSlider/AntTag/AntIconButton/AntMenu/AntMenuItem/ColorIcon/Toast
    ├── icons/                  # 白色描边 SVG 图标（MultiEffect 着色）
    ├── chrome/{WindowsTitleBar,MacTitleBar}.qml  # 两套标题栏
    ├── player/{PlayerView,ControlBar,PlaylistPanel}.qml  # 播放视图/控制条/播放列表
    └── cast/{DeviceList,CastPanel}.qml  # 设备面板 + 投屏远程播控
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

### 5.2 macOS（本机已配好：Intel x86_64 + macOS 26）

**本机实际安装方式**（brew 在此机不可行，见坑#10）：
- **cmake / ninja / aqtinstall**：`python3 -m pip install --user -i https://pypi.tuna.tsinghua.edu.cn/simple cmake ninja aqtinstall`
  → 二进制在 `~/Library/Python/3.9/bin`（**不在 PATH**，构建前 export）。
- **Qt 6.12.0**：`aqt install-qt mac desktop 6.12.0 clang_64 -m qtmultimedia qtshadertools --outputdir ~/Qt --base https://mirror.nju.edu.cn/qt`
  → 装在 `~/Qt/6.12.0/macos`（universal 二进制）。清华源 qtbase 校验和不一致（坑#10），**用 NJU 源**。
- Xcode / CLT 已装（clang）；ffmpeg 已有（`/usr/local/bin/ffmpeg`）。

**新 Mac 复现**：同上两条命令即可；或官方在线安装器（需 Qt 账号）。arm64 机器同样适用（clang_64 是 universal）。

**后端说明**：macOS 默认 FFmpeg 后端（官方构建为 **LGPL**，无 Windows 那个崩溃问题）。`main.cpp` 的 WMF 设置仅在 `Q_OS_WIN` 生效。若遇问题可试 `QT_MEDIA_BACKEND=darwin`（AVFoundation）。

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

### macOS（本机已验证）
```bash
cd ~/my-git/dbh-player
export PATH="$HOME/Library/Python/3.9/bin:$PATH"   # pip 装的 cmake/ninja
cmake -S . -B build -G Ninja \
  -DCMAKE_PREFIX_PATH=$HOME/Qt/6.12.0/macos \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_OSX_ARCHITECTURES=x86_64        # Intel 本机；M5 发布改 "arm64;x86_64"
cmake --build build                        # 末尾自动 ad-hoc 签名（坑#9，必需）
open build/dbhx-player.app --args /path/video.mp4
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
9. **macOS 未签名二进制发不出组播**：sendto 组播返回 `EHOSTUNREACH`（errno 65，"No route to host"），SSDP 静默失败、Scan 无设备。**ad-hoc 签名即解决**（CMake 已加 POST_BUILD `codesign --force -s -`，勿删）；Info.plist 需含 `NSLocalNetworkUsageDescription`（模板在 `cmake/Info.plist.in`）。排查时可用 raw sendto C 程序看 errno 区分权限/路由。
10. **macOS 装 Qt 别用 brew/清华源**：本机（Intel + macOS 26）brew 无 bottle 会源码编译（Qt 要数小时）；清华 Qt 镜像 qtbase 校验和多次不一致（同步中多节点内容不同）；**NJU 源 `mirror.nju.edu.cn/qt` 校验一致且快**；官方 download.qt.io 极慢（~10KB/s）。pip 用清华源正常。
11. **QByteArray::startsWith 无大小写忽略重载**（Qt6.12）：用 `trimmed.toLower().startsWith("range:")`。
12. **QProcess::atEnd() 运行中缓冲为空也返回 true**：直播流据此判断 EOF 会提前断连。必须查 `process->state() == NotRunning && bytesAvailable() == 0`（MediaHttpServer 已处理）。
13. **QStringDecoder 无 ICU 不支持 GB18030**（isValid()=false 静默失败）：中文 SRT 兜底用 **Core5Compat 的 QTextCodec**（已加 `Qt6::Core5Compat` 依赖，aqt 模块 `qt5compat`）。
14. **macOS QLocale::system() 受 bundle 本地化声明影响**：Info.plist 只写 `CFBundleDevelopmentRegion=en` 时，中文系统也返回 en_CN。必须加 `CFBundleLocalizations=[zh-Hans,en]`（已加）。另：`-AppleLanguages "(en)"` 启动参数会被 macOS **持久化**到应用 prefs，测完要 `defaults delete com.dbhx.player AppleLanguages`。
15. **Qt Test 里 QTcpServer 需要事件循环**：客户端 `waitForReadyRead` 不派发其他 socket 的通知，服务端永远 accept 不了。测试里要 `QCoreApplication::processEvents` 泵循环（见 tests/dlnatest.cpp）。
16. **自定义 Button 宽度**：Basic 样式默认 padding 与 implicitWidth 公式不一致会差 1px 导致文字省略号。AntButton 用 TextMetrics + 显式 padding 解决；图标按钮记得设 `Accessible.name`（UI 自动化/无障碍都靠它）。
17. **libmpv + macOS coreaudio 两个致命坑**（升级 libmpv 内核时踩到）：
    - **OpenGL 渲染 API 会死锁 coreaudio**：`vo=libmpv` 用 OpenGL render API（libplacebo GL 初始化）时，coreaudio 音频初始化永久阻塞。**改用 SW 软件渲染 API**（`MPV_RENDER_API_TYPE_SW`，mpv 输出 RGBA 到内存缓冲，QQuickPaintedItem 绘制）即解决，音视频都正常。
    - **verbose 日志触发 coreaudio 空指针崩溃**：`mpv_request_log_messages(mpv, "v")` 会让 `ca_select_device` 走 `CA_GET_STR` 取设备名，在 Qt GUI 进程里空指针解引用崩溃（SIGSEGV）。**日志级别用 "info" 或更低**即可避开（该代码只在 MSGL_V 下执行）。
    - libmpv 预编译库在 `third_party/libmpv/`（x86_64，ffmpeg/libplacebo/libass 静态链入，仅依赖系统框架）；重建见 `scripts/build-libmpv.sh`。mpv≥0.42 强制依赖 libass；cocoa/swift 原生 VO 在新 SDK 编不过，已禁用（视频走 render API）。

## 8. DLNA 投屏架构

**原理**（App 作为控制点 DMC）：
1. **SSDP 发现**：组播 `M-SEARCH` 到 `239.255.255.250:1900`，收集 `MediaRenderer` 的 `LOCATION`。
2. **设备解析**：GET 设备描述 XML，取 `friendlyName` + `AVTransport`/`RenderingControl` 的 `controlURL`。
3. **本机 HTTP 服务**：把本地视频以 `http://<PC_IP>:<port>/media` 暴露，**必须支持 HTTP Range**（电视拖动进度）。
4. **SOAP 控制**：调电视 `AVTransport.SetAVTransportURI + Play`，让**电视自己拉流**；`RenderingControl` 控音量。
5. **转码兜底**（M3）：电视不兼容的格式（MKV/HEVC/DTS）用 ffmpeg 实时转 H.264+AAC 的 MPEG-TS 再喂。

**已实现（M1）**：步骤 1、2 + 设备列表 UI（`dlna/` + `cast/DeviceList.qml`）。
**已实现（M2）**：步骤 3、4 + 投屏编排 + 远程播控面板（`MediaHttpServer`/`SoapClient`/`AvTransportClient`/`RenderingControlClient`/`CastManager` + `cast/CastPanel.qml`）。

## 9. M2 完成记录与下一步（M3）

**M2 已完成并验收（2026-11-13，macOS + 真实电视「央视频电视投屏助手」）**：
- [x] MediaHttpServer：Range 全量/区间/suffix/416/HEAD 均经 curl 逐字节校验；DLNA Streaming 头；LAN IP 优选 en* 物理接口。
- [x] AvTransportClient / RenderingControlClient / SoapClient（含 DIDL-Lite 元数据、SOAP Fault 解析）。
- [x] CastManager：cast→SetAVTransportURI→Play→1s 轮询 GetPositionInfo/GetTransportInfo；电视端 STOPPED 自动断开。
- [x] 投屏面板：设备点击投屏、CastPanel 远程播放/暂停/进度/音量/断开；castFailed 错误横幅。
- [x] 验收：真实电视直推 MP4/H.264 播放完整 5s，远程进度/时长/音量（70%）回读正确，远程 Play 生效；模拟渲染器端到端 11 项断言全过（含 Pause/Seek/SetVolume）；20 次启动 0 崩溃。

**下一步 M3（ffmpeg 转码兜底）**：电视不兼容格式（MKV/HEVC/DTS）实时转 H.264+AAC MPEG-TS。注意 §12：需选 **LGPL ffmpeg 构建**（macOS 官方 Qt 自带 FFmpeg 已是 LGPL；Windows MSYS2 ffmpeg 是 GPL 需替换）。转码流由 MediaHttpServer 以 chunked 方式喂给电视（无 Content-Length，需处理电视不支持 chunked 的情况）。

## 9b. M3–M6 完成记录（2026-11-14 凌晨，macOS）

- **M3**：`MediaProbe`（ffprobe JSON，直推白名单 = MP4/MOV 容器 + H.264 + AAC/MP3）、`TranscodeSession`（优先 `h264_videotoolbox` 保 LGPL，libx264 兜底）、`MediaHttpServer.startLive`（单消费者直播流，无 Range）。模拟渲染器验证：HEVC MKV → 电视拉到合法 TS（0x47 同步字节），MP4 仍直推。
- **M4**：Ant Design v5 全套 UI（`qml/controls/Ant*.qml` + 白色描边 SVG 图标经 MultiEffect 着色）；播放列表（自动连播）、SRT 字幕（UTF-8/UTF-16/**GB18030**，同名自动加载）、倍速 0.5–2x、截图到 Pictures、快捷键（Space/方向键/F/M/Esc/Ctrl+O）、全屏、系统托盘。UI 自动化逐项验证通过。
- **M5**：`scripts/package-macos.sh` 一键出 universal .app + .dmg（macdeployqt 后**删 sqldrivers 再 ad-hoc 重签**）；`dist/dbhx-player-0.1.0-macos-universal.dmg` 已挂载实测可播。Windows 用 `scripts/package-windows.bat`（未跑）。
- **M6**：`Logging`（stderr + AppData/logs 轮转 5 份）、QSettings（音量/静音/倍速/窗口几何）、i18n（中文源 + `translations/dbhx_en.ts` 86 条全译，随系统语言切换）、`tests/`（playertest + dlnatest 共 11 例，ctest 全过）、投屏轮询连续 10 次失败才断开。
- **待用户验收**：真机投屏回归（M3 转码路径在真电视上的表现）、托盘图标、设置持久化手感。Windows 全量回归。

## 9c. 播放内核升级 libmpv（2026-11-14，解决本机 4K 卡顿）

- **动机**：Qt Multimedia FFmpeg 后端播 4K HEVC 偶尔音画卡顿。升级 libmpv（硬解 + 优秀帧同步）。
- **实现**：`src/player/MpvPlayer`（mpv 核心，主线程 + QTimer 轮询事件队列）+ `MpvVideoItem`（QQuickPaintedItem + **SW 渲染 API**）。QML 用 MpvPlayer/MpvVideoItem 替换 PlayerController/VideoOutput，对外属性/信号 API 保持一致（播放列表/字幕/投屏不受影响）。旧 `PlayerController` 暂保留未用。
- **获取库**：brew 无 x86_64 bottle、stolendata 不可达、CI 包是静态 mpv（无 libmpv）→ **从源码构建**（`scripts/build-libmpv.sh`，ffmpeg/libplacebo/libass 静态链入）。预编译 dylib 已提交到 `third_party/libmpv/`。
- **关键坑**：见坑#17（OpenGL render API 死锁 coreaudio → 用 SW 渲染；verbose 日志触发 coreaudio 崩溃 → 日志用 info）。
- **验证**：4K HEVC mkv 音画正常播放、暂停/seek/进度/音量均生效；单元测试全过；release dmg 独立运行通过。
- **注意**：libmpv 当前 x86_64 单架构 → release 包为 x86_64（universal 需 universal libmpv）。SW 渲染 4K 时 CPU 约 30%（可后续按需降渲染分辨率优化）。

## 10. Git 状态与跨机同步

- **remote**：`origin = https://github.com/lzdbh/dbh-player.git`，`main` 跟踪 `origin/main`。
- **M0/M1/M2**：已提交并推送（`1206ff8`、`61ed0eb`、`d05e4cb`）。
- **M3–M6**：macOS 端完成，提交记录见 git log（M3 `34baa41`、M4 `ca45f22`、M5 `ba19c08`、M6 见最新）。
- **回家后**：`git pull` → 按 §5 配环境 → §6 构建。
- ⚠️ **网络**：github.com 在你的网络下需 **Steam++ 加速**（并加 revoke 豁免）才通；push/pull 前确认加速开启。macOS 本机 pip 走清华源、Qt 走 NJU 源（坑#10）。**macOS 已配好 PAT（osxkeychain），可直接 push**。
- **提交约定**：`feat:`/`fix:` 前缀；**UI 改动先验收再提交**。

## 11. 测试与验证

- **稳定性**（Windows 改后端/播放相关后必跑）：循环启动 N 次数崩溃：
  ```bash
  c=0; for i in $(seq 1 20); do timeout 4 ./build/dbhx-player.exe >/dev/null 2>&1; [ $? -eq 139 ] && c=$((c+1)); done; echo "crashes=$c/20"
  ```
- **播放确证**：命令行传测试视频，开 `QT_FORCE_STDERR_LOGGING=1`，确认 duration/position 推进（WMF 不打 FFmpeg 那种日志，可临时在 QML 加 `onPositionChanged` 打印）。
- **DLNA 发现**：电视与电脑同 Wi-Fi 并开启 → 控制条 **📺 → Scan** → 应列出设备。macOS 若 Scan 无设备先查坑#9（签名）。
- **投屏**：Scan 后点击设备 → 电视应播放当前打开的视频，侧栏变为 CastPanel 远程播控。
- **单元测试**：`ctest --test-dir build --output-on-failure`（playertest + dlnatest，11 例）。
- **打包**：`scripts/package-macos.sh`（universal .app + .dmg 到 dist/）。

## 12. 许可证注意（M5 发布前处理）

- **Qt**：LGPLv3 / 商业双授权。闭源产品用 LGPL 需**动态链接**（部署工具默认即是）。
- **ffmpeg**：MSYS2 的 ffmpeg 是 **GPL**（含 x264/x265）。**产品发布需换 LGPL 构建**（去掉 GPL 编码器）或整体 GPL。M3 集成转码时就要选定 LGPL 构建。
- H.264 等专利授权与 Qt/ffmpeg 许可证无关，另行评估。

---

## 附：M2 改动清单（2026-11-13，macOS）

```
 M CMakeLists.txt        # +M2 源码/CastPanel.qml、macOS Info.plist 模板、POST_BUILD ad-hoc 签名
 M qml/Main.qml          # +CastManager、CastPanel、castFailed 错误横幅
 M qml/cast/DeviceList.qml   # 设备点击 → castManager.cast(...)
 M qml/player/PlayerView.qml # 右键 Cast to Device 启用
 M .gitignore            # +test/（本地测试视频）
?? cmake/Info.plist.in   # NSLocalNetworkUsageDescription
?? qml/cast/CastPanel.qml
?? src/dlna/{SoapClient,AvTransportClient,RenderingControlClient,MediaHttpServer,CastManager}.{h,cpp}
```