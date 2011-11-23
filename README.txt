The Mycelium high performance web crawler
=========================================

This is a high performance web crawler which uses asyncrhonous IO through
libevent so it can handle thousands of connections; addresses the [C10K problem](
http://www.kegel.com/c10k.html).

It handles robots.txt. It mantains a single session per dns host, so it can
crawl thousands of hosts in parallel without hammering the same host.

It's highly recommended to use curl compiled with asyncrhonous DNS (libares),
otherwise DNS resolving inside curl will block the program.

How to use it
-------------

- Compile the sources with SCons
- Then execute the crawler binary and pipe urls to the crawler.fifo fifo.

- The results are stored in the disk via the 'bighash' utility. The urls are
  hashed with sha1 checksum.  And the retrieved documents are stored gzipped,
  so it's easy to pick the results with other software. The environment
  variable DB_DIR controls where the documents are stored.

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

Now you can start downloading urls by piping to the crawler.fifo as in:

<code>
echo 'http://slashdot.org' > crawler.fifo
</code>


<pre>
2011-11-23 00:54:11,408 - INFO  crawlog - handle id: 0 retrieving robots: slashdot.org
down: 0.00 iB 0.00 KB/s
2011-11-23 00:54:11,838 - INFO  crawlog - handle id: 0 DONE: http://slashdot.org/robots.txt HTTP 200
2011-11-23 00:54:11,838 - INFO  crawlog - handle id: 0 retrieving content: slashdot.org
2011-11-23 00:54:13,777 - INFO  crawlog - handle id: 0 DONE: http://slashdot.org/ HTTP 200
</pre>

The results are stored like:

<pre>
mycelium_db/d3/d5304d22690a893b6bc5aceb17cbfb6287adf6-0/
├── contents.gz
├── __key__
└── __value__


Where __key__ is the url, __value__ should contain the HTTP headers, and
contents.gz is the retrieved content, if any, gzipped.
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



The results are stored in disk directory as a 'bighash' database which
makes it easy to access the documents downloaded.

The DB_DIR environment variable controls this directory if you want to change
the default.

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

