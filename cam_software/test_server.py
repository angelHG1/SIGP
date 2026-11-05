#!/usr/bin/env python3
"""
Parqueame - Servidor de prueba para cámara ESP32-CAM / XIAO ESP32S3
Recibe fotos en: POST /api/v1/upload/{camera_id}
Guarda las imágenes en ./fotos/ y ofrece una galería web en http://localhost:8000

No requiere librerías externas (usa exclusivamente la librería estándar de Python).
"""

import argparse
from datetime import datetime
import email
from email.parser import BytesFeedParser
import json
import mimetypes
import os
from pathlib import Path
from urllib.parse import parse_qs, urlparse
from http.server import HTTPServer, BaseHTTPRequestHandler

FOTOS_DIR = Path(__file__).resolve().parent / "fotos"


class CameraUploadHandler(BaseHTTPRequestHandler):
    fotos_dir = FOTOS_DIR

    def log_message(self, format, *args):
        # Silenciamos el log por defecto para imprimir mensajes personalizados limpios
        pass

    def do_GET(self):
        parsed = urlparse(self.path)
        path = parsed.path

        # Servir archivos de la carpeta fotos: /fotos/<nombre>
        if path.startswith("/fotos/"):
            filename = os.path.basename(path)
            file_path = self.fotos_dir / filename
            if file_path.exists() and file_path.is_file():
                self.send_response(200)
                content_type, _ = mimetypes.guess_type(str(file_path))
                self.send_header("Content-Type", content_type or "image/jpeg")
                self.send_header("Content-Length", str(file_path.stat().st_size))
                self.send_header("Cache-Control", "no-cache")
                self.end_headers()
                with open(file_path, "rb") as f:
                    self.wfile.write(f.read())
                return
            else:
                self.send_error(404, "Foto no encontrada")
                return

        # Dashboard / Galería web en /
        if path == "/" or path == "/index.html":
            self.serve_gallery()
            return

        # Endpoint de salud
        if path == "/health" or path == "/api/v1/health":
            self.send_response(200)
            self.send_header("Content-Type", "application/json")
            self.end_headers()
            self.wfile.write(json.dumps({"status": "ok", "server": "Parqueame Test Server"}).encode())
            return

        self.send_error(404, "Ruta no encontrada")

    def do_POST(self):
        parsed = urlparse(self.path)
        path = parsed.path

        # Esperamos ruta: /api/v1/upload/{camera_id}
        prefix = "/api/v1/upload/"
        if not path.startswith(prefix):
            self.send_error(404, f"Endpoint inválido. Usar {prefix}<camera_id>")
            return

        camera_id = path[len(prefix):].strip("/ ")
        if not camera_id:
            camera_id = "unknown_camera"

        query_params = parse_qs(parsed.query)
        battery_pct = query_params.get("battery", [None])[0]
        voltage = query_params.get("voltage", [None])[0]
        device_token = self.headers.get("X-Device-Token", None)

        content_type = self.headers.get("Content-Type", "")
        content_length = int(self.headers.get("Content-Length", 0))

        if not content_type.startswith("multipart/form-data") or content_length <= 0:
            self.send_response(400)
            self.send_header("Content-Type", "application/json")
            self.end_headers()
            self.wfile.write(json.dumps({"error": "Content-Type debe ser multipart/form-data con cuerpo válido"}).encode())
            return

        body = self.rfile.read(content_length)

        # Parsear multipart/form-data usando email.parser
        parser = BytesFeedParser()
        parser.feed(b"Content-Type: " + content_type.encode() + b"\r\n\r\n" + body)
        msg = parser.close()

        image_data = None
        for part in msg.walk():
            filename = part.get_filename()
            name = part.get_param("name", header="content-disposition")
            if name == "file" or (filename and filename.lower().endswith((".jpg", ".jpeg", ".png"))):
                image_data = part.get_payload(decode=True)
                break

        if not image_data:
            # Respaldo: si el multipart vino plano o con formato simple, buscar bytes JPEG (\xFF\xD8 ... \xFF\xD9)
            start = body.find(b"\xff\xd8")
            end = body.rfind(b"\xff\xd9")
            if start != -1 and end != -1 and end > start:
                image_data = body[start : end + 2]

        if not image_data:
            print(f"[ERROR] No se pudo extraer la imagen del payload de '{camera_id}' ({content_length} bytes)")
            self.send_response(422)
            self.send_header("Content-Type", "application/json")
            self.end_headers()
            self.wfile.write(json.dumps({"error": "No se encontró archivo de imagen en la petición"}).encode())
            return

        # Asegurar directorio ./fotos
        self.fotos_dir.mkdir(parents=True, exist_ok=True)

        # Guardar imagen con marca de tiempo
        now_str = datetime.now().strftime("%Y%m%d_%H%M%S")
        saved_filename = f"{camera_id}_{now_str}.jpg"
        saved_path = self.fotos_dir / saved_filename

        with open(saved_path, "wb") as f:
            f.write(image_data)

        # Actualizar copia 'latest'
        latest_filename = f"{camera_id}_latest.jpg"
        latest_path = self.fotos_dir / latest_filename
        with open(latest_path, "wb") as f:
            f.write(image_data)

        # Registro en consola
        info_bat = f" | Batería: {battery_pct}% ({voltage}V)" if battery_pct else ""
        info_tok = f" | Token: {device_token}" if device_token else ""
        print(f"[{datetime.now().strftime('%H:%M:%S')}] [FOTO RECIBIDA] Cámara: '{camera_id}' | {len(image_data):,} bytes -> {saved_filename}{info_bat}{info_tok}")

        response = {
            "status": "ok",
            "message": "Foto guardada exitosamente",
            "camera_id": camera_id,
            "filename": saved_filename,
            "size_bytes": len(image_data),
            "battery_pct": battery_pct,
            "voltage": voltage,
        }
        self.send_response(200)
        self.send_header("Content-Type", "application/json")
        self.end_headers()
        self.wfile.write(json.dumps(response).encode())

    def serve_gallery(self):
        self.fotos_dir.mkdir(parents=True, exist_ok=True)
        files = sorted(
            [f for f in self.fotos_dir.glob("*.jpg") if not f.name.endswith("_latest.jpg")],
            key=lambda x: x.stat().st_mtime,
            reverse=True,
        )

        items_html = ""
        for f in files[:30]:  # Mostrar últimas 30
            mtime = datetime.fromtimestamp(f.stat().st_mtime).strftime("%Y-%m-%d %H:%M:%S")
            size_kb = f.stat().st_size / 1024
            items_html += f"""
            <div style="background:#1e293b; border-radius:8px; overflow:hidden; box-shadow:0 4px 6px -1px rgba(0,0,0,0.3);">
                <a href="/fotos/{f.name}" target="_blank">
                    <img src="/fotos/{f.name}" style="width:100%; height:200px; object-fit:cover; display:block;" loading="lazy">
                </a>
                <div style="padding:10px; font-size:13px; color:#cbd5e1;">
                    <strong style="color:#38bdf8;">{f.name}</strong><br>
                    <span>{mtime} &bull; {size_kb:.1f} KB</span>
                </div>
            </div>
            """

        if not items_html:
            items_html = "<p style='color:#94a3b8; grid-column:1/-1; text-align:center;'>Aún no se han recibido fotos. Cuando la cámara envíe una foto aparecerá aquí automáticamente.</p>"

        html = f"""<!DOCTYPE html>
<html lang="es">
<head>
    <meta charset="utf-8">
    <meta http-equiv="refresh" content="5">
    <title>Parqueame - Servidor de Prueba</title>
    <style>
        body {{ font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, sans-serif; background:#0f172a; color:#f8fafc; margin:0; padding:20px; }}
        h1 {{ margin:0 0 8px 0; color:#38bdf8; font-size:24px; }}
        .header {{ display:flex; justify-content:space-between; align-items:center; border-bottom:1px solid #334155; padding-bottom:14px; margin-bottom:20px; }}
        .badge {{ background:#059669; padding:4px 10px; border-radius:12px; font-size:12px; font-weight:bold; }}
        .grid {{ display:grid; grid-template-columns:repeat(auto-fill, minmax(260px, 1fr)); gap:16px; }}
    </style>
</head>
<body>
    <div class="header">
        <div>
            <h1>Parqueame &bull; Receptor de Fotos de Cámara</h1>
            <span style="color:#94a3b8; font-size:13px;">Auto-recarga cada 5 segundos | Directorio: <code>{self.fotos_dir}</code></span>
        </div>
        <span class="badge">ONLINE - {len(files)} fotos</span>
    </div>
    <div class="grid">
        {items_html}
    </div>
</body>
</html>"""

        self.send_response(200)
        self.send_header("Content-Type", "text/html; charset=utf-8")
        self.end_headers()
        self.wfile.write(html.encode("utf-8"))


