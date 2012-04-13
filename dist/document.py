#!/usr/bin/env python
# -*- coding: utf-8 -*-
"""Class for accessing documents"""

import sys
import os
import minimongo


UNSET = 0
NONE = 1
UNRECOGNIZED = 2
TEXT_HTML = 3
XHTML = 4
RSS_XML = 5
ATOM_XML = 6
TEXT_XML = 7
TEXT_PLAIN = 8
APPLICATION_PDF = 9
FILE_DIRECTORY = 10
EMPTY = 11
CONTENT_TYPE_MAX = 12


class Doc(minimongo.Model):
    class Meta:
        host = os.environ.get('MYCELIUM_DB_HOST', 'localhost')
        if os.environ.has_key('MYCELIUM_DB_PORT'):
            port = os.environ.get('MYCELIUM_DB_PORT')
        (database, collection) = os.environ.get('MYCELIUM_DB_NS', 'mycelium.crawl').split('.')
        indices = (
            minimongo.Index('url'),
        )

