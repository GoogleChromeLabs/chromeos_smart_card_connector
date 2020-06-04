#!/usr/bin/env python
# Copyright (c) 2011 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
"""A tiny web server.

This is intended to be used for testing.
"""

import BaseHTTPServer
import logging
import os
import SimpleHTTPServer
import SocketServer
import sys
import urlparse
import shutil

# Using 'localhost' means that we only accept connections
# via the loop back interface.
SERVER_PORT = 5103
SERVER_HOST = ''
SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
SRC_DIR = os.path.dirname(SCRIPT_DIR)


class QuittableHTTPServer(SocketServer.ThreadingMixIn,
                          BaseHTTPServer.HTTPServer):
  """An HTTP server that will quit when |is_running| is set to False.

  We also use SocketServer.ThreadingMixIn in order to handle requests
  asynchronously for faster responses."""
  pass


def KeyValuePair(string, sep='='):
  """"Safely" split a string at |sep| into a [key, value] pair.

  If |sep| does not exist in |string|, then the entire |string| is the key and
  the value is set to an empty string."""
  if sep in string:
    return string.split(sep)
  else:
    return [string, '']


class QuittableHTTPHandler(SimpleHTTPServer.SimpleHTTPRequestHandler):
  """A small handler that looks for '?quit=1' query in the path and shuts itself
  down if it finds that parameter."""

  def do_OPTIONS(self):
    self.send_response(200, 'OK')
    self.send_header('Access-Control-Allow-Methods', 'GET, OPTIONS, HEAD')
    self.send_header('Access-Control-Allow-Headers', 'target')
    self.end_headers()

  def do_GET(self):
    (_, _, _, query, _) = urlparse.urlsplit(self.path)
    url_params = dict([KeyValuePair(key_value)
                       for key_value in query.split('&')])
    if 'quit' in url_params and '1' in url_params['quit']:
      self.send_response(200, 'OK')
      self.send_header('Content-type', 'text/html')
      self.send_header('Content-length', '0')
      self.end_headers()
      self.server.shutdown()
      return

    headers = str(self.headers)
    begin = headers.find('Range: ')
    if begin != -1:
      end = headers.find('\n', begin)
      if end != -1:
        line = headers[begin:end]
        ind = line.find('=')
        byte_range = line[ind + 1:].split('-')
        byte_begin = int(byte_range[0])
        byte_end = int(byte_range[1])
        length = byte_end - byte_begin + 1
        f = self.send_partial(byte_begin, length)
        if f:
          shutil.copyfileobj(f, self.wfile, length)
          f.seek(-byte_begin, os.SEEK_CUR)
          f.close()
          return

    SimpleHTTPServer.SimpleHTTPRequestHandler.do_GET(self)

  def end_headers(self):
    self.send_header("Access-Control-Allow-Origin", "*")
    SimpleHTTPServer.SimpleHTTPRequestHandler.end_headers(self)

  def send_partial(self, offset, length):
    """The following code is lifed from SimpleHTTPServer.send_head()
    The only change is that a 206 response is sent instead of 200.
    """
    path = self.translate_path(self.path)
    f = None
    if os.path.isdir(path):
      if not self.path.endswith('/'):
        # redirect browser - doing basically what apache does
        self.send_response(301)
        self.send_header("Location", self.path + "/")
        self.end_headers()
        return None
      for index in "index.html", "index.htm":
        index = os.path.join(path, index)
        if os.path.exists(index):
          path = index
          break
      else:
        return self.list_directory(path)
    ctype = self.guess_type(path)
    try:
      # Always read in binary mode. Opening files in text mode may cause
      # newline translations, making the actual size of the content
      # transmitted *less* than the content-length!
      f = open(path, 'rb')
      #f.seek(offset)
    except IOError:
      self.send_error(404, "File not found")
      return None
    fs = os.fstat(f.fileno())
    if offset + length > fs[6] or length < 0:
      self.send_error(416, 'Request range not satisfiable')
      return None
    self.send_response(206, 'Partial content')
    f.seek(offset, os.SEEK_CUR)
    self.send_header("Content-Range",
                     str(offset) + '-' + str(length + offset - 1))
    self.send_header("Content-Length", str(length))
    self.send_header("Content-type", ctype)
    self.send_header("Last-Modified", self.date_time_string(fs.st_mtime))
    self.end_headers()
    return f


def Run(server_address,
        server_class=QuittableHTTPServer,
        handler_class=QuittableHTTPHandler):
  httpd = server_class(server_address, handler_class)
  logging.info("Starting local server on port %d", server_address[1])
  logging.info("To shut down send http://localhost:%d?quit=1",
               server_address[1])
  httpd.serve_forever()
  logging.info("Shutting down local server on port %d", server_address[1])


def main(args):
  logging.getLogger().setLevel(logging.INFO)
  os.chdir(os.path.join(SRC_DIR, 'out', 'publish'))
  if args:
    port = int(args[0])
  else:
    port = SERVER_PORT
  Run((SERVER_HOST, port))
  return 0


if __name__ == '__main__':
  sys.exit(main(sys.argv[1:]))
