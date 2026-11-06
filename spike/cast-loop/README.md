# 模拟 DLNA 投屏闭环

无真机 Android TV 时，用一个最小 UPnP AVTransport MediaRenderer 代替电视，验证投屏协议链路。**Renderer 会真的用 HTTP Range 向 PC 媒体服务拉流**，所以 206 命门是被端到端跑过的。

```bash
cd spike/cast-loop
python cast_loop_test.py --media-server ../media-server/target/debug/cast-media-spike
```

Windows 上脚本会自动探测 `.exe`；无外部 Python 依赖，只用标准库。CI 的 `cast-loop` job 也在 Linux runner 上跑同一脚本。

## 覆盖的环节

| 步骤 | 验证点 |
|---|---|
| 发现 | 取 `description.xml`、解析 `controlURL` / `SCPDURL`，取 SCPD 动作表 |
| 推 URL | `SetAVTransportURI` 推 token 化 HTTP 地址（不是画面） |
| 播放 | `Play` → Renderer 发 `Range` GET，PC 回 `206` + `Content-Range`，实收 100 字节 |
| 状态 | `GetPositionInfo` 轮询返回 `RelTime` |
| 拖进度 | `Seek 00:00:30` → Renderer 从**新字节偏移**再拉一次，仍为 206，且 Range 与首次不同 |
| 暂停 | `Pause` 状态切换；动作序列校验 |

## 明确不覆盖

- **SSDP 多播发现**：本机 UDP 1900 被 Windows SSDP 服务占用，脚本走「已知设备地址」路径，等价于方案里的手动 IP 兜底。
- **真实电视厂商的 SOAP 实现差异**、设备兼容表、遥控器 DPad、10-foot UX。
- **Widevine / 解码能力**：Renderer 只验证拉流字节，不解码。

> 因此这项通过**不等于** S4 真机验收通过；`PLAN.md` §4 的 Go/No-Go 判定 2、3 仍需真机确认。
