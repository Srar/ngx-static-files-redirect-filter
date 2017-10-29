#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>

#ifndef _NGX_STATIC_REDIRECT_CONTEXT_INCLUDE_
#define _NGX_STATIC_REDIRECT_CONTEXT_INCLUDE_

typedef struct {
    bool enable;
    ngx_chain_t* html_chain;
} ngx_static_redicect_context;

#endif