def run(host="0.0.0.0", port=8000, fotos_dir=None):
    if fotos_dir:
        CameraUploadHandler.fotos_dir = Path(fotos_dir).resolve()
    CameraUploadHandler.fotos_dir.mkdir(parents=True, exist_ok=True)

    server = HTTPServer((host, port), CameraUploadHandler)
    print("=" * 60)
    print("  PARQUEAME - Servidor de prueba para fotos de cámara")
    print(f"  Escuchando en: http://{host}:{port}")
    print(f"  Endpoint POST: http://{host}:{port}/api/v1/upload/<camera_id>")
    print(f"  Directorio:    {CameraUploadHandler.fotos_dir}")
    print(f"  Galería web:   http://localhost:{port}")
    print("=" * 60)
    print("Esperando conexiones de la cámara... (Presiona Ctrl+C para detener)")

    try:
        server.serve_forever()
    except KeyboardInterrupt:
        print("\nServidor detenido.")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Servidor de prueba Parqueame")
    parser.add_argument("--host", default="0.0.0.0", help="Host/IP de escucha (default: 0.0.0.0)")
    parser.add_argument("--port", type=int, default=8000, help="Puerto (default: 8000)")
    parser.add_argument("--dir", default=str(FOTOS_DIR), help="Directorio donde guardar fotos")
    args = parser.parse_args()

    run(host=args.host, port=args.port, fotos_dir=args.dir)
