
import cStringIO
import socket

from eventlet import api, wsgi

def fixup(parser):
    env = parser.environ
    for key in env.keys():
        if key.startswith('HTTP_'):
            env[key.split(':')[0].upper().replace('-', '_')] = env.pop(key)
    body = env.pop('REQUEST_BODY', '')
    cl = env.pop('HTTP_CONTENT_LENGTH', '0')
    env['CONTENT_LENGTH'] = cl
    env['wsgi.input'] = cStringIO.StringIO(body)
    return env


import pyhttp11
h = pyhttp11.HttpParser()

print h.execute("POST http://foo.com/ HTTP/1.1\r\nHost: foo.com\r\nContent-length: 12\r\n\r\nHello, World")
print h.has_error()
print h.is_finished()
fixup(h)
print h.environ


class MongrelProtocol(wsgi.HttpProtocol):
    def handle_one_request(self):
        if self.server.max_http_version:
            self.protocol_version = self.server.max_http_version

        buff = ''
        while True:
            try:
                buff += self.request.recv(4096)
                h.reset()
                h.execute(buff)
                if not h.has_error() and h.is_finished():
                    break
            except socket.error, e:
                self.close_connection = 1
                return

        self.environ = fixup(h)
        self.requestline = "%s %s %s" % (
            self.environ['REQUEST_METHOD'],
            self.environ['REQUEST_URI'],
            self.environ['HTTP_VERSION'])
        self.application = self.server.app

        try:
            self.server.outstanding_requests += 1
            try:
                self.handle_one_response()
            except socket.error, e:
                # Broken pipe, connection reset by peer
                if e[0] in (32, 54):
                    pass
                else:
                    raise
        finally:
            self.server.outstanding_requests -= 1

def app(env, start_response):
    start_response('200 OK', [('Content-type', 'text/plain')])
    return ["Hello, world\r\n"]


wsgi.server(api.tcp_listener(('', 8889)), app, file('/dev/null', 'w'), protocol=MongrelProtocol)
#wsgi.server(api.tcp_listener(('', 8889)), app, file('/dev/null', 'w'))
