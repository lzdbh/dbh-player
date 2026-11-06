#!/usr/bin/env python3
"""P0 cast-loop spike: verify the DLNA protocol chain without a real TV.

A minimal UPnP AVTransport MediaRenderer stands in for an Android TV, and a
controller stands in for the PC. The renderer really pulls bytes from the S3
media server with HTTP Range, so the 206 path is exercised end to end.

    python cast_loop_test.py --media-server ../media-server/target/debug/cast-media-spike

Discovery here uses a known renderer URL (the manual-IP fallback path), because
UDP 1900 is occupied by the Windows SSDP service on this machine. Real SSDP
M-SEARCH and TV vendor SOAP quirks are NOT covered by this test.
"""

from __future__ import annotations

import argparse
import os
import re
import subprocess
import sys
import tempfile
import threading
import time
import urllib.error
import urllib.request
import xml.etree.ElementTree as ET
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer

SERVICE = "urn:schemas-upnp-org:service:AVTransport:1"


def tag(xml: str, name: str) -> str | None:
    m = re.search(
        r"<(?:[A-Za-z0-9]+:)?%s[^>]*>(.*?)</(?:[A-Za-z0-9]+:)?%s>" % (name, name),
        xml,
        re.S,
    )
    return m.group(1).strip() if m else None


def soap_envelope(action: str, body: str) -> str:
    return (
        '<?xml version="1.0"?>'
        '<s:Envelope xmlns:s="http://schemas.xmlsoap.org/soap/envelope/" '
        's:encodingStyle="http://schemas.xmlsoap.org/soap/encoding/">'
        "<s:Body>"
        f'<u:{action} xmlns:u="{SERVICE}">{body}</u:{action}>'
        "</s:Body></s:Envelope>"
    )


def soap_response(action: str, body: str) -> str:
    return (
        '<?xml version="1.0"?>'
        '<s:Envelope xmlns:s="http://schemas.xmlsoap.org/soap/envelope/" '
        's:encodingStyle="http://schemas.xmlsoap.org/soap/encoding/">'
        "<s:Body>"
        f'<u:{action}Response xmlns:u="{SERVICE}">{body}</u:{action}Response>'
        "</s:Body></s:Envelope>"
    )


DESCRIPTION = """<?xml version="1.0"?>
<root xmlns="urn:schemas-upnp-org:device-1-0">
  <specVersion><major>1</major><minor>0</minor></specVersion>
  <device>
    <deviceType>urn:schemas-upnp-org:device:MediaRenderer:1</deviceType>
    <friendlyName>Spike Renderer (simulated TV)</friendlyName>
    <manufacturer>dbhx-player spike</manufacturer>
    <modelName>P0 Simulated Renderer</modelName>
    <UDN>uuid:dbhx-player-spike-renderer-1</UDN>
    <serviceList>
      <service>
        <serviceType>urn:schemas-upnp-org:service:AVTransport:1</serviceType>
        <serviceId>urn:upnp-org:serviceId:AVTransport</serviceId>
        <SCPDURL>/AVTransport/scpd.xml</SCPDURL>
        <controlURL>/AVTransport/control</controlURL>
        <eventSubURL>/AVTransport/event</eventSubURL>
      </service>
    </serviceList>
  </device>
</root>"""

SCPD = """<?xml version="1.0"?>
<scpd xmlns="urn:schemas-upnp-org:service-1-0">
  <specVersion><major>1</major><minor>0</minor></specVersion>
  <actionList>
    <action><name>SetAVTransportURI</name></action>
    <action><name>Play</name></action>
    <action><name>Pause</name></action>
    <action><name>Seek</name></action>
    <action><name>GetPositionInfo</name></action>
  </actionList>
  <serviceStateTable>
    <stateVariable sendEvents="no"><name>AVTransportURI</name><dataType>string</dataType></stateVariable>
    <stateVariable sendEvents="no"><name>TransportState</name><dataType>string</dataType></stateVariable>
  </serviceStateTable>
</scpd>"""


