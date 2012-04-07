The HTTP Crawler
================

The web crawler is programmed in C++. Uses libevent for asynchronous IO and curl to handle details of HTTP transfers. It's designed to handle thousands of concurrent connections. It has a mechanism (Url_classifier) to queue request for each host separately, so it never hammers a single DNS name with more than one connection. It also respects hosts.txt

When started it listens on a TCP port for http urls to retrieve. You can pipe urls to this port, one per line and they will be queued for retrieval.

You can pipe urls with netcat, for example:

.. code::
# start the crawler
build/release/crawler/crawler
# pipe some urls
cat urls.txt | nc localhost 1024


The environment variables that affect some configuration parameters are:

* Specific for the crawler:
 - MYCELIUM_CRAWLER_PORT: port to listen for urls
 - MYCELIUM_CRAWLER_PARALLEL: number of parallel crawlers to run

* General for all the tools that interact with the DB:

 - MYCELIUM_DB_HOST: mongodb host for storing the documents, default is "localhost"
 - MYCELIUM_DB_NS: database.collection, defaults to "mycelium.crawl"


The crawler periodically prints on stdout the amount of downloaded data, the bitrate that it's downloading in that interval of time, and the number of documents retrieved.

Downloaded: 0.00 iB rate: 0.00 KB/s docs: 0


Interactive commands:

There are some interactive commands to check what's going on with the crawler on realtime. You can see the list of commands by typing 'help' in the console where the crawler is running:

help
commands: qlen dumpq reschedule status help quit

* qlen: shows the number of urls enqueued in each queue
* dumpq: shows the actual urls in each queue, you can see that they are grouped by host
* reschedule: reschedule idle workers, there should be no need to do this during normal usage.
* status: see the state of each worker {ROBOTS, CONTENT, IDLE}, the time spent in the last state and the current url.
* quit: the crawler will exit.

