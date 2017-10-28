#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>

ngx_int_t body_filter(ngx_module_t module, ngx_http_request_t *r, ngx_chain_t *in, ngx_int_t (*next)(ngx_http_request_t *r, ngx_chain_t *in));