class Renderer:
    def __init__(self, duration: int, total_bytes: int):
        self.state = {
            "uri": None,
            "transport": "STOPPED",
            "position": 0,
            "duration": duration,
            "total_bytes": total_bytes,
            "pulls": [],
            "actions": [],
        }

    def pull(self, position: int) -> dict:
        """TV pulls bytes from the PC media server at the requested position."""
        start = int(self.state["total_bytes"] * position / self.state["duration"])
        end = min(start + 99, self.state["total_bytes"] - 1)
        req = urllib.request.Request(
            self.state["uri"], headers={"Range": f"bytes={start}-{end}"}
        )
        with urllib.request.urlopen(req, timeout=8) as resp:
            data = resp.read()
            result = {
                "position": position,
                "range": f"bytes={start}-{end}",
                "status": resp.status,
                "content_range": resp.headers.get("Content-Range"),
                "bytes": len(data),
            }
        self.state["pulls"].append(result)
        return result

    def handle(self, action: str, body: str) -> str:
        self.state["actions"].append(action)
        if action == "SetAVTransportURI":
            self.state["uri"] = tag(body, "CurrentURI")
            self.state["transport"] = "STOPPED"
            return soap_response(action, "")
        if action == "Play":
            self.state["transport"] = "PLAYING"
            if self.state["uri"]:
                self.pull(self.state["position"])
            return soap_response(action, "")
        if action == "Pause":
            self.state["transport"] = "PAUSED"
            return soap_response(action, "")
        if action == "Seek":
            target = tag(body, "Target") or "00:00:00"
            parts = [int(p) for p in target.split(":")]
            seconds = parts[0] * 3600 + parts[1] * 60 + parts[2] if len(parts) == 3 else 0
            self.state["position"] = seconds
            if self.state["uri"]:
                self.pull(seconds)  # seek must continue at the NEW byte offset
            return soap_response(action, "")
        if action == "GetPositionInfo":
            p = self.state["position"]
            h, rem = divmod(p, 3600)
            m, s = divmod(rem, 60)
            d = self.state["duration"]
            dh, drem = divmod(d, 3600)
            dm, ds = divmod(drem, 60)
            rel = f"{h:02d}:{m:02d}:{s:02d}"
            dur = f"{dh:02d}:{dm:02d}:{ds:02d}"
            return soap_response(
                action,
                f"<TrackDuration>{dur}</TrackDuration><RelTime>{rel}</RelTime>"
                "<TransportState>PLAYING</TransportState>",
            )
        raise ValueError(f"unsupported action: {action}")


def make_handler(renderer: Renderer):
    class Handler(BaseHTTPRequestHandler):
        def log_message(self, *_args):
            pass

        def _send(self, status: int, body: str, ctype: str = "text/xml; charset=utf-8"):
            raw = body.encode("utf-8")
            self.send_response(status)
            self.send_header("Content-Type", ctype)
            self.send_header("Content-Length", str(len(raw)))
            self.end_headers()
            self.wfile.write(raw)

        def do_GET(self):
            if self.path.startswith("/description.xml"):
                self._send(200, DESCRIPTION)
            elif self.path.startswith("/AVTransport/scpd.xml"):
                self._send(200, SCPD)
            elif self.path.startswith("/health"):
                self._send(200, "ok", "text/plain; charset=utf-8")
            else:
                self._send(404, "not found", "text/plain; charset=utf-8")

        def do_POST(self):
            if not self.path.startswith("/AVTransport/control"):
                self._send(404, "not found", "text/plain; charset=utf-8")
                return
            length = int(self.headers.get("Content-Length", "0"))
            body = self.rfile.read(length).decode("utf-8", "replace")
            soap_action = self.headers.get("SOAPACTION", "").strip('"')
            action = soap_action.split("#")[-1] or "GetPositionInfo"
            try:
                self._send(200, renderer.handle(action, body))
            except Exception as exc:  # surface SOAP faults to the test
                fault = (
                    '<?xml version="1.0"?>'
                    '<s:Envelope xmlns:s="http://schemas.xmlsoap.org/soap/envelope/">'
                    f"<s:Body><s:Fault><faultstring>{exc}</faultstring></s:Fault></s:Body></s:Envelope>"
                )
                self._send(500, fault)

    return Handler


def soap_call(url: str, action: str, body: str) -> str:
    req = urllib.request.Request(
        url,
        data=soap_envelope(action, body).encode("utf-8"),
        headers={
            "Content-Type": 'text/xml; charset="utf-8"',
            "SOAPACTION": f'"{SERVICE}#{action}"',
        },
        method="POST",
    )
    with urllib.request.urlopen(req, timeout=8) as resp:
        return resp.read().decode("utf-8", "replace")


