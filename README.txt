The Mycelium Information retrieval system
=========================================

Check the latest up-to-date user documentation in http://pedro.larroy.com/mycelium/sphinx/

For the impatient
-----------------
$ ./bootstrap.sh
$ ./build.py
$ scons
$ build/release/crawler
$ echo 'http://google.com' | nc localhost 1024


How to use it
-------------

- Initialize the git submodules:

$ git submodule init
$ git submodule update


- Build the 3rd-party libraries:

$ ./build.py

- Compile the sources with SCons:

$ scons

- Alternatively you might build with system curl:

$  scons --system_curl

- But as said previously, synchronous DNS resolving will harm the performance
  and block. So it's  not recommended unless curl has been compiled with
  c-ares, as it will be done by build.py

Running
-------

- The environment variables that affect some configuration parameters are:
    * Specific for the crawler:
        MYCELIUM_CRAWLER_PORT: port to listen for urls
        MYCELIUM_CRAWLER_PARALLEL: number of parallel crawlers to run

    * General for all the tools that interact with the DB:

    MYCELIUM_DB_HOST: mongodb host for storing the documents, default is "localhost"
    MYCELIUM_DB_NS: database.collection, defaults to "mycelium.crawl"

Dependencies
============

The software is build on debian / ubuntu systems, although it should be fairly
easy to port to other platforms.

Some (might be incomplete) list of libraries that are required:

- z
- boost_filesystem
- boost_system
- boost_regex
- log4cxx
- pthread
- curl
- event
- ssl
- libidn11-dev

Other software:

- scons
- flex
- autoconf (for building curl and c-ares)


Troubleshoting
==============

AttributeError: 'SConsEnvironment' object has no attribute 'CXXFile':
File "/home/piotr/devel/mycelium/SConstruct", line 210:
variant_dir='build/{0}'.format(build), duplicate=0)
File "/usr/lib/scons/SCons/Script/SConscript.py", line 614:
return method(*args, **kw)
File "/usr/lib/scons/SCons/Script/SConscript.py", line 551:
return _SConscript(self.fs, *files, **subst_kw)
File "/usr/lib/scons/SCons/Script/SConscript.py", line 260:
exec _file_ in call_stack[-1].globals
File "/home/piotr/devel/mycelium/src/SConscript", line 11:
env.CXXFile(target='Robots_flex.cc',source='robots.ll')

To solve this problem make sure flex is installed.

