pragma Singleton
import QtQuick

// Ant Design v5 design tokens. The palette is shared; platform skins differ
// in chrome (title bar) and font, per the two-shell requirement.
QtObject {
    readonly property bool isMac: AppContext.isMac
    readonly property bool isWindows: AppContext.isWindows

    // Ant Design v5 seed-derived palette
    readonly property color colorPrimary: "#1677ff"
    readonly property color colorPrimaryHover: "#4096ff"
    readonly property color colorPrimaryActive: "#0958d9"
    readonly property color colorPrimaryBg: "#e6f4ff"
    readonly property color colorPrimaryBorder: "#91caff"

    readonly property color colorSuccess: "#52c41a"
    readonly property color colorWarning: "#faad14"
    readonly property color colorError: "#ff4d4f"
    readonly property color colorErrorHover: "#ff7875"
    readonly property color colorErrorBg: "#fff2f0"

    readonly property color colorText: "#e0000000"        // rgba(0,0,0,0.88)
    readonly property color colorTextSecondary: "#a6000000" // rgba(0,0,0,0.65)
    readonly property color colorTextTertiary: "#73000000"  // rgba(0,0,0,0.45)
    readonly property color colorTextQuaternary: "#40000000" // rgba(0,0,0,0.25)

    readonly property color colorBgContainer: "#ffffff"
    readonly property color colorBgLayout: "#f5f5f5"
    readonly property color colorBgElevated: "#ffffff"
    readonly property color colorFillSecondary: "#0f000000"  // rgba(0,0,0,0.06)
    readonly property color colorFillTertiary: "#0a000000"   // rgba(0,0,0,0.04)
    readonly property color colorBorder: "#d9d9d9"
    readonly property color colorBorderSecondary: "#f0f0f0"

    // Dark player chrome (video overlay) keeps antd blue as the accent
    readonly property color playerScrim: "#d9000000"
    readonly property color playerText: "#ffffff"
    readonly property color playerTextSecondary: "#a6ffffff"

    readonly property int radiusSM: 4
    readonly property int radius: 6
    readonly property int radiusLG: 8
    readonly property int controlHeight: 32
    readonly property int controlHeightSM: 24
    readonly property int fontSize: 14
    readonly property int fontSizeSM: 12
    readonly property int fontSizeLG: 16

    readonly property int titleBarHeight: isMac ? 38 : 40
    readonly property string fontFamily: isMac ? ".AppleSystemUIFont" : "Segoe UI"

    // Legacy aliases (pre-antd names still referenced by some components)
    readonly property color windowBg: colorBgLayout
    readonly property color titleBarBg: isMac ? "#ebebeb" : "#ffffff"
    readonly property color text: colorText
    readonly property color textSecondary: colorTextSecondary
    readonly property color accent: colorPrimary
    readonly property color border: colorBorder
    readonly property int cornerRadius: radius
}