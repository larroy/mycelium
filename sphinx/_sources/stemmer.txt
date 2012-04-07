Stemmer
=======

The stemmer is from http://snowball.tartarus.org/download.php

It can be used from python as:

::

    >>> import stemmer
    >>> help(stemmer)
    >>> s = stemmer.Stemmer('english')
    >>> s.stem('greately')
    'great'
