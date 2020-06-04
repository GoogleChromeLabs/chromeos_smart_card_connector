#!/usr/bin/env python
# Copyright (c) 2013 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
"""Creates simple terminal for a NaCl module.

This script is designed to make the process of porting terminal based
Native Client executables simple by generating boilerplate .html and .js
files for a given Native Client module (.nmf).
"""

from __future__ import print_function

import argparse
import logging
import os
import sys

HTML_TEMPLATE = '''\
<!DOCTYPE html>
<html>
  <head>
    <title>%(title)s</title>
    <META HTTP-EQUIV="Pragma" CONTENT="no-cache" />
    <META HTTP-EQUIV="Expires" CONTENT="-1" />
    <script type="text/javascript" src="hterm.concat.js"></script>
    <script type="text/javascript" src="pipeserver.js"></script>
    <script type="text/javascript" src="naclprocess.js"></script>
    <script type="text/javascript" src="naclterm.js"></script>
    <script type="text/javascript" src="%(module_name)s.js"></script>
    %(include)s

    <style type="text/css">
      body {
        position: absolute;
        padding: 0;
        margin: 0;
        height: 100%%;
        width: 100%%;
        overflow: hidden;
      }

      #terminal {
        display: block;
        position: static;
        width: 100%%;
        height: 100%%;
      }
    </style>
    %(style)s
  </head>
  <body>
    <div id="terminal"></div>
  </body>
</html>
'''

JS_TEMPLATE = '''\
NaClTerm.nmf = '%(nmf)s'
'''

FORMAT = '%(filename)s: %(message)s'
logging.basicConfig(format=FORMAT)


def CreateTerm(filename, name=None, style=None, include=None):
  if not name:
    basename = os.path.basename(filename)
    name, _ = os.path.splitext(basename)

  style = style or []
  include = include or []

  htmlfile = name + '.html'
  logging.info('creating html: %s', htmlfile)
  with open(htmlfile, 'w') as outfile:
    args = {}
    args['title'] = name
    args['module_name'] = name

    styleHTML = ['<link rel="stylesheet" href="%s">' % css for css in style]
    args['style'] = '\n    '.join(styleHTML)

    includeHTML = ['<script src="%s"></script>' % js for js in include]
    args['include'] = '\n    '.join(includeHTML)

    outfile.write(HTML_TEMPLATE % args)

  jsfile = name + '.js'
  logging.info('creating js: %s', jsfile)
  with open(jsfile, 'w') as outfile:
    args = {}
    args['module_name'] = name
    args['nmf'] = os.path.basename(filename)
    outfile.write(JS_TEMPLATE % args)


def main(args):
  parser = argparse.ArgumentParser(description=__doc__)
  parser.add_argument('nmf', help='nmf file to load')
  parser.add_argument('-n', '--name', help='name of the application')
  parser.add_argument('-v', '--verbose', action='store_true',
                      help='be more verbose')
  parser.add_argument('-s', '--style', action='append', default=[],
                      help='include a CSS file in the generated HTML')
  parser.add_argument('-i', '--include', action='append', default=[],
                      help='include a JavaScript file in the generated HTML')

  options = parser.parse_args(args)
  if not options.verbose:
    logging.disable(logging.INFO)

  CreateTerm(options.nmf, options.name, options.style, options.include)
  return 0


if __name__ == '__main__':
  sys.exit(main(sys.argv[1:]))
