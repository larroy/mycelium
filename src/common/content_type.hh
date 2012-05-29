#pragma once

namespace content_type {
    typedef enum content_type_t {
        UNSET=0,

        // No Content-type header
        NONE, 

        // Content-type header that we aren't interested
        UNRECOGNIZED,

        // text/html
        TEXT_HTML,

        // application/xhtml+xml
        XHTML,

        // application/rss+xml
        RSS_XML,

        // application/atom+xml
        ATOM_XML,

        // text/xml
        TEXT_XML,

        // text/plain
        TEXT_PLAIN,

        // application/pdf
        APPLICATION_PDF,

        FILE_DIRECTORY,

        // Empty file, used for local files 
        EMPTY,

        CONTENT_TYPE_MAX

    } content_type_t;
};    


