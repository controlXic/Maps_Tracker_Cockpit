import http.server
import socketserver
import os
import cgi

PORT = 5000
UPLOAD_DIR = "GPX"
os.makedirs(UPLOAD_DIR, exist_ok=True)

class FileServer(http.server.SimpleHTTPRequestHandler):
    def do_POST(self):
        if self.path == "/GPX":
            form = cgi.FieldStorage(
                fp=self.rfile,
                headers=self.headers,
                environ={
                    "REQUEST_METHOD": "POST",
                    "CONTENT_TYPE": self.headers["Content-Type"],
                }
            )
            file_item = form["file"]
            if file_item.filename:
                filepath = os.path.join(UPLOAD_DIR, os.path.basename(file_item.filename))
                with open(filepath, "wb") as f:
                    f.write(file_item.file.read())
                self.send_response(303)
                self.send_header("Location", "/")
                self.end_headers()
            else:
                self.send_error(400, "No se recibió ningún archivo")
        else:
            self.send_error(404, "Ruta no encontrada")

# Cambiamos a ThreadingHTTPServer para evitar bloqueos
with http.server.ThreadingHTTPServer(("", PORT), FileServer) as httpd:
    print(f"Servidor iniciado en http://0.0.0.0:{PORT}")
    httpd.serve_forever()
