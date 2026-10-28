"""Local interactive preview of the real portable C engine/UI; no calculator ROM.

Requires a host C compiler (CC or --cc). Preview progress is session-only.
No commercial game files or personal saves are read.
"""
import argparse
import ctypes
from http.server import BaseHTTPRequestHandler, HTTPServer
import json
from pathlib import Path
import struct
import subprocess
import sys
import time
import zlib

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools"))
from run_tests import compiler_command


def load_library(compiler=None):
    build = ROOT / "build"
    build.mkdir(exist_ok=True)
    library = build / ("preview.dll" if sys.platform == "win32" else "preview.so")
    command = compiler_command(compiler) + ["-std=c99", "-shared", "-O2", "-DCC_HOST", "-Isrc", "preview/bridge.c", "src/game.c", "src/ui.c", "src/render.c", "-o", str(library)]
    if sys.platform != "win32":
        command += ["-fPIC", "-lm"]
    subprocess.run(command, cwd=ROOT, check=True)
    lib = ctypes.CDLL(str(library))
    lib.preview_frame.restype = ctypes.POINTER(ctypes.c_ubyte)
    lib.preview_palette.restype = ctypes.POINTER(ctypes.c_ubyte)
    lib.preview_init.argtypes = [ctypes.c_uint32]
    lib.preview_key.argtypes = [ctypes.c_uint8]
    lib.preview_tick.argtypes = [ctypes.c_uint32]
    lib.preview_init(int(time.time()) & 0xFFFFFFFF)
    return lib


def png(lib):
    frame = bytes(lib.preview_frame()[:320*240])
    palette = bytes(lib.preview_palette()[:14*3])
    rows = b"".join(b"\0" + frame[y*320:(y+1)*320] for y in range(240))
    def chunk(kind, data):
        return struct.pack(">I", len(data)) + kind + data + struct.pack(">I", zlib.crc32(kind+data))
    return b"\x89PNG\r\n\x1a\n" + chunk(b"IHDR", struct.pack(">IIBBBBB", 320, 240, 8, 3, 0, 0, 0)) + chunk(b"PLTE", palette) + chunk(b"IDAT", zlib.compress(rows)) + chunk(b"IEND", b"")


HTML = """<!doctype html><html><head><meta charset='utf-8'><title>Crooked Coin CE — C Preview</title><style>
*{box-sizing:border-box}body{margin:0;background:#091d22;color:#edf4e1;font:16px system-ui}main{max-width:850px;margin:45px auto;padding:0 20px}h1{font-size:28px;font-weight:650;margin-bottom:8px}p{color:#88a8a6;line-height:1.5}img{width:640px;max-width:100%;image-rendering:pixelated;display:block;border:2px solid #1f4c51;box-shadow:0 25px 90px #030f13;margin:30px 0 22px}button{background:#14383e;color:#edf4e1;border:1px solid #2b7374;padding:12px 15px;border-radius:5px;font-size:14px;cursor:pointer}button:hover{border-color:#e2aa37}nav{display:flex;gap:8px;flex-wrap:wrap}.gold{color:#e2aa37}small{color:#88a8a6}kbd{font:inherit;color:#85dabf}footer{margin-top:24px}
</style></head><body><main><small class='gold'>TI-84 PLUS CE · 320 × 240 · NATIVE C</small><h1>Crooked Coin CE</h1><p>The actual C engine and renderer, running on your computer. Keyboard: <kbd>Enter</kbd> flip, <kbd>arrows</kbd> navigate, <kbd>Escape</kbd> back. Use the buttons for calculator function keys.</p><img id='screen' alt='Live Crooked Coin CE screen'><nav>
<button data-k='1'>Enter / 2nd</button><button data-k='2'>Clear</button><button data-k='7'>Y= · Shop</button><button data-k='8'>Window · Stats</button><button data-k='9'>Zoom · Badges</button><button data-k='10'>Trace · Help</button><button data-k='11'>Graph · Menu</button><button data-k='3'>↑</button><button data-k='4'>↓</button><button data-k='5'>←</button><button data-k='6'>→</button></nav><footer><small>Host preview · session-only progress · no calculator or CEmu execution claimed.<br>Original game: Unfair Flips by Heather Flowers. Independent adaptation with original art and writing.</small></footer></main><script>
let busy=false;async function key(k){await fetch('/key',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify({key:k})})}document.querySelectorAll('button').forEach(b=>b.onclick=()=>key(+b.dataset.k));const map={Enter:1,' ':1,Escape:2,ArrowUp:3,ArrowDown:4,ArrowLeft:5,ArrowRight:6,s:7,t:8,a:9,h:10,m:11,f:12};onkeydown=e=>{if(map[e.key]&&!e.repeat){e.preventDefault();key(map[e.key])}};async function frame(){if(busy)return;busy=true;try{let r=await fetch('/frame');let u=URL.createObjectURL(await r.blob());let i=document.getElementById('screen');let prev=i.src;i.src=u;if(prev.startsWith('blob:'))URL.revokeObjectURL(prev)}finally{busy=false}}setInterval(frame,80);frame();
</script></body></html>"""


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--cc")
    parser.add_argument("--port", type=int, default=8766)
    parser.add_argument("--screenshots", action="store_true", help="render labeled setup fixtures, then exit")
    args = parser.parse_args()
    lib = load_library(args.cc)
    if args.screenshots:
        folder = ROOT / "docs" / "screenshots"
        folder.mkdir(parents=True, exist_ok=True)
        names = ["play", "shop", "statistics", "achievements", "ending-edge", "ending-visitor", "ending-sky", "ending-shatter", "ending-ten", "april"]
        for i, name in enumerate(names):
            lib.preview_fixture(i)
            (folder / (name+".png")).write_bytes(png(lib))
        print("Rendered 10 C UI setup fixtures; these are HOST screenshots, not emulator captures.")
        return
    class Handler(BaseHTTPRequestHandler):
        last = time.monotonic()
        def log_message(self, *args):
            pass
        def respond(self, data, kind):
            self.send_response(200)
            self.send_header("Content-Type", kind)
            self.send_header("Cache-Control", "no-store")
            self.send_header("Content-Length", str(len(data)))
            self.end_headers()
            self.wfile.write(data)
        def advance(self):
            now = time.monotonic()
            lib.preview_tick(min(1000, int((now-Handler.last)*1000)))
            Handler.last = now
        def do_GET(self):
            if self.path == "/frame":
                self.advance()
                self.respond(png(lib), "image/png")
            elif self.path == "/":
                self.respond(HTML.encode(), "text/html; charset=utf-8")
            else:
                self.send_error(404)
        def do_POST(self):
            if self.path != "/key":
                self.send_error(404); return
            self.advance()
            try:
                data=json.loads(self.rfile.read(min(1024,int(self.headers.get("Content-Length", "0")))))
                key=int(data["key"])
                if not 0 <= key <= 12: raise ValueError()
                lib.preview_key(key)
            except (ValueError,KeyError,TypeError):
                self.send_error(400); return
            self.respond(b"{}", "application/json")
    print(f"Preview: http://127.0.0.1:{args.port} (session-only progress)", flush=True)
    HTTPServer(("127.0.0.1",args.port), Handler).serve_forever()


if __name__ == "__main__":
    main()
