HTML lexer
==========

This module parses HTML markup, extracts a textual representation in UTF-8 encoding, extracts links to other pages, rss / atom feeds if any. The resulting information of lexing an html document is stored in two data structures:

* ProcHTML
* Analysis

This is an example of how to use the common module from Python:

::

    piotr@gomasio:0:~/devel/mycelium/dist$ python
    Python 2.7.2+ (default, Oct  4 2011, 20:06:09) 
    [GCC 4.6.1] on linux2
    Type "help", "copyright", "credits" or "license" for more information.
    >>> import common
    >>> res = common.html_lex('<html><head><title>Hi there</title></head><body>my body is great</body></html>', 'http://example.com')
    >>> res.text
    '\nHi there\nmy body is great'
    >>> res.analysis.title
    'Hi there'
    >>> help(common)
    ...
    
