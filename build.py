#!/usr/bin/env python
import os
import sys
import subprocess
import os
import getopt
import multiprocessing


NPROCS = int(multiprocessing.cpu_count()*1.30+1)
STAGE = ''

def xsystem(args):
    if subprocess.call(args) != 0:
        raise RuntimeError('{0}: check_exit failed, while executing: '.format(STAGE) + ' '.join(args))

def xcall(args):
    p = subprocess.Popen(args, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    (stdout, stderr) = p.communicate()
    ret = p.returncode
    return (stdout, stderr, ret)


def main():
    #if len(sys.argv) > 1:
    (options, args) = getopt.getopt(sys.argv[1:], 'j:')
    for (o,a) in options:
        if o in ('-j'):
            global NPROCS
            NPROCS = int(a)


    old_dir = os.getcwd()

    STAGE = 'Building c-ares...'
    print STAGE
    os.chdir('3rd_party/c-ares/')
    xsystem(['./buildconf'])
    xsystem(['./configure',
        '--enable-debug',
        '--enable-nonblocking',
        '--prefix={0}'.format(os.path.join(old_dir, '3rd_party','c-ares_install'))
    ])
    xsystem(['make', '-j{0}'.format(NPROCS)])
    xsystem(['make', 'install'])
    os.chdir(old_dir)

    STAGE = 'Building libcurl...'
    print STAGE
    os.chdir('3rd_party/curl/')
    xsystem(['./buildconf'])
    xsystem(['./configure',
        '--disable-ldap',
        '--without-libssh2',
        '--without-librtmp',
        '--enable-debug',
        '--enable-ares={0}'.format(os.path.join(old_dir, '3rd_party','c-ares_install')),
        '--prefix={0}'.format(os.path.join(old_dir, '3rd_party', 'curl_install')),
    ])
    xsystem(['make', '-j{0}'.format(NPROCS)])
    xsystem(['make', 'install'])
    os.chdir(old_dir)


    STAGE = 'Building libstemmer...'
    print STAGE
    os.chdir('3rd_party/libstemmer_c/')
    xsystem(['make', '-j{0}'.format(NPROCS)])
    os.chdir(old_dir)


    STAGE = 'Building mongodb...'
    print STAGE
    os.chdir('3rd_party/mongodb/')
    xsystem(['scons .'])
    os.chdir(old_dir)


    STAGE = 'Building main sources...'
    print STAGE
    xsystem(['scons', '-j{0}'.format(NPROCS)])

    return

if __name__ == '__main__':
    main()

