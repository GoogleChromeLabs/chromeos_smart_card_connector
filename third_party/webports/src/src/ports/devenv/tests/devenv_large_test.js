/*
 * Copyright (c) 2014 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

'use strict';

// Install several packages.
// This test must be run before any tests that use these packages.
TEST_F(DevEnvTest, 'testPackageInstall', function() {
  var self = this;
  return Promise.resolve().then(function() {
    return self.installPackage('coreutils');
  }).then(function() {
    return self.installPackage('zlib');
  }).then(function() {
    return self.installPackage('curl');
  }).then(function() {
    return self.installPackage('make');
  }).then(function() {
    return self.installPackage('python');
  }).then(function() {
    return self.installPackage('git');
  });
});

// Run a test on devenv, and clean the home directory afterwards. This allows
// for tests that touch files. Coreutils must be installed before this kind of
// test can be run.
function DevEnvFileTest() {
  DevEnvTest.call(this);
}
DevEnvFileTest.prototype = new DevEnvTest();
DevEnvFileTest.prototype.constructor = DevEnvFileTest;

DevEnvFileTest.prototype.setUp = function() {
  var self = this;
  return Promise.resolve().then(function() {
    return DevEnvTest.prototype.setUp.call(self);
  });
};

DevEnvFileTest.prototype.tearDown = function() {
  var self = this;
  return Promise.resolve().then(function() {
    return self.rmRf('/home/user');
  }).then(function() {
    return DevEnvTest.prototype.tearDown.call(self);
  });
};

TEST_F(DevEnvFileTest, 'testDirs', function() {
  // Test mkdir, ls, and rmdir.
  var self = this;
  return Promise.resolve().then(function() {
    return self.checkCommand('mkdir foo', 0, '');
  }).then(function() {
    return self.checkCommand('ls', 0, 'foo\n');
  }).then(function() {
    return self.checkCommand('rmdir foo', 0, '');
  });
});

TEST_F(DevEnvFileTest, 'testCatRm', function() {
  // Test cat and rm.
  var self = this;
  var str = 'Hello, world!\n';
  return Promise.resolve().then(function() {
    return self.writeFile('/home/user/foo.txt', str);
  }).then(function() {
    return self.checkCommand('cat foo.txt', 0, str);
  }).then(function() {
    return self.checkCommand('rm foo.txt', 0, '');
  });
});

TEST_F(DevEnvFileTest, 'testGit', function() {
  var self = this;
  return Promise.resolve().then(function() {
    return self.checkCommand('git config --global user.name "John Doe"');
  }).then(function() {
    return self.checkCommand(
        'git config --global user.email johndoe@example.com');
  }).then(function() {
    return self.checkCommand('git config --global color.ui false');
  }).then(function() {
    return self.checkCommand('mkdir foo');
  }).then(function() {
    return self.checkCommandReLines(
        'cd foo && git init', 0,
        ['warning: templates not found ' +
         '/naclports-dummydir/share/git-core/templates',
         'Initialized empty Git repository in /home/user/foo/.git/']);
  }).then(function() {
    return self.checkCommand('mkdir foo/bar', 0, '');
  }).then(function() {
    return self.writeFile(
        '/home/user/foo/bar/README', 'hello there\n');
  }).then(function() {
    return self.checkCommand('cd foo && git add .', 0, '');
  }).then(function() {
    return self.checkCommandReLines(
        'cd foo && git commit -am initial', 0,
        [/^\[master \(root-commit\) [0-9a-f]{7}\] initial$/,
         ' 1 file changed, 1 insertion(+)',
         ' create mode 100644 bar/README']);
  }).then(function() {
    return self.writeFile(
        '/home/user/foo/bar/README', 'hello there\ntesting\n');
  }).then(function() {
    return self.writeFile(
        '/home/user/foo/test.txt', 'more stuff\n');
  }).then(function() {
    return self.checkCommand('cd foo && git add test.txt', 0, '');
  }).then(function() {
    return self.checkCommandReLines(
        'cd foo && git commit -am change2', 0,
        [/^\[master [0-9a-f]{7}\] change2$/,
         ' 2 files changed, 2 insertions(+)',
         ' create mode 100644 test.txt']);
  }).then(function() {
    return self.checkCommand('rm foo/test.txt', 0, '');
  }).then(function() {
    return self.checkCommandReLines(
        'cd foo && git commit -am "change number 3"', 0,
        [/^\[master [0-9a-f]{7}\] change number 3$/,
         ' 1 file changed, 1 deletion(-)',
         ' delete mode 100644 test.txt']);
  }).then(function() {
    return self.checkCommandReLines(
        'cd foo && PAGER=cat git log --full-diff -p .', 0,
        [/^commit [0-9a-f]{40}$/,
         'Author: John Doe <johndoe@example.com>',
         /^Date:   .+$/,
         '',
         '    change number 3',
         '',
         'diff --git a/test.txt b/test.txt',
         'deleted file mode 100644',
         /^index [0-9a-f]{7}\.\.0000000$/,
         '--- a/test.txt',
         '+++ /dev/null',
         '@@ -1 +0,0 @@',
         '-more stuff',
         '',
         /^commit [0-9a-f]{40}$/,
         'Author: John Doe <johndoe@example.com>',
         /^Date:   .+$/,
         '',
         '    change2',
         '',
         'diff --git a/bar/README b/bar/README',
         /^index [0-9a-f]{7}\.\.[0-9a-f]{7} 100644$/,
         '--- a/bar/README',
         '+++ b/bar/README',
         '@@ -1 +1,2 @@',
         ' hello there',
         '+testing',
         'diff --git a/test.txt b/test.txt',
         'new file mode 100644',
         /^index 0000000..[0-9a-f]{7}$/,
         '--- /dev/null',
         '+++ b/test.txt',
         '@@ -0,0 +1 @@',
         '+more stuff',
         '',
         /^commit [0-9a-f]{40}$/,
         'Author: John Doe <johndoe@example.com>',
         /^Date:   .+$/,
         '',
         '    initial',
         '',
         'diff --git a/bar/README b/bar/README',
         'new file mode 100644',
         /^index 0000000..[0-9a-f]{7}$/,
         '--- /dev/null',
         '+++ b/bar/README',
         '@@ -0,0 +1 @@',
         '+hello there']);
  });
});

TEST_F(DevEnvFileTest, 'testMake', function() {
  var self = this;
  var i = 0;
  var makefile = [
    '.SUFFIXES:',
    'all: part0.z part1.z part2.z part3.z part4.z part5.z part6.z part7.z',
    '%.z: %.y',
    '\tcp $< $@',
    '%.y: %.x',
    '\tcp $< $@',
    '%.x: foo.txt',
    '\tcp $< $@',
  ].join('\n') + '\n';
  var foo = 'This is a test!\nTesting!\n';
  return Promise.resolve().then(function() {
    return self.writeFile('/home/user/Makefile', makefile);
  }).then(function() {
    return self.writeFile('/home/user/foo.txt', foo);
  }).then(function() {
    return self.checkCommand('make -j10');
  }).then(function() {
    function checkOne() {
      return Promise.resolve().then(function() {
        return self.readFile('/home/user/part' + i + '.z');
      }).then(function(data) {
        ASSERT_EQ(foo, data);
        i++;
        if (i < 8) {
          return checkOne();
        }
      });
    }
    return checkOne();
  });
});

TEST_F(DevEnvFileTest, 'testPythonBasic', function() {
  var self = this;
  var script = [
    '#!/usr/bin/python',
    '',
    'import sys',
    '',
    'sys.exit(42)',
  ].join('\n') + '\n';
  return Promise.resolve().then(function() {
    return self.writeFile('/home/user/test.py', script);
  }).then(function() {
    return self.checkCommand('./test.py', 42);
  });
});

TEST_F(DevEnvFileTest, 'testPythonSubprocess', function() {
  var self = this;
  // TODO(bradnelson): Drop this once subprocess works on glibc.
  if (self.params['TOOLCHAIN'] === 'glibc') {
    // Skip on glibc.
    return;
  }
  var script = [
    '#!/usr/bin/python',
    '',
    'import subprocess',
    'import sys',
    '',
    'n = int(sys.argv[1])',
    'if n > 3:',
    '  sys.exit(n)',
    'else:',
    '  a = subprocess.Popen([sys.executable, "test.py", str(n * 2)])',
    '  b = subprocess.Popen([sys.executable, "test.py", str(n * 2 + 1)])',
    '  a.wait()',
    '  b.wait()',
    '  sys.exit(a.returncode + b.returncode + n)',
  ].join('\n') + '\n';
  return Promise.resolve().then(function() {
    return self.writeFile('/home/user/test.py', script);
  }).then(function() {
    function test(n) {
      if (n > 3) {
        return n;
      } else {
        return test(n * 2) + test(n * 2 + 1) + n;
      }
    }
    return self.checkCommand('./test.py 1', test(1));
  });
});

TEST_F(DevEnvFileTest, 'testGetUrlAndUnzip', function() {
  var self = this;
  return Promise.resolve().then(function() {
    return self.checkCommand(
       'geturl http://nacltools.storage.googleapis.com/io2014/voronoi.zip ' +
       'voronoi.zip');
  }).then(function() {
    return self.checkCommand('unzip voronoi.zip');
  }).then(function() {
    return self.readFile('/home/user/voronoi/Makefile');
  }).then(function(data) {
    // Check there's something in the makefile.
    ASSERT_GT(data.length, 100);
  });
});

TEST_F(DevEnvFileTest, 'testCurlAndUnzip', function() {
  var self = this;
  return Promise.resolve().then(function() {
    return self.checkCommand(
       'curl https://nacltools.storage.googleapis.com/io2014/voronoi.zip -O');
  }).then(function() {
    return self.checkCommand('unzip voronoi.zip');
  }).then(function() {
    return self.readFile('/home/user/voronoi/Makefile');
  }).then(function(data) {
    // Check there's something in the makefile.
    ASSERT_GT(data.length, 100);
  });
});
