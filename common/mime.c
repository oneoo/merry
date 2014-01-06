#include "mime.h"

static mime_t *mime_types[26];
static int inited = 0;

static void add_mime_type(char *ext, char *type)
{
    int ext_len = strlen(ext);
    int type_len = strlen(type);
    mime_t *n = malloc(sizeof(mime_t) + ext_len + 2 + type_len);

    n->next = NULL;
    n->ext = (char *) n + sizeof(mime_t);
    n->type = n->ext + ext_len + 1;

    memcpy(n->ext, ext, ext_len);
    memcpy(n->type, type, type_len);

    n->ext[ext_len] = '\0';
    n->type[type_len] = '\0';

    int k = (tolower(ext[0]) - 'a') % 26;

    if(mime_types[k] == NULL) {
        mime_types[k] = n;

    } else {
        n->next = mime_types[k];
        mime_types[k] = n;
    }
}

void init_mime_types()
{
    inited = 1;
    int i = 0;

    for(i = 0; i < 26; i++) {
        mime_types[i] = NULL;
    }

    add_mime_type("txt", "text/plain");
    add_mime_type("htm", "text/html");
    add_mime_type("html", "text/html");
    add_mime_type("shtml", "text/html");
    add_mime_type("xhtml", "application/xhtml+xml");
    add_mime_type("js", "text/javascript");
    add_mime_type("json", "application/json");
    add_mime_type("atom", "application/atom+xml");
    add_mime_type("css", "text/css");
    add_mime_type("rss", "text/xml");
    add_mime_type("xml", "text/xml");

    add_mime_type("jpg", "image/jpeg");
    add_mime_type("jpeg", "image/jpeg");
    add_mime_type("gif", "image/gif");
    add_mime_type("png", "image/png");
    add_mime_type("ico", "image/x-icon");
    add_mime_type("jng", "image/x-jng");
    add_mime_type("bmp", "image/bmp");
    add_mime_type("webp", "image/webp");
    add_mime_type("wbmp", "image/vnd.wap.wbmp");
    add_mime_type("tif", "image/tiff");
    add_mime_type("tiff", "image/tiff");

    add_mime_type("mid", "audio/mid");
    add_mime_type("midi", "audio/mid");
    add_mime_type("kar", "audio/mid");
    add_mime_type("mp3", "audio/mpeg");
    add_mime_type("oga", "audio/ogg");
    add_mime_type("ogg", "audio/ogg");
    add_mime_type("m4a", "audio/x-m4a");
    add_mime_type("ra", "audio/x-realaudio");
    add_mime_type("wav", "audio/x-wav");
    add_mime_type("mp4a", "audio/mp4");

    add_mime_type("3gp", "video/3gpp");
    add_mime_type("3gpp", "video/3gpp");
    add_mime_type("mpeg", "video/mpeg");
    add_mime_type("mpg", "video/mpeg");
    add_mime_type("ogv", "video/ogg");
    add_mime_type("mov", "video/quicktime");
    add_mime_type("webm", "video/webm");
    add_mime_type("flv", "video/x-flv");
    add_mime_type("m4v", "video/x-m4v");
    add_mime_type("mng", "video/x-mng");
    add_mime_type("asx", "video/x-ms-asf");
    add_mime_type("asf", "video/x-ms-asf");
    add_mime_type("wmv", "video/x-ms-wmv");
    add_mime_type("avi", "video/x-msvideo");
    add_mime_type("mp4", "application/mp4");

    add_mime_type("manifest", "text/cache-manifest");
    add_mime_type("mml", "text/mathml");
    add_mime_type("jad", "text/vnd.sun.j2me.app-descriptor");
    add_mime_type("wml", "text/vnd.wap.wml");
    add_mime_type("htc", "text/x-component");

    add_mime_type("jar", "application/java-archive");
    add_mime_type("war", "application/java-archive");
    add_mime_type("ear", "application/java-archive");
    add_mime_type("7z", "application/x-7z-compressed");
    add_mime_type("zip", "application/zip");
    add_mime_type("tar", "application/x-tar");
    add_mime_type("gz", "application/x-gzip");
    add_mime_type("bz2", "application/x-bzip");
    add_mime_type("tbz", "application/x-bzip-compressed-tar");
    add_mime_type("rar", "application/x-rar-compressed");
    add_mime_type("xpi", "application/x-xpinstall");
    add_mime_type("rpm", "application/x-redhat-package-manager");
    add_mime_type("crx", "application/x-chrome-extension");
    add_mime_type("deb", "application/application/x-debian-package");
    add_mime_type("rtf", "application/rtf");
    add_mime_type("ps", "application/postscript");
    add_mime_type("eps", "application/postscript");
    add_mime_type("ai", "application/postscript");
    add_mime_type("pdf", "application/pdf");
    add_mime_type("swf", "application/x-shockwave-flash");
    add_mime_type("der", "application/x-x509-ca-cert");
    add_mime_type("pem", "application/x-x509-ca-cert");
    add_mime_type("crt", "application/x-x509-ca-cert");


    add_mime_type("woff", "application/x-woff");
    add_mime_type("ttc", "font/truetype");
    add_mime_type("ttf", "font/truetype");
    add_mime_type("otf", "font/opentype");
    add_mime_type("svg", "image/svg+xml");
    add_mime_type("svgz", "image/svg+xml");

    add_mime_type("lua", "text/plain");
    add_mime_type("php", "text/plain");
    add_mime_type("py", "text/plain");
    add_mime_type("rb", "text/plain");
    add_mime_type("java", "text/plain");
    add_mime_type("log", "text/plain");

    add_mime_type("doc", "application/msword");
    add_mime_type("xls", "application/vnd.ms-excel");
    add_mime_type("ppt", "application/vnd.ms-powerpoint");
    add_mime_type("docx",
                  "application/vnd.openxmlformats-officedocument.wordprocessingml.document");
    add_mime_type("pptx",
                  "application/vnd.openxmlformats-officedocument.presentationml.presentation");
    add_mime_type("xlsx",
                  "application/vnd.openxmlformats-officedocument.spreadsheetml.sheet");
}

const char *get_mime_type(const char *filename)
{
    if(inited == 0) {
        init_mime_types();
    }

    int i = 0;
    const char *ext = filename;
    int l = strlen(filename);

    for(i = l; i--;) {
        if(ext[i] == '.') {
            ext = &ext[i + 1];

            break;
        }
    }

    char *t = "application/octet-stream";
    mime_t *u = mime_types[(tolower(ext[0]) - 'a') % 26];

    while(u) {
        if(stricmp(u->ext, ext) == 0) {
            return u->type;
        }

        u = u->next;
    }

    return t;
}
