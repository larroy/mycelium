The Mycelium high performance web crawler
=========================================

This is a high performance web crawler which uses asyncrhonous IO through
libevent so it can handle thousands of connections.

It's highly recommended to use curl compiled with asyncrhonous DNS (libares)


It handles robots.txt

How to use it:

- Compile the sources with SCons, then execute the binary and pipe urls to the
  crawler.fifo fifo.

The results are stored in the DB_DIR directory as a 'bighash' database which
makes it easy to access the documents downloaded.

