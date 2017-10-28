#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>

#ifndef _NGX_STATIC_REDIRECT_TYPES_INCLUDE_
#define _NGX_STATIC_REDIRECT_TYPES_INCLUDE_

static ngx_str_t ngx_static_redicect_content_types[] = {
    ngx_string("text/css"),
    ngx_string("application/javascript"),
    ngx_string("text/html"),
};
static ngx_int_t ngx_static_redicect_content_types_len = sizeof(ngx_static_redicect_content_types) / sizeof(ngx_str_t);

#endif