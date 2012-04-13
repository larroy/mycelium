#!/usr/bin/env python
# -*- coding: utf-8 -*-
"""Document indexer"""

import os
import sys
import subprocess
import document
import utils
import utils.progress
import common

def usage():
    sys.stderr.write('usage: {0}\n'.format(sys.argv[0]))

def index_text(d):
    pass


def parse_html(d):
    # TODO: fork in case of segfault etc.
    res = common.html_lex(str(d.content), utils.eu8(d.url))
    pass

def main():
    cursor = document.Doc.collection.find({'indexed': False})

    print 'Indexing {0} documents'.format(cursor.count())
    pb = utils.progress.progressBar(0, cursor.count())
    i = 0
    for d in cursor:
        #print d['url']
        if hasattr(d, 'content_type') and hasattr(d, 'http_code') and hasattr(d, 'content') and d['http_code'] == 200:
            if d.content_type == document.TEXT_PLAIN:
                index_text(d)

            if d.content_type == document.TEXT_HTML:
                parse_html(d)
                index_text(d)

        pb(i)
        i += 1

    print



    return 1

if __name__ == '__main__':
    sys.exit(main())

