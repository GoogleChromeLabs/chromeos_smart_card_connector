#!/usr/bin/env python
# Copyright 2015 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""
An example HTTP server that returns the correct CORS headers for using with the
libgit2 URL transport scheme.

The libgit2-demo allows you to use two alternate schemes: "pepper_http://" and
"pepper_https://" to communicate with remote git servers. These schemes will
communicate using the Pepper URLLoader interface instead of Pepper TCPSocket
interface. URLLoader is available to all Native Client applications, but
TCPSockets are only available to Chrome Apps.

These alternate schemes require the git server to respond with specific CORS
headers, which as of this writing, most git servers don't do. This server shows
gives an example of a CGI script that calls git http-backend, as well as
returning the correct CORS headers.

To use this script, run it in the directory you want to serve from. It must
contain a subdirectory "git" which contains one or more subdirectories for your
git repositories. e.g.

    serve_root/
    serve_root/git
    serve_root/git/my_repo1
    serve_root/git/my_repo2
    etc.

These git repositories can then be accessed via the following URLs:

    http://localhost:8080/git/my_repo1
    http://localhost:8080/git/my_repo2
    etc.

If you use these URLs in the libgit2-demo, it will use the Pepper socket API.
to use the URLLoader interface, you must instead use the alternate schemes:

    pepper_http://localhost:8080/git/my_repo1
    pepper_http://localhost:8080/git/my_repo2

"""

import os
import select
import subprocess
import urlparse
from SimpleHTTPServer import SimpleHTTPRequestHandler

class GetHandler(SimpleHTTPRequestHandler):
  def do_GET(self):
    if self.handle_git('GET'):
      return
    SimpleHTTPRequestHandler.do_GET(self)

  def do_HEAD(self):
    if self.handle_git('HEAD'):
      return
    SimpleHTTPRequestHandler.do_HEAD(self)

  def do_OPTIONS(self):
    self.send_response(200, 'Script output follows')
    self.send_header('Access-Control-Allow-Origin', '*')
    self.send_header('Access-Control-Allow-Methods', 'POST, GET, OPTIONS')
    self.send_header('Access-Control-Allow-Headers', 'content-type')
    self.end_headers()

  def do_POST(self):
    if self.handle_git('POST'):
      return
    SimpleHTTPRequestHandler.do_POST(self)

  def handle_git(self, method):
    parsed_path = urlparse.urlparse(self.path)
    if not parsed_path.path.startswith('/git/'):
      return False

    path_no_git = parsed_path.path[len('/git/'):]
    first_slash = path_no_git.find('/')
    if first_slash < 0:
      return False

    # Assume that all git projects are in /git/<PROJECT NAME>
    git_project_root = path_no_git[:first_slash]
    path_info = path_no_git[first_slash:]

    env = dict(os.environ)
    env['GIT_HTTP_EXPORT_ALL'] = '1'
    env['REQUEST_METHOD'] = method
    env['QUERY_STRING'] = parsed_path.query
    env['PATH_INFO'] = path_info
    env['GIT_PROJECT_ROOT'] = os.path.join(os.getcwd(), 'git', git_project_root)
    env['REMOTE_USER'] = 'git-user'

    if self.headers.typeheader is None:
        env['CONTENT_TYPE'] = self.headers.type
    else:
        env['CONTENT_TYPE'] = self.headers.typeheader
    length = self.headers.getheader('content-length')
    if length:
        env['CONTENT_LENGTH'] = length

    nbytes = 0
    if length is not None:
      nbytes = int(length)

    self.send_response(200, 'Script output follows')
    self.send_header('Access-Control-Allow-Origin', '*')

    # from CGIHTTPServer.CGIHTTPRequestHandler
    p = subprocess.Popen(['git', 'http-backend'],
                         stdin=subprocess.PIPE,
                         stdout=subprocess.PIPE,
                         stderr=subprocess.PIPE,
                         env=env)
    if method == "POST" and nbytes > 0:
        data = self.rfile.read(nbytes)
    else:
        data = None
    # throw away additional data [see bug #427345]
    while select.select([self.rfile._sock], [], [], 0)[0]:
        if not self.rfile._sock.recv(1):
            break
    stdout, stderr = p.communicate(data)
    self.wfile.write(stdout)
    if stderr:
        self.log_error('%s', stderr)
    p.stderr.close()
    p.stdout.close()
    status = p.returncode
    if status:
        self.log_error("CGI script exit status %#x", status)
    else:
        self.log_message("CGI script exited OK")
    return True


if __name__ == '__main__':
  from BaseHTTPServer import HTTPServer
  server = HTTPServer(('localhost', 8080), GetHandler)
  print 'Starting server, use <Ctrl-C> to stop'
  server.serve_forever()
