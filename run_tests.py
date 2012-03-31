#!/usr/bin/env python
# -*- coding: utf-8 -*-
"""Tests"""
__author__ = 'Pedro Larroy'

import sys
import os
import unittest
import subprocess

def xsystem(args):
    if subprocess.call(args) != 0:
        raise RuntimeError('check_exit failed, while executing:', ' '.join(args))

def xcall(args, env=None):
    p = subprocess.Popen(args, stdout=subprocess.PIPE, stderr=subprocess.PIPE, env=env)
    (stdout, stderr) = p.communicate()
    ret = p.returncode
    return (stdout, stderr, ret)

class Cpp_unit_tests(unittest.TestCase):
    def runTest(self):
        print 'running C++ unit tests:'
        prg = 'build/debug/unit_tests/unit_tests'
        self.assert_(os.path.exists(prg))
        res = xcall([prg])
        sys.stdout.write(res[0])
        sys.stderr.write(res[1])
        self.assert_(res[2] == 0)

class Crawler_test(unittest.TestCase):
    def runTest(self):
        prg = 'build/debug/crawler/crawler'
        self.assert_(os.path.exists(prg))
#        env = {'CRAWLER_DB_NAMESPACE': 'mycelium.test'}
#        res = xcall([prg], env)



if __name__ == "__main__":
    if not os.path.exists('build/debug'):
        sys.stderr.write('Please do a debug build to run the tests\n')
        sys.exit(1)

    unittest.main()
