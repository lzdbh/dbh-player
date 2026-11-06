//! P0 S3 spike: minimal HTTP 206 Range media server.
//!
//! Verifies the DLNA cast critical path: TV pulls bytes from PC with Range,
//! PC must answer 206 + Content-Range, or seeking breaks.
//!
//! Run: cargo run -- <absolute-file-path> <token> [port]
//! This is throwaway validation code; production MediaServer lives in src-tauri.

use axum::{
    body::Body,
    extract::{Path, Query, State},
    http::{header, HeaderMap, StatusCode},
    response::Response,
    routing::get,
    Router,
};
use std::{collections::HashMap, path::PathBuf, sync::Arc};
use tokio::{
    fs::File,
    io::{AsyncReadExt, AsyncSeekExt, SeekFrom},
};
use tokio_util::io::ReaderStream;

struct AppState {
    /// media_id -> absolute path. Clients NEVER pass a raw path.
    files: HashMap<String, PathBuf>,
    token: String,
}

fn text_response(status: StatusCode, msg: &str) -> Response {
    Response::builder()
        .status(status)
        .header(header::CONTENT_TYPE, "text/plain; charset=utf-8")
        .body(Body::from(msg.to_string()))
        .unwrap()
}

fn mime_for(path: &PathBuf) -> &'static str {
    match path
        .extension()
        .and_then(|e| e.to_str())
        .unwrap_or("")
        .to_lowercase()
        .as_str()
    {
        "mkv" => "video/x-matroska",
        "mp4" | "m4v" => "video/mp4",
        "ts" => "video/mp2t",
        "avi" => "video/x-msvideo",
        "webm" => "video/webm",
        "srt" => "text/plain; charset=utf-8",
        "vtt" => "text/vtt; charset=utf-8",
        _ => "application/octet-stream",
    }
}

/// Parses a single-range header. Returns None when the supplied range is invalid.
/// Multi-range is intentionally unsupported (TV renderers use single ranges).
fn parse_range(value: &str, len: u64) -> Option<(u64, u64)> {
    let spec = value.strip_prefix("bytes=")?;
    let (start_s, end_s) = spec.split_once('-')?;
    if spec.contains(',') {
        return None;
    }
    if start_s.is_empty() {
        let suffix: u64 = end_s.parse().ok()?;
        if suffix == 0 || len == 0 {
            return None;
        }
        let start = len.saturating_sub(suffix);
        Some((start, len - 1))
    } else {
        let start: u64 = start_s.parse().ok()?;
        if len == 0 || start >= len {
            return None;
        }
        let end = if end_s.is_empty() {
            len - 1
        } else {
            end_s.parse::<u64>().ok()?.min(len - 1)
        };
        if start > end {
            None
        } else {
            Some((start, end))
        }
    }
}

async fn serve(
    Path(id): Path<String>,
    Query(query): Query<HashMap<String, String>>,
    State(state): State<Arc<AppState>>,
    headers: HeaderMap,
) -> Response {
    if query.get("token").map(String::as_str) != Some(state.token.as_str()) {
        return text_response(StatusCode::FORBIDDEN, "invalid token");
    }
    let path = match state.files.get(&id) {
        Some(p) => p,
        None => return text_response(StatusCode::NOT_FOUND, "unknown media id"),
    };

    let len = match tokio::fs::metadata(path).await {
        Ok(m) if m.is_file() => m.len(),
        _ => return text_response(StatusCode::NOT_FOUND, "media file missing"),
    };
    let mime = mime_for(path);

    let range_header = headers
        .get(header::RANGE)
        .and_then(|v| v.to_str().ok())
        .map(str::to_owned);

    let (status, start, end, content_range) = match range_header {
        None => (StatusCode::OK, 0, len.saturating_sub(1), None),
        Some(ref r) => match parse_range(r, len) {
            Some((s, e)) => (
                StatusCode::PARTIAL_CONTENT,
                s,
                e,
                Some(format!("bytes {}-{}/{}", s, e, len)),
            ),
            None => {
                return Response::builder()
                    .status(StatusCode::RANGE_NOT_SATISFIABLE)
                    .header(header::CONTENT_RANGE, format!("bytes */{}", len))
                    .header(header::ACCEPT_RANGES, "bytes")
                    .body(Body::empty())
                    .unwrap();
            }
        },
    };

    let mut file = match File::open(path).await {
        Ok(f) => f,
        Err(_) => return text_response(StatusCode::INTERNAL_SERVER_ERROR, "open failed"),
    };
    if start > 0 && file.seek(SeekFrom::Start(start)).await.is_err() {
        return text_response(StatusCode::INTERNAL_SERVER_ERROR, "seek failed");
    }
    let take = end - start + 1;
    let stream = ReaderStream::new(file.take(take));

    let mut builder = Response::builder()
        .status(status)
        .header(header::CONTENT_TYPE, mime)
        .header(header::ACCEPT_RANGES, "bytes")
        .header(header::CONTENT_LENGTH, take);
    if let Some(cr) = content_range {
        builder = builder.header(header::CONTENT_RANGE, cr);
    }
    builder.body(Body::from_stream(stream)).unwrap()
}

async fn health(
    Query(query): Query<HashMap<String, String>>,
    State(state): State<Arc<AppState>>,
) -> Response {
    if query.get("token").map(String::as_str) == Some(state.token.as_str()) {
        text_response(StatusCode::OK, "ok")
    } else {
        text_response(StatusCode::FORBIDDEN, "invalid token")
    }
}

#[tokio::main]
async fn main() {
    let args: Vec<String> = std::env::args().collect();
    let file = args
        .get(1)
        .cloned()
        .unwrap_or_else(|| "spike/fixtures/sample.bin".to_string());
    let token = args.get(2).cloned().unwrap_or_else(|| "dev-token".to_string());
    let port: u16 = args
        .get(3)
        .and_then(|p| p.parse().ok())
        .unwrap_or(18765);

    let mut files = HashMap::new();
    files.insert("sample".to_string(), PathBuf::from(&file));
    let state = Arc::new(AppState { files, token: token.clone() });

    let app = Router::new()
        .route("/media/{id}", get(serve))
        .route("/health", get(health))
        .with_state(state);

    // Spike binds loopback. Production binds the LAN address and filters by paired device IP.
    let addr = format!("127.0.0.1:{}", port);
    let listener = tokio::net::TcpListener::bind(&addr).await.unwrap();
    println!("S3 spike listening on http://{}", addr);
    println!("  media: /media/sample?token={}", token);
    axum::serve(listener, app).await.unwrap();
}
