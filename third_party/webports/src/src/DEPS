deps = {
  "src/third_party/libapps":
    "https://chromium.googlesource.com/apps/libapps.git@6e65bd5",
  "src/third_party/zip.js":
    "https://github.com/gildas-lormeau/zip.js.git@4c93974",
  "src/third_party/clang_tools":
    "https://chromium.googlesource.com/chromium/src/tools/clang.git@7aa7166",
}

deps_os = {
  "win": {
    "src/third_party/cygwin":
      "http://src.chromium.org/svn/trunk/deps/third_party/cygwin@11984",
    "src/native_client/build":
      "http://src.chromium.org/native_client/trunk/src/native_client/build"
  },
}

hooks = [
  {
    "name": "clean_pyc",
    "pattern": ".",
    "action": [
        "python", "src/build_tools/clean_pyc.py", "src/build_tools", "src/lib"
    ],
  },
  {
    "name": "pip_install",
    "pattern": ".",
    "action": [
        "src/build_tools/pip_install.sh"
    ],
  },
]
