#!/usr/bin/env python
# -*- coding: utf-8 -*-
""""""
from SCons.Script import *
import re
from utils import plat_id
from utils import which


__author__ = 'Pedro Larroy'
__version__ = '1.0'

def add_option(name, help, nargs):
    AddOption('--{0}'.format(name),
        dest = name,
        type = 'string',
        nargs = nargs,
        action = 'store',
        help = help)

def has_option(name):
    x = GetOption(name)
    if x == False or x is None or x == '':
        return False
    return True

def get_option(name):
    return GetOption(name)

def get_sources(file):
    fd = open(file, 'r')
    sources = fd.readlines()
    sources = map(lambda x: x.rstrip(), sources)
    if not len(sources):
        raise SCons.Errors.StopError('no sources in %s!' % file)
    sources=[l for l in sources if len(l)]
    return sources


def setup_quiet_build(env, colorblind=False):
    """Will fill an SCons env object with nice colors and quiet build strings. Makes warnings evident."""
    # colors
    c = dict()
    c['cyan']   = '\033[96m'
    c['purple'] = '\033[95m'
    c['blue']   = '\033[94m'
    c['bold_blue']   = '\033[94;1m'
    c['green']  = '\033[92m'
    c['yellow'] = '\033[93m'
    c['red']    = '\033[91m'
    c['magenta']= '\033[35m'
    c['bold_magenta']= '\033[35;1m'
    c['inverse']= '\033[7m'
    c['bold']   = '\033[1m'
    c['rst']    = '\033[0m'

    # if the output is not a terminal, remove the c
    # also windows console doesn't know about ansi c seems
    if not sys.stdout.isatty()\
        or re.match('^win.*', plat_id())\
        or colorblind:

       for key, value in c.iteritems():
          c[key] = ''

    compile_cxx_msg = '%s[CXX]%s %s$SOURCE%s' % \
       (c['blue'], c['rst'], c['yellow'], c['rst'])

    compile_c_msg = '%s[CC]%s %s$SOURCE%s' % \
       (c['cyan'], c['rst'], c['yellow'], c['rst'])

    compile_shared_msg = '%s[SHR]%s %s$SOURCE%s' % \
       (c['bold_blue'], c['rst'], c['yellow'], c['rst'])

    link_program_msg = '%s[LNK exe]%s %s$TARGET%s' % \
       (c['bold_magenta'], c['rst'], c['bold'] + c['yellow'] + c['inverse'], c['rst'])

    link_lib_msg = '%s[LIB st]%s %s$TARGET%s' % \
       ('', c['rst'], c['cyan'], c['rst'])

    ranlib_library_msg = '%s[RANLIB]%s %s$TARGET%s' % \
       ('', c['rst'], c['cyan'], c['rst'])

    link_shared_library_msg = '%s[LNK shr]%s %s$TARGET%s' % \
       (c['bold_magenta'], c['rst'], c['bold'], c['rst'])

    pch_compile = '%s[PCH]%s %s$SOURCE%s -> %s$TARGET%s' %\
       (c['bold_magenta'], c['rst'], c['bold'], c['rst'], c['bold'], c['rst'])

    env['CXXCOMSTR']   = compile_cxx_msg
    env['SHCXXCOMSTR'] = compile_shared_msg
    env['CCCOMSTR']    = compile_c_msg
    env['SHCCCOMSTR']  = compile_shared_msg
    env['ARCOMSTR']    = link_lib_msg
    env['SHLINKCOMSTR'] = link_shared_library_msg
    env['LINKCOMSTR']  = link_program_msg
    env['RANLIBCOMSTR']= ranlib_library_msg
    env['GCHCOMSTR'] = pch_compile

def color_sample():
    """Show a sample of colors that will be used for SCons build"""
    env = dict()
    setup_quiet_build(env)
    for item in env.iteritems():
        print item[0],item[1]