def wait_health(url: str, timeout: float = 20.0) -> bool:
    deadline = time.time() + timeout
    while time.time() < deadline:
        try:
            with urllib.request.urlopen(url, timeout=1) as resp:
                if resp.status == 200:
                    return True
        except Exception:
            time.sleep(0.2)
    return False


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--media-server", required=True, help="path to cast-media-spike binary")
    ap.add_argument("--sample", default="", help="media file; a temp file is generated if omitted")
    ap.add_argument("--token", default="loop-token")
    ap.add_argument("--media-port", type=int, default=18766)
    ap.add_argument("--renderer-port", type=int, default=18800)
    args = ap.parse_args()

    binary = args.media_server
    if not os.path.exists(binary) and os.path.exists(binary + ".exe"):
        binary = binary + ".exe"
    if not os.path.exists(binary):
        print(f"FAIL: media server binary not found: {args.media_server}")
        return 1

    tmp = None
    sample = args.sample
    if not sample:
        tmp = tempfile.NamedTemporaryFile(delete=False, suffix=".mkv")
        tmp.write(os.urandom(524288))
        tmp.close()
        sample = tmp.name

    media_url = (
        f"http://127.0.0.1:{args.media_port}/media/sample?token={args.token}"
    )
    health = f"http://127.0.0.1:{args.media_port}/health?token={args.token}"
    proc = subprocess.Popen([binary, sample, args.token, str(args.media_port)])
    renderer = Renderer(duration=60, total_bytes=os.path.getsize(sample))
    server = ThreadingHTTPServer(("127.0.0.1", args.renderer_port), make_handler(renderer))
    thread = threading.Thread(target=server.serve_forever, daemon=True)
    results = []

    try:
        if not wait_health(health):
            print("FAIL: media server did not become healthy")
            return 1
        thread.start()
        base = f"http://127.0.0.1:{args.renderer_port}"
        if not wait_health(base + "/health"):
            print("FAIL: simulated renderer did not start")
            return 1

        # 1) discovery: fetch device description and parse the control URL
        with urllib.request.urlopen(base + "/description.xml", timeout=5) as resp:
            desc = resp.read().decode()
        root = ET.fromstring(desc)
        ns = {"u": "urn:schemas-upnp-org:device-1-0"}
        control_path = root.findtext(".//u:controlURL", namespaces=ns)
        scpd_path = root.findtext(".//u:SCPDURL", namespaces=ns)
        control_url = base + (control_path or "/AVTransport/control")
        results.append(("discover description.xml + controlURL", bool(control_path and scpd_path)))
        with urllib.request.urlopen(base + (scpd_path or "/AVTransport/scpd.xml"), timeout=5) as resp:
            scpd_ok = b"SetAVTransportURI" in resp.read()
        results.append(("fetch SCPD action list", scpd_ok))

        # 2) SetAVTransportURI: push a tokenized URL, not the pixels
        soap_call(
            control_url,
            "SetAVTransportURI",
            "<InstanceID>0</InstanceID>"
            f"<CurrentURI>{media_url}</CurrentURI><CurrentURIMetaData></CurrentURIMetaData>",
        )
        results.append(("SetAVTransportURI accepted", renderer.state["uri"] == media_url))

        # 3) Play -> TV pulls the first range from the PC media server
        soap_call(control_url, "Play", "<InstanceID>0</InstanceID><Speed>1</Speed>")
        first = renderer.state["pulls"][-1]
        results.append(
            ("Play -> TV GET Range returns 206", first["status"] == 206 and first["bytes"] == 100)
        )
        results.append(
            ("Play -> Content-Range present", bool(first["content_range"]))
        )

        # 4) GetPositionInfo polling
        pos_xml = soap_call(control_url, "GetPositionInfo", "<InstanceID>0</InstanceID>")
        results.append(("GetPositionInfo returns RelTime", tag(pos_xml, "RelTime") is not None))

        # 5) Seek -> TV must pull from the NEW byte offset and still get 206
        soap_call(control_url, "Seek", "<InstanceID>0</InstanceID><Unit>REL_TIME</Unit><Target>00:00:30</Target>")
        seek = renderer.state["pulls"][-1]
        results.append(
            ("Seek -> new offset 206", seek["status"] == 206 and seek["position"] == 30 and seek["bytes"] == 100)
        )
        results.append(
            ("Seek -> new Range differs from initial", seek["range"] != first["range"])
        )

        # 6) Pause
        soap_call(control_url, "Pause", "<InstanceID>0</InstanceID>")
        results.append(("Pause state", renderer.state["transport"] == "PAUSED"))
        results.append(
            ("Renderer action sequence", renderer.state["actions"][:3] == ["SetAVTransportURI", "Play", "GetPositionInfo"])
        )
    finally:
        server.shutdown()
        server.server_close()
        proc.terminate()
        try:
            proc.wait(timeout=5)
        except subprocess.TimeoutExpired:
            proc.kill()
        if tmp:
            os.unlink(tmp.name)

    print("=== P0 cast-loop (simulated DLNA renderer) ===")
    print(f"media server : {media_url}")
    print(f"renderer     : http://127.0.0.1:{args.renderer_port}/description.xml")
    for label, ok in results:
        print(f"  [{'PASS' if ok else 'FAIL'}] {label}")
    for pull in renderer.state["pulls"]:
        print(
            f"  pull @ {pull['position']}s  {pull['range']}  -> "
            f"{pull['status']} {pull['content_range']} ({pull['bytes']} bytes)"
        )
    failed = sum(1 for _, ok in results if not ok)
    print(f"=== {len(results) - failed}/{len(results)} passed ===")
    print("NOTE: simulated renderer, NOT a real TV. SSDP multicast and vendor SOAP quirks are untested.")
    return 1 if failed else 0


if __name__ == "__main__":
    sys.exit(main())
