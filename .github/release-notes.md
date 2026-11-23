## 更新内容

- 修复窗口四角锯齿:Windows 11 改用 DWM 原生圆角(系统级抗锯齿 + 系统阴影),并取消整窗遮罩合成,播放时 GPU 开销更低。
- 控制栏改为仅在全屏时自动隐藏;窗口模式常驻,退出全屏或暂停时立即恢复。

## 下载

| 平台 | 文件 |
| --- | --- |
| Windows x64 | `DBHX-Player-__VERSION__-winx64-Setup.exe` |
| macOS(Apple Silicon + Intel) | `dbhx-player-__VERSION__-macos-universal.dmg` |

## 说明

- Windows 安装包为按用户安装,无需管理员权限。
- 安装包未签名:Windows SmartScreen 可能提示“未知发布者”,macOS 首次打开需右键选择“打开”。
- DLNA 投屏需要允许应用访问本地网络(macOS 会弹窗询问)。