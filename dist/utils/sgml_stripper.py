import re

import Entity_handler

entity_h = Entity_handler.Entity_handler()
strip_tags_re = re.compile(r'<[^>]*?>')
entity_re = re.compile(r'&#?[-\w\d:.]+;')


def sgml_stripper(s):
    if s is None or (type(s) != type(str()) and type(s) != type(unicode())):
        return ''

    s = strip_tags_re.sub('',s)
    m = entity_re.search(s)
    if not m:
        return s

    was_unicode = False
    if type(s)==type(unicode()):
        was_unicode = True
        s = s.encode('utf-8')

    s = entity_h.replace_all_entities(s)
    if was_unicode:
        s = s.decode('utf-8')

    return s

