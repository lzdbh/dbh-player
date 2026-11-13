pragma Singleton
import QtQuick

// Per-platform design tokens. Physical layout is shared; only the skin differs.
QtObject {
    readonly property bool isMac: AppContext.isMac
    readonly property bool isWindows: AppContext.isWindows

    readonly property color windowBg: isMac ? "#f5f5f7" : "#f3f3f3"
    readonly property color titleBarBg: isMac ? "#e8e8ed" : "#f9f9f9"
    readonly property color text: isMac ? "#1d1d1f" : "#1a1a1a"
    readonly property color textSecondary: isMac ? "#6e6e73" : "#606060"
    readonly property color accent: isMac ? "#007aff" : "#0067c0"
    readonly property color border: isMac ? "#d2d2d7" : "#e5e5e5"

    readonly property int titleBarHeight: isMac ? 38 : 40
    readonly property int cornerRadius: isMac ? 10 : 8
    readonly property string fontFamily: isMac ? ".AppleSystemUIFont" : "Segoe UI"
}