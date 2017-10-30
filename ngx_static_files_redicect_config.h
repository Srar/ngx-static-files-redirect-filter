#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>

#include <pcre.h>

#ifndef _NGX_STATIC_REDIRECT_CONFIG_INCLUDE_
#define _NGX_STATIC_REDIRECT_CONFIG_INCLUDE_

typedef struct {
    ngx_str_t  new_host;
    ngx_str_t  split_tag;
    ngx_flag_t take_src_host;
    ngx_flag_t base64_src_host;
    ngx_flag_t base64_src_url;
    ngx_flag_t enable;
    ngx_flag_t utf8_content;

    ngx_int_t buffer_size_limit;

    ngx_int_t ramdom_domain_minmum;
    ngx_int_t ramdom_domain_maxmum;

    ngx_regex_compile_t *html_regex; 
    ngx_regex_compile_t *file_extension_regex; 
} ngx_static_redicect_filter_config;

#endif