#!/usr/bin/env python
# -*- coding: utf-8 -*-
"""Utility script to get misc info for build and other tasks:
* canonical platform name, get with function plat_id
* Branch and revision, get with function branch_name
"""

__author__ = 'Pedro Larroy'
__version__ = '1.2'


# Notes: please install pysvn, otherwise:
#   on windows, the branch name is taken from local path
#   on linux is taken from remote path


from urlparse import urlparse
import os
import platform
import re
import subprocess
import sys
import sys



def xsystem(args):
    if subprocess.call(args) != 0:
        raise RuntimeError('check_exit failed, while executing: ', ' '.join(args))

def xcall(args):
    p = subprocess.Popen(args, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    (stdout, stderr) = p.communicate()
    ret = p.returncode
    return (stdout, stderr, ret)

# Path to SubWCRev.exe from TortoiseSVN for branch_name() in win32
SUBWCREV = 'c:/Program Files/TortoiseSVN/bin/SubWCRev.exe'


# platform ids can be:
# linux_x86
# linux_x86_64
# win32
# win64
def plat_id():
    '''Current OS to canonical platform names'''
    puname = platform.uname()
    os = puname[0]
    arch = puname[4]
    if os == 'Linux' and arch == 'x86_64':
        return 'linux_x86_64'
    elif os == 'Linux' and re.match('i.86',arch):
        return 'linux_x86'
    elif os == 'Windows' and arch == 'AMD64':
        return 'win64'
    elif os == 'Windows':
        return 'win32'
    else:
        return None

def _get_git_info():
    branch_name = None
    revision = None
    try:
       proc = subprocess.Popen(['git','branch','--no-color','-v'], stdout=subprocess.PIPE, stderr=subprocess.PIPE)
       stdout = proc.communicate()[0]
       if proc.returncode == 0:
           for l in re.split('(?:\r\n|\n)', stdout):
               m = re.match('^\*\s+(\([^)]+\)|[^ ]+)\s+([^ ]+).*',l)
               if m:
                   branch_name = m.group(1)
                   branch_name = re.sub('[()]', '', branch_name)
                   branch_name = re.sub(' ', '_', branch_name)
                   revision = m.group(2)
                   break
                   #print 'found: ', branch_name, ",", revision
               else:
                   #print 'nomatch', l
                   pass

    except OSError, e:
        #errmsg = sys.argv[0] + ': couldn\'t find branch name\n'
        #sys.stderr.write(errmsg)
        #sys.stderr.write(str(e))
        pass

    return branch_name, revision

def _get_hg_info():
    branch_name = None
    revision = None
    try:
       proc = subprocess.Popen(['hg','branch'], stdout=subprocess.PIPE, stderr=subprocess.PIPE)
       stdout = proc.communicate()[0]
       branch_name = stdout.rstrip()

       proc = subprocess.Popen(['hg','parents','--template={node}'], stdout=subprocess.PIPE, stderr=subprocess.PIPE)
       revision = proc.communicate()[0]
    except OSError, e:
        #errmsg = sys.argv[0] + ': couldn\'t find branch name\n'
        #sys.stderr.write(errmsg)
        #sys.stderr.write(str(e))
        pass

    return branch_name, revision


def _get_svn_info():
    try:
        from pysvn import Client, ClientError
        try:
            info = Client().info(".")
            return info["url"].split("/")[-1], info["revision"].number

        except ClientError:
            # Not an svn working dir
            #sys.stderr.write("""Hmm, doesn't look like an SVN repo""")
            pass

    except ImportError:
        #sys.stderr.write(" * please consider installing pysvn\n")
        #sys.stderr.write("""*** please install pysvn:
        #- debian: sudo apt-get install python-svn
        #- windows: http://pysvn.tigris.org/project_downloads.html
#***""")
        pass


    branch_name = None
    revision = None
    try:
        platform = plat_id()
        #
        # svn executable is found, either linux or win with svn
        #
        if re.match('^linux.*', platform) or which('svn'):
            svn_proc = subprocess.Popen(['svn','info'], stdout=subprocess.PIPE, stderr=subprocess.PIPE)
            svn_stdout = svn_proc.communicate()[0]
            if svn_proc.returncode == 0:
                for l in re.split('(?:\r\n|\n)',svn_stdout):
                    m = re.match('([^:]+): (.*)',l)
                    if m:
                        if m.group(1) == 'URL':
                            url = m.group(2)
                            purl = urlparse(url)
                            (head,tail) = os.path.split(purl.path)
                            branch_name = tail
                        elif m.group(1) == 'Revision':
                            revision = m.group(2)

                    else:
                        #print 'nomatch', l
                        pass

        #
        # Try tortoise
        #
        elif re.match('^win.*', platform):
            svn_proc = subprocess.Popen([SUBWCREV,'.'], stdout=subprocess.PIPE, stderr=subprocess.PIPE)
            svn_stdout = svn_proc.communicate()[0]
            if svn_proc.returncode == 0:
                for l in re.split('(?:\r\n|\n)',svn_stdout):
                    m = re.match('Updated to revision (\d+)',l)
                    if m:
                        revision = m.group(1)
                        break

                (head, tail) = os.path.split(os.getcwd())
                branch_name = tail

    except OSError, e:
        #errmsg = sys.argv[0] + ': couldn\'t find branch name\n'
        #sys.stderr.write(errmsg)
        #sys.stderr.write(str(e))
        pass

    return branch_name, revision


def branch_name():
    '''Guesses branch name'''
    branch_name, revision = _get_svn_info()
    #
    # Try with git
    #
    if not branch_name:
        branch_name, revision = _get_git_info()
    #
    # Try hg
    #
    if not branch_name:
        branch_name, revision = _get_hg_info()

    if branch_name or revision:
        return '{0}@{1}'.format(branch_name,revision)
    else:
        return None


def which(program):
    """Unix like which, to show where is an executable in path"""
    def is_exe(fpath):
        return os.path.exists(fpath) and os.access(fpath, os.X_OK)

    fpath, fname = os.path.split(program)
    if fpath:
        if is_exe(program):
            return program
    else:
        for path in os.environ["PATH"].split(os.pathsep):
            exe_file = os.path.join(path, program)
            if is_exe(exe_file):
                return exe_file

    return None






def find_files_re(path, regex=None):
    """Find files in path matching regex, returns a list"""
    if not os.path.isdir(path):
        raise RuntimeError('{0}: not a directory'.format(path))

    result = []
    cregex = None
    if regex:
        cregex = re.compile(regex)

    for dirpath, dirnames, filenames in os.walk(path):
        #print dirpath, dirnames, filenames
        if re.search('\.[^/]', dirpath):
            #sys.stderr.write('Skipping {0}\n'.format(dirpath))
            continue

        for f in filenames:
            #print 'match {0} against {1}'.format(f,sys.argv[2])
            if cregex and cregex.match(f):
                #print 'match: ',f
                full_file_path = os.path.join(dirpath, f)
                result.append(full_file_path)

    return result

def xmkdir(d):
    rev_path_list = list()
    head = d
    while len(head) and head != os.sep:
        rev_path_list.append(head)
        (head, tail) = os.path.split(head)

    rev_path_list.reverse()
    for p in rev_path_list:
        try:
            os.mkdir(p)
        except OSError, e:
            if e.errno != 17:
                raise


def main():
    """Will print plat_id and branch_name"""
    if len(sys.argv) == 2:
        if sys.argv[1] == 'branch_name':
            print branch_name()
        elif sys.argv[1] == 'plat_id':
            print plat_id()
    else:
        print plat_id()
        print branch_name()
    return


if __name__ == '__main__':
    main()
