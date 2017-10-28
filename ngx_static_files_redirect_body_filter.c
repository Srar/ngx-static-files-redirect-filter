#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>

#include "ngx_static_files_redirect_tools.h"
#include "ngx_static_files_redicect_config.h"
#include "ngx_static_files_redicect_context.h"
#include "ngx_static_files_redirect_tag_tools.h"
#include "ngx_static_files_redirect_body_filter.h"

typedef struct {
    ngx_int_t start;
    ngx_int_t end;
    ngx_int_t length;

    ngx_str_t *new_url;
} ngx_static_redicect_render_item; 

ngx_int_t body_filter(ngx_module_t module, ngx_http_request_t *r, ngx_chain_t *in, ngx_int_t (*next)(ngx_http_request_t *r, ngx_chain_t *in)) {

    // clock_t begin = clock();
    
    ngx_static_redicect_context *context; 
    ngx_static_redicect_filter_config *config;
    
    context = ngx_http_get_module_ctx(r, module);
    config = ngx_http_get_module_loc_conf(r, module);
    
    if(!context -> enable) return next(r, in);
    if (in == NULL) return next(r, in);

    ngx_chain_add_copy(r -> pool, &context -> html_chain, in);
    if(is_last_chain(in) != 1) return NGX_OK;

    /* 将多个chain内的buffer全部合并成一个u_char */
    ngx_str_t *html = link_chains(r -> pool, context -> html_chain);

    /* 用户请求的uri */
    ngx_str_t *uri  = create_ngx_string(r -> pool, substring(r -> pool, (char*)r -> uri_start, 0, (int)(r -> uri_end - r -> uri_start)));

    /* 待重新拼接的html tags */
    ngx_int_t redicecting_array_len = 0;
    ngx_static_redicect_render_item* redicecting_array = NULL;

    if(context -> is_html) {
        ngx_static_redicect_regex_search_result_t *tags_array = regex_search(r -> pool, config -> html_regex -> regex, html);
        redicecting_array = ngx_palloc(r -> pool, sizeof(ngx_static_redicect_render_item) * tags_array -> len);
        for(int i = 0; i < tags_array -> len; i++) {
            ngx_static_redicect_regex_search_result *tag = &tags_array -> array[i];    
            ngx_static_redicect_regex_search_result *property_url = search_html_tag_property(r -> pool, tag -> str, create_ngx_string(r -> pool, "src"));
            if(property_url == NULL) 
                property_url = search_html_tag_property(r -> pool, tag -> str, create_ngx_string(r -> pool, "href"));
            if(property_url == NULL) continue;
            ngx_str_t *result = redirect_static_file(
                r -> pool, 
                r -> http_connection -> ssl, 
                &r -> headers_in.host -> value, uri, property_url -> str);

            if(result == NULL) continue;
                
            if(ngx_regex_exec(config -> file_extension_regex -> regex, result, NULL, 0) == NGX_REGEX_NO_MATCHED) continue;

            ngx_str_t *url = create_ngx_string(r -> pool, (char*)config -> host.data);
            
            url = ngx_append(r -> pool, url, &config -> split_tag);

            if(config -> base64_url == 1) {
                ngx_str_t * base64_url = ngx_palloc(r -> pool, sizeof(ngx_str_t));
                base64_url -> len  = 0;
                base64_url -> data = ngx_palloc(r -> pool, ngx_base64_encoded_length(result -> len)); 
                ngx_encode_base64url(base64_url, result);   
                url = ngx_append(r -> pool, url, base64_url);
            } else {
                url = ngx_append(r -> pool, url, result);
            }

            redicecting_array[redicecting_array_len].start   = tag -> start + property_url -> start;
            redicecting_array[redicecting_array_len].end     = tag -> start + property_url -> start + property_url -> str -> len;
            redicecting_array[redicecting_array_len].length  = redicecting_array[redicecting_array_len].end - redicecting_array[redicecting_array_len].start;
            redicecting_array[redicecting_array_len].new_url = url;

            // printf("%s\n", (char*)url -> data);
            redicecting_array_len++;
        }
        // printf("tags: %ld after tags: %ld\n", tags_array -> len, redicecting_array_len);
    }   

    ngx_int_t   render_offset = 0;
    ngx_buf_t   *render_buf   = NULL; 
    ngx_chain_t *render_chain = ngx_alloc_chain_link(r -> pool);
    render_chain -> buf  = NULL;
    render_chain -> next = NULL;

    for(ngx_int_t i = 0; i < redicecting_array_len; i++) {
        ngx_static_redicect_render_item item = redicecting_array[i];
        render_buf = ngx_create_temp_buf(r -> pool, item.start - render_offset);
        render_buf -> pos    = html -> data + render_offset;
        render_buf -> last   = html -> data + item.start;
        add_buf_to_last_of_chain(r -> pool, render_chain, render_buf);
        render_offset = item.end;
        add_str_to_last_of_chain(r -> pool, render_chain, (char*)item.new_url -> data, 0, item.new_url -> len, false);
        if(i + 1 == redicecting_array_len) {
            render_buf = ngx_create_temp_buf(r -> pool, html -> len - item.end);
            render_buf -> pos    = html -> data + item.start;
            render_buf -> last   = html -> data + html -> len;
            render_buf -> last_buf = true;
            render_buf -> last_in_chain = true;
            add_buf_to_last_of_chain(r -> pool, render_chain, render_buf);
        }
    }

    // clock_t end = clock();
    // double time_spent = (double)(end - begin) / CLOCKS_PER_SEC;
    // printf("%f sec.\n", time_spent);
    return next(r, render_chain);
}