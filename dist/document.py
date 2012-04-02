import os
import minimongo

class Doc(minimongo.Model):
    class Meta:
        host = os.environ.get('MYCELIUM_DB_HOST', 'localhost')
        if os.environ.has_key('MYCELIUM_DB_PORT'):
            port = os.environ.get('MYCELIUM_DB_PORT')
        (database, collection) = os.environ.get('MYCELIUM_DB_NS', 'mycelium.crawl').split('.')
        indices = (
            minimongo.Index('url'),
        )

    def __init__(self):
        pass

