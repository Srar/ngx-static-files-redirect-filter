#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>

#include "ngx_static_files_redirect_tools.h"
#include "ngx_static_files_redicect_config.h"
#include "ngx_static_files_redicect_context.h"
#include "ngx_static_files_redirect_header_filter.h"
#include "ngx_static_files_redicect_content_types.h"

ngx_int_t header_filter(ngx_module_t module, ngx_http_request_t *r, ngx_int_t (*next)(ngx_http_request_t *r)) 
{
    ngx_static_redicect_context *context;
    ngx_static_redicect_filter_config *config;

    // printf("========= status %d ========= \n", (int)r -> headers_out.status);
    // printf("url: %s \n", substring(r -> pool, (char*)r -> uri_start, 0, (char*)r -> uri_end - (char*)r -> uri_start));
    
    config = ngx_http_get_module_loc_conf(r, module);
    context = ngx_http_get_module_ctx(r, module);
    if(context == NULL) {
        context = ngx_palloc(r -> pool, sizeof(ngx_static_redicect_context));
        ngx_http_set_ctx(r, context, module);
    }
    context -> enable = config -> enable;
    
    if(r -> headers_out.status != NGX_HTTP_OK || context -> enable == 0) {
        context -> enable = 0;
        return next(r);
    }

    for(int i = 0; i < ngx_static_redicect_content_types_len; i++ ) {
        ngx_str_t content_type = ngx_static_redicect_content_types[i];
        if(r -> headers_out.content_type.len < content_type.len) continue;
        if(ngx_strstr(r -> headers_out.content_type.data, content_type.data) != NULL) {
            context -> is_css = i == 0;
            context -> is_javascript = i == 1;
            context -> is_html = i == 2;
            context -> html_chain = NULL;
            context -> enable = true;
            r -> filter_need_temporary = 1;
            r -> filter_need_in_memory = 1;
            r -> allow_ranges = 0;
            ngx_http_clear_content_length(r);
            ngx_http_clear_accept_ranges(r);
            break;
        }
        if(i + 1 == ngx_static_redicect_content_types_len) context -> enable = 0;
    }

    return next(r);
}