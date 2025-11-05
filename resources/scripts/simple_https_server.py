"""
Description: This script is a simple python web server which accepts a PUT request and writes the supplied data to a file.
There is a known issue with the signal module when trying to run this on Windows 7.  It has worked in various *nix environments.

This script is a combination of the following links:
https://gist.githubusercontent.com/codification/1393204/raw/3fd4a48d072ec8f7f453d146814cb6e7fc6a129f/server.py
http://www.acmesystems.it/python_httpserver
"""

import sys
import os
import ssl
import time
import json
import uuid
import hashlib
from threading import Thread
from itertools import count
from os import curdir, sep

if sys.version_info[0] < 3:
    from BaseHTTPServer import HTTPServer, BaseHTTPRequestHandler
else:
    from http.server import BaseHTTPRequestHandler, HTTPServer

_REQ_COUNTER = count(1)


class MyHandler(BaseHTTPRequestHandler):
    protocol_version = 'HTTP/1.1'

    # Static configuration (evaluated at class load; can be overridden via env before start)
    echo_mode = os.environ.get('PUT_ECHO_MODE', 'echo')  # echo | json | empty
    chunked = os.environ.get('PUT_CHUNKED', '0') in ('1', 'true', 'yes')
    chunk_size = max(1, int(os.environ.get('PUT_CHUNK_SIZE', '512')))
    inflate_kb = int(os.environ.get('PUT_INFLATE_RESPONSE_KB', '0'))
    add_hash = os.environ.get('PUT_ADD_HASH', '1') in ('1', 'true', 'yes')
    banner = os.environ.get('HTTPS_SERVER_BANNER', 'WiseConnect HTTPS Server')

    def _send_body(self, status, body=b'', headers=None, chunked=False):
        self.send_response(status)
        if headers:
            for k, v in headers.items():
                self.send_header(k, v)
        if chunked:
            self.send_header('Transfer-Encoding', 'chunked')
        else:
            self.send_header('Content-Length', str(len(body)))
        self.send_header('Connection', 'close')
        self.end_headers()
        if chunked:
            total = len(body)
            for i in range(0, total, self.chunk_size):
                chunk = body[i:i + self.chunk_size]
                try:
                    self.wfile.write((f"{len(chunk):X}\r\n").encode('ascii'))
                    self.wfile.write(chunk)
                    self.wfile.write(b"\r\n")
                except Exception as e:
                    print("Chunk write failed:", e)
                    break
            try:
                self.wfile.write(b"0\r\n\r\n")
            except Exception as e:
                print("Final chunk write failed:", e)
        else:
            if body:
                try:
                    self.wfile.write(body)
                except Exception as e:
                    print('Body write failed:', e)
        try:
            self.wfile.flush()
        except Exception:
            pass
        try:
            self.connection.shutdown(1)
        except Exception:
            pass
        self.close_connection = True

    def log_message(self, fmt, *args):  # Keep default style but route to print
        try:
            print("%s - " % self.client_address[0] + (fmt % args))
        except Exception:
            pass

    def do_POST(self):
        rid = f"POST-{next(_REQ_COUNTER)}-{uuid.uuid4().hex[:6]}"
        start = time.time()
        print('\n--------- REQUEST METHOD: ','POST----------')
        print(self.headers)
        try:
            length = int(self.headers.get('Content-Length', '0'))
        except ValueError:
            self.send_error(400, 'Bad Content-Length')
            return
        body = self.rfile.read(length) if length > 0 else b''
        print('POST: bytes read =', len(body))
        resp_obj = {'status': 'ok', 'received_bytes': len(body), 'request_id': rid}
        payload = json.dumps(resp_obj, separators=(',', ':')).encode('utf-8')
        self._send_body(200, payload, {'Content-Type': 'application/json; charset=utf-8'})
        print('-------------  POST SUCCESS  -------------- (%.2f ms)\n' % ((time.time() - start) * 1000.0))

    def do_PUT(self):
        rid = f"PUT-{next(_REQ_COUNTER)}-{uuid.uuid4().hex[:6]}"
        start = time.time()
        print('\n---------- REQUEST METHOD: ','PUT ------------')
        print(self.headers)
        if self.headers.get('Expect', '').lower() == '100-continue':
            self.send_response_only(100)
            self.end_headers()
        length_hdr = self.headers.get('Content-Length')
        if not length_hdr:
            self.send_error(411, 'Length Required')
            return
        try:
            length = int(length_hdr)
        except ValueError:
            self.send_error(400, 'Bad Content-Length')
            return
        body = self.rfile.read(length) if length > 0 else b''
        print('PUT: bytes read =', len(body))
        target_path = curdir + self.path
        existed = os.path.exists(target_path)
        try:
            parent = os.path.dirname(target_path)
            if parent and not os.path.isdir(parent):
                os.makedirs(parent, exist_ok=True)
            with open(target_path, 'wb') as dst:
                dst.write(body)
        except Exception as e:
            print('PUT Failed')
            print(e)
            self.send_error(500, 'Write failed')
            return
        status = 201 if not existed else 200

        mode = os.environ.get('PUT_ECHO_MODE', self.echo_mode)
        if mode == 'empty':
            resp = b''
            ctype = 'text/plain; charset=utf-8'
        elif mode == 'json':
            info = {
                'status': 'ok',
                'created': (status == 201),
                'bytes': len(body),
                'path': self.path,
                'request_id': rid,
            }
            if self.add_hash and body:
                try:
                    info['sha256'] = hashlib.sha256(body).hexdigest()
                except Exception:
                    pass
            resp = json.dumps(info, separators=(',', ':')).encode('utf-8')
            ctype = 'application/json; charset=utf-8'
        else:  # echo (default)
            resp = body
            ctype = self.headers.get('Content-Type', 'application/octet-stream')

        if self.inflate_kb > 0 and resp:
            multiplier = max(1, (self.inflate_kb * 1024) // len(resp))
            if multiplier > 1:
                resp = resp * multiplier
                print('[%s] Inflated response to %d bytes' % (rid, len(resp)))

        use_chunked = self.chunked or os.environ.get('PUT_CHUNKED', '0') in ('1', 'true', 'yes')
        if use_chunked:
            print('PUT: sending chunked response bytes =', len(resp))
        else:
            print('PUT: sending response bytes =', len(resp))

        self._send_body(status, resp, {'Content-Type': ctype, 'Server': self.banner}, chunked=use_chunked)
        print('-------------   PUT SUCCESS  -------------- (%.2f ms)\n' % ((time.time() - start) * 1000.0))

    def do_HEAD(self):
        self._send_body(200, b'', {'Content-Type': 'text/html'})

    def do_GET(self):
        rid = f"GET-{next(_REQ_COUNTER)}-{uuid.uuid4().hex[:6]}"
        print('\n----------- REQUEST METHOD: ','GET -------------\n')
        print(self.headers)
        if self.path in ('/', ''):
            banner = (self.banner + ' OK\n').encode('utf-8')
            self._send_body(200, banner, {'Content-Type': 'text/plain; charset=utf-8'})
            return
        mimetype = 'application/octet-stream'
        if self.path.endswith('.html'): mimetype = 'text/html'
        elif self.path.endswith('.jpg'): mimetype = 'image/jpg'
        elif self.path.endswith('.gif'): mimetype = 'image/gif'
        elif self.path.endswith('.js'): mimetype = 'application/javascript'
        elif self.path.endswith('.css'): mimetype = 'text/css'
        elif self.path.endswith('.mp3'): mimetype = 'text/plain'
        elif self.path.endswith('.txt'): mimetype = 'text/plain'
        target = curdir + sep + self.path
        try:
            with open(target, 'rb') as f:
                data = f.read()
            self._send_body(200, data, {'Content-Type': mimetype})
            print('--------------   GET SUCCESS  --------------\n')
        except IOError:
            self.send_error(404, 'File Not Found: %s' % self.path)
            print('[%s] GET 404 %s' % (rid, target))


def run_on(port):
    print("\nLaunching HTTPS server on port %i ..." % port)
    server_address = ('', port)
    httpd = HTTPServer(server_address, MyHandler)
    httpd.socket = ssl.wrap_socket(
        httpd.socket,
        certfile=os.environ.get('HTTPS_CERT_FILE', 'server-cert.pem'),
        keyfile=os.environ.get('HTTPS_KEY_FILE', 'server-key.pem'),
        server_side=True
    )
    print("Server successfully acquired the socket with port:", port)
    print("Press Ctrl+C to shut down the server and exit.")
    print("\nAwaiting New connection\n")
    httpd.serve_forever()


if __name__ == "__main__":
    if len(sys.argv) < 2:
        print('Usage: python simple_https_server.py <port1> [<port2> ...]')
        print('Env: PUT_ECHO_MODE=echo|json|empty PUT_CHUNKED=1 PUT_CHUNK_SIZE=256 PUT_INFLATE_RESPONSE_KB=8 PUT_ADD_HASH=1')
        sys.exit(1)
    ports = [int(arg) for arg in sys.argv[1:]]
    for port_number in ports:
        server = Thread(target=run_on, args=[port_number], daemon=True)
        server.start()
    print('Server threads started for ports:', ports)
    try:
        while True:
            time.sleep(1.0)
    except KeyboardInterrupt:
        print('Shutdown requested, exiting.')
