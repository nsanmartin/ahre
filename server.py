import http.server
import socketserver
from http import HTTPStatus

page = b"""
Hello:
<a href="link 1"> Link Nro 1</a>
<a href="link 2"> Link Nro 2</a>
<a href="link 2"> Link Nro 2</a>
"""

class Handler(http.server.SimpleHTTPRequestHandler):
    def do_GET(self):
        self.send_response(HTTPStatus.OK)
        self.end_headers()
        #self.wfile.write(b'Hello world')
        self.wfile.write(page)


httpd = socketserver.TCPServer(('', 8000), Handler)
httpd.serve_forever()


# python3 server.py
# 127.0.0.1 - - [11/Apr/2017 11:36:49] "GET / HTTP/1.1" 200 -
# http :8000
'''
HTTP/1.0 200 OK
Date: Tue, 11 Apr 2017 15:36:49 GMT
Server: SimpleHTTP/0.6 Python/3.5.2
Hello world
'''
