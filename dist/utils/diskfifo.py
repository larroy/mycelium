import tempfile
import cPickle
import os

class DiskFifo:
    """A disk based FIFO which can be iterated, appended and extended in an interleaved way"""

    def __init__(self, filename = None):
        """If the filename parameter is passed it will create or use the given file, otherwise it uses a temporary file"""

        if filename is None:
            self.fd = tempfile.TemporaryFile()
            self.wpos = 0
        else:
            self.fd = open(filename, 'a+b')
            self.fd.seek(0, os.SEEK_END)
            self.wpos = self.fd.tell()
        self.rpos = 0
        self.pickler = cPickle.Pickler(self.fd)
        self.unpickler = cPickle.Unpickler(self.fd)
        self.size = 0

    def __len__(self):
        return self.size

    def extend(self, sequence):
        map(self.append, sequence)

    def append(self, x):
        self.fd.seek(self.wpos)
        self.pickler.clear_memo()
        self.pickler.dump(x)
        self.wpos = self.fd.tell()
        self.size = self.size + 1

    def next(self):
        try:
            self.fd.seek(self.rpos)
            x = self.unpickler.load()
            self.rpos = self.fd.tell()
            return x

        except EOFError:
            raise StopIteration

    def __iter__(self):
        self.rpos = 0
        return self

