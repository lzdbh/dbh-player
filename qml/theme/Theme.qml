pragma Singleton
import QtQuick

// Dark slate design tokens (reference: dark media player UI with translucent
// panels, white text and a blue accent). Physical layout is shared; only the
// chrome differs per platform.
QtObject {
    readonly property bool isMac: AppContext.isMac
    readonly property bool isWindows: AppContext.isWindows

    readonly property color colorPrimary: "#2f8cf0"
    readonly property color colorPrimaryHover: "#4a9df5"
    readonly property color colorPrimaryActive: "#1c6fd1"
    readonly property color colorPrimaryBg: "#1d2f42"
    readonly property color colorPrimaryBorder: "#2c4b68"

    readonly property color colorSuccess: "#52c41a"
    readonly property color colorWarning: "#faad14"
    readonly property color colorError: "#ff6b6b"
    readonly property color colorErrorHover: "#ff8a8a"
    readonly property color colorErrorBg: "#3a2323"

    readonly property color colorText: "#eef2f6"
    readonly property color colorTextSecondary: "#aab6c1"
    readonly property color colorTextTertiary: "#7f8d9a"
    readonly property color colorTextQuaternary: "#57656f"

    readonly property color colorBgContainer: "#2e3c48"
    readonly property color colorBgLayout: "#26333e"
    readonly property color colorBgElevated: "#1e282f"
    readonly property color colorFillSecondary: "#1affffff"  // white ~10%
    readonly property color colorFillTertiary: "#0fffffff"   // white ~6%
    readonly property color colorBorder: "#33ffffff"
    readonly property color colorBorderSecondary: "#1fffffff"

    // Dark player chrome over video
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

    // Legacy aliases
    readonly property color windowBg: colorBgLayout
    readonly property color titleBarBg: isMac ? "#222e38" : colorBgLayout
    readonly property color text: colorText
    readonly property color textSecondary: colorTextSecondary
    readonly property color accent: colorPrimary
    readonly property color border: colorBorder
    readonly property int cornerRadius: radius
}