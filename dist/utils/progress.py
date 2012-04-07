import datetime
import sys

def est_finish(started, done, total):
    """Return a datetime object estimating date of finishing given start time,
    number of elements done, and total elements"""

    if not total or total <= 0 or done <= 0:
        return datetime.datetime.now()
    delta = datetime.datetime.now() - started
    remaining = (delta.total_seconds() * total) / done
    res = datetime.datetime.now() + datetime.timedelta(seconds=remaining)
    return res.strftime('%Y-%m-%d %H:%M')

def getTerminalSize():
    """Returns an (x,y) tuple with the size of the terminal"""

    def ioctl_GWINSZ(fd):
        try:
            import fcntl, termios, struct
            cr = struct.unpack('hh', fcntl.ioctl(fd, termios.TIOCGWINSZ,
        '1234'))
        except:
            return None
        return cr
    cr = ioctl_GWINSZ(0) or ioctl_GWINSZ(1) or ioctl_GWINSZ(2)
    if not cr:
        try:
            fd = os.open(os.ctermid(), os.O_RDONLY)
            cr = ioctl_GWINSZ(fd)
            os.close(fd)
        except:
            pass
    if not cr:
        try:
            cr = (env['LINES'], env['COLUMNS'])
        except:
            cr = (25, 80)
    return int(cr[1]), int(cr[0])




## {{{ http://code.activestate.com/recipes/168639/ (r1)
class progressBar:
    def __init__(self, minValue = 0, maxValue = 10, totalWidth=getTerminalSize()[0]):
        """Creates a callable progressBar object"""
        self.progBar = "[]"   # This holds the progress bar string
        self.min = minValue
        self.max = maxValue
        self.span = maxValue - minValue
        self.width = totalWidth
        self.done = 0
        self.percentDone_last = -1
        self.percentDone = 0
        self.updateAmount(0)  # Build progress bar string
        self.lastMsg = ''

    def updateAmount(self, done = 0, msg=''):
        if done < self.min:
            done = self.min

        if done > self.max:
            done = self.max

        self.done = done

        if self.span == 0:
            self.percentDone = 100
        else:
            self.percentDone = int(((self.done - self.min) * 100) / self.span)

        if self.percentDone == self.percentDone_last and self.lastMsg == msg:
            return False
        else:
            self.percentDone_last = self.percentDone

        # Figure out how many hash bars the percentage should be
        allFull = self.width - 2
        numHashes = (self.percentDone / 100.0) * allFull
        numHashes = int(round(numHashes))

        # build a progress bar with hashes and spaces
        self.progBar = "[" + '#'*numHashes + ' '*(allFull-numHashes) + "]"

        # figure out where to put the percentage, roughly centered
        if not msg:
            percentString = str(self.percentDone) + "%"
        else:
            percentString = str(self.percentDone) + "%" + ' ' + msg

        percentString = '{0}% {1}'.format(str(self.percentDone),msg)

        percentPlace = len(self.progBar)/2 - len(percentString)/2
        if percentPlace < 0:
            percentPlace = 0

        # slice the percentage into the bar
        self.progBar = self.progBar[0:percentPlace] + percentString[:allFull] + self.progBar[percentPlace+len(percentString):]
        return True

    def __call__(self, amt, msg='', force=False):
        if self.updateAmount(amt, msg):
            sys.stdout.write(str(self))
            sys.stdout.write("\r")
            sys.stdout.flush()


    def __str__(self):
        return str(self.progBar)
## end of http://code.activestate.com/recipes/168639/ }}}




