#!/usr/bin/env python
# -*- coding: utf-8 -*-
"""Module to index local files"""

__author__ = 'Pedro Larroy'

import os
import sys
import subprocess
import multiprocessing
import document
import string
import utils
import common
import re

PDFTOTEXT = "pdftotext"

def usage():
    sys.stderr.write('usage: {0} [list of directories to index]\n'.format(sys.argv[0]))

def xsystem(args):
    if subprocess.call(args) != 0:
        raise RuntimeError('check_exit failed, while executing:', ' '.join(args))

def xcall(args):
    p = subprocess.Popen(args, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    (stdout, stderr) = p.communicate()
    ret = p.returncode
    return (stdout, stderr, ret)

def extension(fname):
    return os.path.splitext(fname.lower())[1]


def filter_ascii_control(s):
    return filter(lambda x: x > chr(0x9) and x < chr(0xE) or x > chr(0x1F) and x != chr(0x7F), s)

class Indexer:
    known_extensions = ['.pdf', '.txt']
    def __init__(self):
        self.totext_fns = {
            '.pdf': self.totext_pdf
        }

    def directory(self, directory):
        for path, dirnames, filenames in os.walk(directory):
            for filename in filenames:
                self.file(os.path.join(path, filename))

    def totext_pdf(self, fname):
        result = xcall([PDFTOTEXT, "-enc", "UTF-8", "--", fname, "-"])
        if result[2]:
            return result
        else:
            return (filter_ascii_control(result[0]), result[1], result[2])

    def totext(self, fname):
        '''Converts to textual utf-8 representation'''
        (out, err, res) = self.totext_fns[extension(fname)](fname)
        if res == 0 and not utils.valid_utf8(out):
            out = ''
            err += '{0}: invalid utf-8 content'.format(sys.argv[0])
            res = 1

            sys.stderr.write('error indexing "{0}": invalid utf-8'.format(fname))

        return (out, err, res)

    def file(self, fname):
        # TODO: index also filenames
        if extension(fname) not in Indexer.known_extensions:
            return

        urls = 'file://{0}'.format(os.path.realpath(fname))
        url = common.Url(urls)
        url.normalize()
        doc = document.Doc()
        doc.url = url.get()
        assert(re.match(r'^file:///.+', doc.url))
        sys.stdout.write('indexing "{0}"'.format(doc.url))
        sys.stdout.flush()

        (out, err, res) = self.totext(fname)
        if res == 0:
            print ' ok.'
            doc.content = out
            doc.content_type = document.TEXT_PLAIN
            doc.flags = 1
            doc.http_code = 200
        else:
            doc.flags = 0
            doc.http_code = 415
            error = 'error indexing "{0}" reason: "{1}"'.format(fname, err)
            doc.error = error
            print ' error: {1}.'.format(err)
            sys.stderr.write(error)
        doc.save()

def main():
    if len(sys.argv) < 2:
        usage()
        return 1

    for arg in sys.argv:
        assert(os.path.isdir(arg) or os.path.isfile(arg))

    index = Indexer()
    for arg in sys.argv:
        if (os.path.isdir(arg)):
            index.directory(arg)
        elif (os.path.isfile(arg)):
            index.file(arg)
        else:
            assert(0)

    return 0

if __name__ == '__main__':
    sys.exit(main())

