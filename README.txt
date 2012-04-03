The Mycelium Information retrieval system
=========================================

<pre>
   ,---------.       +-----------+
 .(           )      |           |
(   Interwebs..)  <--|  crawler  |--->  MongoDB
 (_         ___)     |           |      mycelium.crawl
   \.------.)        +-----------+

</pre>

This is a high performance web crawler which uses asyncrhonous IO through
libevent so it can handle thousands of connections; addresses the [C10K problem](
http://www.kegel.com/c10k.html).

It handles robots.txt. It mantains a single session per dns host, so it can
crawl thousands of hosts in parallel without hammering the same host.

It listens on a tcp socket for urls to crawl. (By default port 1024)

It saves the crawled documents to mongodb.

It's highly recommended to use curl compiled with asyncrhonous DNS (libares),
otherwise DNS resolving inside curl will block the program.

There's also utilities for indexing local content, currently it only handles
PDF files which are converted to text with pdftotext:
    dists/local_index.py

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

Example / Screenshots:
----------------------

Start the crawler:

<pre>
piotr@gomasio:130:~/devel/mycelium$ build/release/crawler
log4cxx: Large window sizes are not allowed.
2011-11-23 00:43:14,481 - INFO  crawlog - Initializing system...
down: 0.00 iB 0.00 KB/s
down: 0.00 iB 0.00 KB/s
</pre>

The status gets printed every few seconds, displaying the total amount
downloaded and the current download speed.

Now you can start downloading urls by sending them to the control socket:

<code>
zcat feeds.txt.gz | nc localhost 1024
</code>


<pre>
2011-11-23 00:54:11,408 - INFO  crawlog - handle id: 0 retrieving robots: slashdot.org
down: 0.00 iB 0.00 KB/s
2011-11-23 00:54:11,838 - INFO  crawlog - handle id: 0 DONE: http://slashdot.org/robots.txt HTTP 200
2011-11-23 00:54:11,838 - INFO  crawlog - handle id: 0 retrieving content: slashdot.org
2011-11-23 00:54:13,777 - INFO  crawlog - handle id: 0 DONE: http://slashdot.org/ HTTP 200
</pre>

Interactive commands
--------------------

There are a few interactive commands for debug and diagnosis, typing help on
stdin shows them:

<pre>
help
on_read_interactive_cb: read: 5
commands: qlen dumpq reschedule status help quit
down: 0.00 iB 0.00 KB/s
</pre>

- qlen: Shows the amount of urls in the queue distributor, total, and per host
- dumpq: Show the urls in the queues
- reschedule: To force IDLE handles back to work, should not be any need to use
  this unless curl malfunctions, which happened with some versions of curl
- status: Show the status of the internal curl handlers
- quit: obvious

Building
========

Execute:

scons

Dependencies
============

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

