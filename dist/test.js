function purl(x) {
    print('url: ' + x['url'])
    print(x['content'])
}
db.crawl.find().forEach(purl)
