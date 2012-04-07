import re
import htmlentitydefs

# http://www.w3.org/International/questions/qa-forms-utf-8.en.php
valid_utf8_re = re.compile(r"""
(?:
    [\x07-\x0D\x20-\x7E]             # ASCII
   | [\xC2-\xDF][\x80-\xBF]             # non-overlong 2-byte
   |  \xE0[\xA0-\xBF][\x80-\xBF]        # excluding overlongs
   | [\xE1-\xEC\xEE\xEF][\x80-\xBF]{2}  # straight 3-byte
   |  \xED[\x80-\x9F][\x80-\xBF]        # excluding surrogates
   |  \xF0[\x90-\xBF][\x80-\xBF]{2}     # planes 1-3
   | [\xF1-\xF3][\x80-\xBF]{3}          # planes 4-15
   |  \xF4[\x80-\x8F][\x80-\xBF]{2}     # plane 16
)*\Z
""", re.X)

def valid_utf8(str):
    """Returns true if the sequence is unicode or a valid utf-8 string"""
    if type(str) == type(unicode()):
        return True
    if valid_utf8_re.match(str):
        return True
    else:
        return False

def eu8(s):
    """If s is unicode will encode to utf-8"""
    if type(s) == type(unicode()):
        return s.encode('utf-8')

def deu8(s):
    if type(s) == type(unicode()):
        return s
    return s.decode('utf-8')


##
#
# @param text The HTML (or XML) source text.
# @return The plain text, as a Unicode string, if necessary.
def unescape(text):
    """Removes HTML or XML character references and entities from a text string."""
    def fixup(m):
        text = m.group(0)
        if text[:2] == "&#":
            # character reference
            try:
                if text[:3] == "&#x":
                    return unichr(int(text[3:-1], 16))
                else:
                    return unichr(int(text[2:-1]))
            except ValueError:
                pass
        else:
            # named entity
            try:
                text = unichr(htmlentitydefs.name2codepoint[text[1:-1]])
            except KeyError:
                pass
        return text # leave as is
    return re.sub("&#?\w+;", fixup, text)


tokenize_s = re.compile(r' +', re.UNICODE)
tokenize_f = re.compile(r'^\w+$', re.UNICODE)
def tokenize(txt):
    assert( type(txt) == type(unicode()) )
    return filter(lambda x: tokenize_f.match(x), tokenize_s.split(txt))


word_tokenize_re = re.compile(r'(\s+|\'s|-|\'m|\'d|\x21-\x2F|\W)', re.UNICODE)
word_tokenize_token_re = re.compile(r'^(:?\w+|\'|\'s|\'m|\'d)$', re.UNICODE)
def word_tokenize(txt):
    """Breaks text into tokens, returns words, numbers and contracted words separately"""
    txt = deu8(txt)
    return filter(lambda x: word_tokenize_token_re.match(x), word_tokenize_re.split(txt))



def uniq(seq):
    """unique elements from a sequence (stable)"""
    s = {}
    res = []
    for i in seq:
        if not s.has_key(i):
            res.append(i)
            s[i] = None
    return res


def xmkdir(d):
    """Create directories recursively for a given path with multiple components such as a/b/c"""
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


