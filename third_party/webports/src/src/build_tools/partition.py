#!/usr/bin/env python
# Copyright (c) 2013 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
"""Manage partitioning of port builds.

Download historical data from the naclports builders, and use it to
partition all of the projects onto the builder shards so each shard builds in
about the same amount of time.

Example use:

    $ ./partition.py -b linux-clang-

    builder 0 (total: 2786)
      bzip2
      zlib
      boost
      glibc-compat
      openssl
      libogg
      ...
    builder 1 (total: 2822)
      zlib
      libpng
      Regal
      glibc-compat
      ncurses
      ...
    builder 2 (total: 2790)
      zlib
      libpng
      bzip2
      jpeg
      ImageMagick
      glibc-compat
      ...
    Difference between total time of builders: 36


Pipe the results above (with appropriate use of -n and -p) into partition*.txt
so the partition can be used in the future.

The script is also used by the buildbot to read canned partitions.

Example use:

    $ ./partition.py -t <index> -n <number_of_shards>
"""

from __future__ import print_function

import argparse
import json
import os
import sys
import urllib2

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
ROOT_DIR = os.path.dirname(SCRIPT_DIR)
TOOLCHAINS = ('bionic', 'clang-newlib', 'glibc', 'pnacl')

sys.path.append(os.path.join(ROOT_DIR, 'lib'))

import naclports
import naclports.source_package
from naclports.util import LogVerbose


class Error(naclports.Error):
  pass


def GetBuildOrder(projects):
  rtn = []
  packages = [naclports.source_package.CreatePackage(p) for p in projects]
  for package in packages:
    for dep in package.DEPENDS:
      for ordered_dep in GetBuildOrder([dep]):
        if ordered_dep not in rtn:
          rtn.append(ordered_dep)
    if package.NAME not in rtn:
      rtn.append(package.NAME)
  return rtn


def GetDependencies(projects):
  deps = GetBuildOrder(projects)
  return set(deps) - set(projects)


def DownloadDataFromBuilder(builder, build):
  max_tries = 30

  for _ in xrange(max_tries):
    url = 'http://build.chromium.org/p/client.nacl.ports/json'
    url += '/builders/%s/builds/%d' % (builder, build)
    LogVerbose('Downloading %s' % url)
    f = urllib2.urlopen(url)
    try:
      data = json.loads(f.read())
      text = data['text']
      if text == ['build', 'successful']:
        LogVerbose('  Success!')
        return data
      LogVerbose('  Not successful, trying previous build.')
    finally:
      f.close()
    build -= 1

  raise Error('Unable to find a successful build:\nBuilder: %s\nRange: [%d, %d]'
              % (builder, build - max_tries, build))


class Project(object):

  def __init__(self, name):
    self.name = name
    self.time = 0
    self.dep_names = GetDependencies([name])
    self.dep_times = [0] * len(self.dep_names)

  def UpdateDepTimes(self, project_map):
    for i, dep_name in enumerate(self.dep_names):
      dep = project_map[dep_name]
      self.dep_times[i] = dep.time

  def GetTime(self, used_dep_names):
    time = self.time
    for i, dep_name in enumerate(self.dep_names):
      if dep_name not in used_dep_names:
        time += self.dep_times[i]
    return time


class Projects(object):

  def __init__(self):
    self.projects = []
    self.project_map = {}
    self.dependencies = set()

  def AddProject(self, name):
    if name not in self.project_map:
      project = Project(name)
      self.projects.append(project)
      self.project_map[name] = project
    return self.project_map[name]

  def AddDataFromBuilder(self, builder, build):
    data = DownloadDataFromBuilder(builder, build)
    for step in data['steps']:
      text = step['text'][0]
      text_tuple = text.split()
      if len(text_tuple) != 2 or text_tuple[0] not in TOOLCHAINS:
        continue
      _, name = text_tuple
      project = self.AddProject(name)
      project.time = step['times'][1] - step['times'][0]

  def PostProcessDeps(self):
    for project in self.projects:
      project.UpdateDepTimes(self.project_map)
      for dep_name in project.dep_names:
        self.dependencies.add(dep_name)

  def __getitem__(self, name):
    return self.project_map[name]


class ProjectTimes(object):

  def __init__(self):
    self.project_names = set()
    self.projects = []
    self.total_time = 0

  def GetTotalTimeWithProject(self, project):
    return self.total_time + project.GetTime(self.project_names)

  def AddProject(self, projects, project):
    self.AddDependencies(projects, project)
    self.project_names.add(project.name)
    self.projects.append(project)
    self.total_time = self.GetTotalTimeWithProject(project)

  def AddDependencies(self, projects, project):
    for dep_name in project.dep_names:
      if dep_name not in self.project_names:
        self.AddProject(projects, projects[dep_name])

  def HasProject(self, project):
    return project.name in self.project_names

  def TopologicallySortedProjectNames(self, projects):
    sorted_project_names = []

    def Helper(project):
      for dep_name in project.dep_names:
        if dep_name not in sorted_project_names:
          Helper(projects[dep_name])
      sorted_project_names.append(project.name)

    for project in self.projects:
      Helper(project)

    return sorted_project_names


