#![cfg_attr(not(debug_assertions), windows_subsystem = "windows")]

//! P0 S1a spike: transparent Tauri window + clickable Overlay + IPC round-trip.
//!
//! S1b (libmpv native surface rendering a test pattern) is intentionally NOT
//! wired in yet: the Windows libmpv DLL is missing on this machine. Once the
//! DLL is placed under lib/, the MpvEngine goes into this process and the
//! overlay stays a separate transparent layer on top of the native surface.

#[tauri::command]
fn overlay_ping(label: String) -> String {
    format!("pong: {}", label)
}

#[tauri::command]
fn surface_status() -> serde_json::Value {
    serde_json::json!({
        "layer": "transparent-overlay",
        "libmpv": "not-linked (S1b blocked on libmpv DLL)",
        "platform": std::env::consts::OS
    })
}

fn main() {
    tauri::Builder::default()
        .invoke_handler(tauri::generate_handler![overlay_ping, surface_status])
        .run(tauri::generate_context!())
        .expect("failed to run tauri app");
}
