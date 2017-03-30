#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>

static char *ngx_http_header_hash(ngx_conf_t *cf, 
                                  ngx_command_t *cmd,
                                  void *conf);

/* Module directive */
static ngx_command_t  ngx_http_header_hash_commands[] = {
    {
        ngx_string("header_hash"),
        NGX_HTTP_LOC_CONF|NGX_CONF_NOARGS,
        ngx_http_header_hash,
        0,
        0,
        NULL
    },
    ngx_null_command
};

/* Module context */
static ngx_http_module_t  ngx_http_header_hash_module_ctx = {
    NULL,   /* preconfiguration */
    NULL,   /* postconfiguration */

    NULL,   /* create main configuration */
    NULL,   /* init main configuration */

    NULL,   /* create server configuration */
    NULL,   /* merge server configuration */

    NULL,   /* create location configuration */
    NULL,   /* merge location configuration */
};

/* Module definition */
ngx_module_t  ngx_http_header_hash_module = {
    NGX_MODULE_V1,
    &ngx_http_header_hash_module_ctx,   /* module context */
    ngx_http_header_hash_commands,      /* module directives */
    NGX_HTTP_MODULE,                    /* module type */
    NULL,                               /* init master */
    NULL,                               /* init module */
    NULL,                               /* init process */
    NULL,                               /* init thread */
    NULL,                               /* exit thread */
    NULL,                               /* exit process */
    NULL,                               /* exit master */
    NGX_MODULE_V1_PADDING
};

/* D. J. Bernstein hash function */
static size_t 
djb_hash(const char* cp)
{
    size_t hash = 5381;
    while (*cp)
        hash = 33 * hash ^ (unsigned char) *cp++;
    return hash;
}

/* Generate resp body based on headers_in */
static int
ngx_http_header_hash_body_gen(ngx_http_headers_in_t *headers_in,
                              u_char* buf,
                              size_t buf_len)
{
    unsigned char* buf_end;
    size_t djb_hash_val = 0;

    const ngx_str_t* host       = headers_in->host       ? &headers_in->host->value         : NULL;
    const ngx_str_t* user_agent = headers_in->user_agent ? &headers_in->user_agent->value   : NULL;

    //Creat content buf for hashing
    char hash_content[buf_len];
    snprintf(hash_content,buf_len,
             "{\n"
             "\thost:\"%s\"\n"
             "\tuser_agent:\"%s\"\n"
             "}\n",
             host->data,
             user_agent->data);

    djb_hash_val = djb_hash(hash_content); 

    buf_end = ngx_snprintf(buf, buf_len,
                          "Headers hash:\n"
                          "%s\n"
                          "Hash value:\n"
                          "%d\n",
                          hash_content,
                          djb_hash_val
                          );

    return buf_end - buf;
}

static ngx_int_t
ngx_http_header_hash_handler(ngx_http_request_t *r)
{
    ngx_int_t     rc;
    ngx_buf_t    *b;
    ngx_chain_t   out;
    int body_size;

    /* Allocate a new buffer for sending out the reply. */
    b = ngx_calloc_buf(r->pool);
    if (b == NULL) {
        return NGX_HTTP_INTERNAL_SERVER_ERROR;
    }

    /* Insertion in the buffer chain. */
    out.buf = b;
    out.next = NULL;    

    size_t buf_len = 1024;
    u_char *buf = ngx_palloc(r->pool, buf_len);
    if (buf == NULL) {
        return NGX_HTTP_INTERNAL_SERVER_ERROR;
    }
    body_size = ngx_http_header_hash_body_gen(&r->headers_in,
                                              buf,
                                              buf_len);
    if (body_size <= 0) {
        return NGX_HTTP_INTERNAL_SERVER_ERROR;
    }

    b->start = b->pos = buf;
    b->end = b->last = buf + body_size;
    b->memory = 1;
    b->last_buf = 1;
    b->last_in_chain = 1;

    /* Sending the headers for the reply. */
    r->headers_out.status = NGX_HTTP_OK; /* 200 status code */
    /* Get the content length of the body. */
    r->headers_out.content_length_n = body_size;
    
    rc = ngx_http_send_header(r);
    if (rc != NGX_OK) {
        return rc;
    }

    return ngx_http_output_filter(r, &out);
}


static char *
ngx_http_header_hash(ngx_conf_t *cf, ngx_command_t *cmd, void *conf)
{
    ngx_http_core_loc_conf_t  *clcf;

    clcf = ngx_http_conf_get_module_loc_conf(cf, ngx_http_core_module);
    clcf->handler = ngx_http_header_hash_handler;

    return NGX_CONF_OK;
}
