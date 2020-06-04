/*
 * Copyright (c) 2014 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

var PAK_FILE = 'id1/pak0.pak';

function updateStatus(message) {
  document.getElementById('status').innerHTML += message + '<br />';
}

/*
 * Creates and add to the DOM the NaCl embed tag which
 * in effect launches Quake.
 */
function runQuake(pwd) {
  var embed = document.createElement('embed');
  embed.width = 800;
  embed.height = 600;
  embed.type = 'application/x-nacl';
  embed.src = 'quakespasm.nmf';
  if (pwd)
    embed.setAttribute('PWD', pwd);
  document.getElementById('quake').appendChild(embed);
}

function extractNextEntry(entries, i, filesystem, oncomplete) {
  if (i === entries.length) {
    oncomplete();
    return;
  }

  var entry = entries[i];
  if (entry.directory) {
    filesystem.root.getDirectory(entry.filename, { create : true },
      function() {
        extractNextEntry(entries, i + 1, filesystem, oncomplete);
      }
    );
    return;
  }

  filesystem.root.getFile(entry.filename, { create : true },
    function(fileEntry) {
      var writer = new zip.FileWriter(fileEntry);
      entry.getData(writer,
        function() {
          extractNextEntry(entries, i + 1, filesystem, oncomplete);
        });
    },
    function(error) {
      updateStatus('error creating temp file: ' + error);
    }
  );
}

function extractZipFile(file, filesystem, oncomplete) {
  updateStatus('Extracting file: ' + file.name);
  zip.createReader(new zip.BlobReader(file),
    function(zipReader) {
      zipReader.getEntries(function(entries) {
        extractNextEntry(entries, 0, filesystem, oncomplete);
      });
    },
    function() {
      updateStatus('Error reading zip file');
    }
  )
}

function uploadDidChange(event) {
  function oncomplete() {
    // Once we have extracted all the files then
    // launch the game.
    updateStatus("Extracted all file. Launching Quake.");
    runQuake('/tmp');
  }

  window.webkitRequestFileSystem(window.TEMPORARY, 50 * 1024 * 1024,
    function(fs) {
      var file = event.target.files[0];
      extractZipFile(file, fs, oncomplete);
    },
    function() {
      updateStatus('Error accessing html5 filesystem.');
    }
  );
}

function loadFromLocalStorage() {
  function loadFailure() {
    updateStatus('Quake data not found in local html5 filesystem.');
    updateStatus('Please locate a quake level set (for example the '
      + '<a href="http://www.libsdl.org/projects/quake/data/quakesw-1.0.6.zip">'
      + 'shareware levels</a>) and either extract them alongside the nmf file,'
      + ' or use the upload button below to unzip them in the local html5'
      + ' filesystem.');
    // Create an html5 file input elemnt in so the user can upload the game
    // data as a zip file.
    document.getElementById('quake').innerHTML =
      '<input type="file" accept="application/zip" id="infile">';
    document.getElementById('infile').addEventListener('change',
                                                       uploadDidChange,
                                                       false);
  }

  function loadSuccess() {
    updateStatus('Found pak file in local storage. Launching quake.');
    runQuake('/tmp');
  }

  window.webkitRequestFileSystem(window.TEMPORARY, 3 * 1024 * 1024,
    function(fs) {
      fs.root.getFile(PAK_FILE, {}, loadSuccess, loadFailure);
    }
  );
}

function onLoad() {
  updateStatus('Searching for Quake data (' + PAK_FILE + ')');
  var req = new XMLHttpRequest();
  req.onload = function() {
    if (this.status === 200) {
      updateStatus('Found ' + PAK_FILE + '. Launching quake.');
      runQuake();
    } else {
      // Failed to find PAK_FILE on webserver, try local storage
      updateStatus('Quake data not found alongside executable.');
      loadFromLocalStorage();
    }
  };
  req.open('GET', PAK_FILE);
  req.send(null);
}

window.onload = function() {
  onLoad();
};
