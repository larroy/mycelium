#!/usr/bin/env python
# -*- coding: utf-8 -*-
"""Tests"""
__author__ = 'Pedro Larroy'

import sys
import os
import unittest
import subprocess
sys.path.append('dist')
import common


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
        self.assertTrue(os.path.exists(prg))
        res = xcall([prg])
        sys.stdout.write(res[0])
        sys.stderr.write(res[1])
        self.assertTrue(res[2] == 0)

class HTML_lexer_test(unittest.TestCase):
    def runTest(self):
        res = common.html_lex('<html><head><title>Hi there</title></head><body>my <a href="body/">body</a> is great. Would like to come <a href="inside">inside</a>?</body></html>', 'http://example.com')
        self.assertEqual(res.analysis.title, 'Hi there')
        self.assertEqual(res.text, '\nHi there\nmy body is great. Would like to come inside?')
        self.assertEqual(res.links, '\x01http://example.com/body/\x02body\x03\x01http://example.com/inside\x02inside\x03')
        self.assertEqual(res.base_url, 'http://example.com')
        self.assertTrue(res.analysis.follow)
        self.assertTrue(res.analysis.index)




class Crawler_test(unittest.TestCase):
    def runTest(self):
        prg = 'build/debug/crawler/crawler'
        self.assertTrue(os.path.exists(prg))
#        env = {'CRAWLER_DB_NAMESPACE': 'mycelium.test'}
#        res = xcall([prg], env)



if __name__ == "__main__":
    if not os.path.exists('build/debug'):
        sys.stderr.write('Please do a debug build to run the tests\n')
        sys.exit(1)

    unittest.main()