def Partition(projects, dims):
  # Greedy algorithm: sort the projects by slowest to fastest, then add the
  # projects, in order, to the shard that has the least work on it.
  #
  # Note that this takes into account the additional time necessary to build a
  # projects dependencies, if those dependencies have not already been built.
  parts = [ProjectTimes() for _ in xrange(dims)]
  sorted_projects = sorted(projects.projects, key=lambda p: -p.time)
  for project in sorted_projects:
    if any(part.HasProject(project) for part in parts):
      continue

    key = lambda p: p[1].GetTotalTimeWithProject(project)
    index, _ = min(enumerate(parts), key=key)
    parts[index].AddProject(projects, project)
  return parts


def LoadCanned(parts):
  # Return an empty partition for the no-sharding case.
  if parts == 1:
    return [[]]
  partitions = []
  partition = []
  input_file = os.path.join(SCRIPT_DIR, 'partition%d.txt' % parts)
  LogVerbose("LoadCanned: %s" % input_file)
  with open(input_file) as fh:
    for line in fh:
      if line.strip()[0] == '#':
        continue
      if line.startswith('  '):
        partition.append(line[2:].strip())
      else:
        if partition:
          partitions.append(partition)
          partition = []
  assert not partition
  assert len(partitions) == parts, partitions
  # Return a small set of packages for testing.
  if os.environ.get('TEST_BUILDBOT'):
    partitions[0] = [
        'corelibs',
        'glibc-compat',
        'nacl-spawn',
        'ncurses',
        'readline',
        'libtar',
        'zlib',
        'lua',
    ]
  return partitions


def FixupCanned(partitions):
  all_projects = [p for p in naclports.source_package.SourcePackageIterator()]
  all_names = [p.NAME for p in all_projects if not p.DISABLED]
  disabled_names = [p.NAME for p in all_projects if p.DISABLED]

  # Blank the last partition and fill it with anything not in the first two.
  partitions[-1] = []
  covered = set()
  for partition in partitions[:-1]:
    for item in partition:
      covered.add(item)

  for item in all_names:
    if item not in covered:
      partitions[-1].append(item)

  # Order by dependencies.
  partitions[-1] = GetBuildOrder(partitions[-1])

  # Check that all the items still exist.
  for i, partition in enumerate(partitions):
    for item in partition:
      if item not in all_names and item not in disabled_names:
        raise Error('non-existent package in partition %d: %s' % (i, item))

  # Check that partitions include all of their dependencies.
  for i, partition in enumerate(partitions):
    deps = GetDependencies(partition)
    for dep in deps:
      if not dep in partition:
        raise Error('dependency missing from partition %d: %s' % (i, dep))

  return partitions


def PrintCanned(index, parts):
  canned = GetCanned(index, parts)
  print(' '.join(canned))


def GetCanned(index, parts):
  assert index >= 0 and index < parts, [index, parts]
  partitions = LoadCanned(parts)
  partitions = FixupCanned(partitions)
  LogVerbose("Found %d packages for shard %d" % (len(partitions[index]), index))
  return partitions[index]


def main(args):
  parser = argparse.ArgumentParser()
  parser.add_argument('--check', action='store_true',
                      help='check canned partition information is up-to-date.')
  parser.add_argument('-v', '--verbose', action='store_true',
                      help='Output extra information.')
  parser.add_argument('-t', '--print-canned', type=int,
                      help='Print a the canned partition list and exit.')
  parser.add_argument('-b', '--bot-prefix', help='builder name prefix.',
                      default='linux-clang-')
  parser.add_argument('-n', '--num-bots',
                      help='Number of builders on the waterfall to collect '
                      'data from or to print a canned partition for.',
                      type=int, default=5)
  parser.add_argument('-p', '--num-parts',
                      help='Number of parts to partition things into '
                      '(this will differ from --num-bots when changing the '
                      'number of shards).',
                      type=int, default=5)
  parser.add_argument('--build-number', help='Builder number to look at for '
                      'historical data on build times.', type=int, default=-1)
  options = parser.parse_args(args)
  naclports.SetVerbose(options.verbose)

  if options.check:
    for num_bots in xrange(1, 6):
      print('Checking partioning with %d bot(s)' % (num_bots))
      # GetCanned with raise an Error if the canned partition information is
      # bad, which in turn will trigger a non-zero return from this script.
      GetCanned(0, num_bots)
    return

  if options.print_canned is not None:
    PrintCanned(options.print_canned, options.num_bots)
    return

  projects = Projects()
  for bot in range(options.num_bots):
    bot_name = '%s%d' % (options.bot_prefix, bot)
    LogVerbose('Attempting to add data from "%s"' % bot_name)
    projects.AddDataFromBuilder(bot_name, options.build_number)
  projects.PostProcessDeps()

  parts = Partition(projects, options.num_parts)
  for i, project_times in enumerate(parts):
    print('builder %d (total: %d)' % (i, project_times.total_time))
    project_names = project_times.TopologicallySortedProjectNames(projects)
    print('  %s' % '\n  '.join(project_names))

  times = list(sorted(part.total_time for part in parts))
  difference = 0
  for i in range(1, len(times)):
    difference += times[i] - times[i - 1]
  print('Difference between total time of builders: %d' % difference)


if __name__ == '__main__':
  try:
    sys.exit(main(sys.argv[1:]))
  except Error as e:
    sys.stderr.write("%s\n" % e)
    sys.exit(1)
