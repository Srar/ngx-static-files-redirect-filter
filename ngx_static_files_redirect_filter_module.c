#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>

#include <pcre.h>
#include <stdio.h>
#include <string.h>
#include <sys/time.h>

#include "ngx_static_files_redirect_tools.h"
#include "ngx_static_files_redicect_config.h"
#include "ngx_static_files_redirect_header_filter.h"
#include "ngx_static_files_redirect_body_filter.h"

static ngx_int_t init_filter(ngx_conf_t *config);
static void* on_config_struct_created(ngx_conf_t *config);
static char* on_config_marged(ngx_conf_t *config, void* parent, void* child);

static ngx_int_t leading_header_filter(ngx_http_request_t *r);
static ngx_int_t leading_body_filter(ngx_http_request_t *r, ngx_chain_t *in);

static ngx_http_output_header_filter_pt ngx_http_next_header_filter;
static ngx_http_output_body_filter_pt ngx_http_next_body_filter;

static ngx_http_module_t context = {
    NULL, /* proconfiguration */
    init_filter, /* postconfiguration */

    NULL, /* create main configuration */
    NULL, /* init main configuration */

    NULL, /* create server configuration */
    NULL, /* merge server configuration */

    // ngx_http_test_create_loc_conf, /* create location configuration */
    on_config_struct_created,
    on_config_marged   /* merge location configuration */
};

static ngx_command_t commands[] = {
    {
        ngx_string("static_redirect"),
        NGX_HTTP_MAIN_CONF | NGX_HTTP_SRV_CONF | NGX_HTTP_LOC_CONF | NGX_HTTP_LMT_CONF | NGX_CONF_TAKE1,
        ngx_conf_set_flag_slot,
        NGX_HTTP_LOC_CONF_OFFSET,
        offsetof(ngx_static_redicect_filter_config, enable),
        NULL,
    },
    {
        ngx_string("static_redirect_utf8_content"),
        NGX_HTTP_MAIN_CONF | NGX_HTTP_SRV_CONF | NGX_HTTP_LOC_CONF | NGX_HTTP_LMT_CONF | NGX_CONF_TAKE1,
        ngx_conf_set_flag_slot,
        NGX_HTTP_LOC_CONF_OFFSET,
        offsetof(ngx_static_redicect_filter_config, utf8_content),
        NULL,
    },
    {
        ngx_string("static_redirect_host"),
        NGX_HTTP_MAIN_CONF | NGX_HTTP_SRV_CONF | NGX_HTTP_LOC_CONF | NGX_HTTP_LMT_CONF | NGX_CONF_TAKE1,
        ngx_conf_set_str_slot,
        NGX_HTTP_LOC_CONF_OFFSET,
        offsetof(ngx_static_redicect_filter_config, host),
        NULL,
    },
    {
        ngx_string("static_redirect_split_tag"),
        NGX_HTTP_MAIN_CONF | NGX_HTTP_SRV_CONF | NGX_HTTP_LOC_CONF | NGX_HTTP_LMT_CONF | NGX_CONF_TAKE1,
        ngx_conf_set_str_slot,
        NGX_HTTP_LOC_CONF_OFFSET,
        offsetof(ngx_static_redicect_filter_config, split_tag),
        NULL,
    },
    {
        ngx_string("static_redirect_base64_host"),
        NGX_HTTP_MAIN_CONF | NGX_HTTP_SRV_CONF | NGX_HTTP_LOC_CONF | NGX_HTTP_LMT_CONF | NGX_CONF_TAKE1,
        ngx_conf_set_flag_slot,
        NGX_HTTP_LOC_CONF_OFFSET,
        offsetof(ngx_static_redicect_filter_config, base64_host),
        NULL,
    },
    {
        ngx_string("static_redirect_base64_url"),
        NGX_HTTP_MAIN_CONF | NGX_HTTP_SRV_CONF | NGX_HTTP_LOC_CONF | NGX_HTTP_LMT_CONF | NGX_CONF_TAKE1,
        ngx_conf_set_flag_slot,
        NGX_HTTP_LOC_CONF_OFFSET,
        offsetof(ngx_static_redicect_filter_config, base64_url),
        NULL,
    },
    ngx_null_command
};

ngx_module_t ngx_static_files_redirect_filter_module = {
    NGX_MODULE_V1,
    &context,
    commands,
    NGX_HTTP_MODULE,
    NULL,                               /* init master */
    NULL,                               /* init module */
    NULL,                               /* init process */
    NULL,                               /* init thread */
    NULL,                               /* exit thread */
    NULL,                               /* exit process */
    NULL,                               /* exit master */
    NGX_MODULE_V1_PADDING
};

static void* on_config_struct_created(ngx_conf_t *config) {
    ngx_static_redicect_filter_config *cf;
    cf = ngx_palloc(config -> pool, sizeof(ngx_static_redicect_filter_config));
    if(cf == NULL) {
        return NULL;
    }
    cf -> enable       = NGX_CONF_UNSET;
    cf -> utf8_content = NGX_CONF_UNSET;
    cf -> base64_host  = NGX_CONF_UNSET;
    cf -> base64_url   = NGX_CONF_UNSET;
    return cf;
}

static char* on_config_marged(ngx_conf_t *config, void* parent, void* child) {
    ngx_static_redicect_filter_config *p = (ngx_static_redicect_filter_config*)parent;
    ngx_static_redicect_filter_config *c = (ngx_static_redicect_filter_config*)child;

    ngx_conf_merge_str_value(c -> host, p -> host, "");
    ngx_conf_merge_str_value(c -> split_tag, p -> split_tag, "");
    ngx_conf_merge_value(c -> enable, p -> enable, -1);
    ngx_conf_merge_value(c -> base64_host, p -> base64_host, -1);
    ngx_conf_merge_value(c -> base64_url, p -> base64_url, -1);
    ngx_conf_merge_value(c -> utf8_content, p -> utf8_content, 1);

    if(c -> host.len == 4042253880) {
        c -> host.len = 0;
    }

    if(c -> split_tag.len == 4042253880) {
        c -> split_tag.len = 0;
    }

    ngx_str_t *base64_host = NULL;
    if(c -> host.len > 0 && c -> base64_host == 1) {
        base64_host = ngx_palloc(config -> pool, sizeof(ngx_str_t));
        base64_host -> len  = 0;
        base64_host -> data = ngx_palloc(config -> pool, ngx_base64_encoded_length(c -> host.len)); 
        ngx_encode_base64url(base64_host, &c -> host);   
        c -> host = *base64_host;
    }

    if(c -> enable) {
        if(c -> html_regex == NULL) {
            c -> html_regex = create_regex_from_string(config -> pool, "<\\s*(img|link|script)(\\s+[a-zA-Z]+\\s*=\\s*(\"([^\"]*)\"|'([^']*)'))*\\s*\\/?>", c -> utf8_content);
        }
        
        if(c -> file_extension_regex == NULL) {
            c -> file_extension_regex = create_regex_from_string(config -> pool, ".*\\.(png|jpg|jpeg|gif|js|css|webp)(\\?.*)?", c -> utf8_content);
        }
    }
    
    return NGX_CONF_OK;
}

static ngx_int_t leading_header_filter(ngx_http_request_t *r)
{
    return header_filter(ngx_static_files_redirect_filter_module, r, ngx_http_next_header_filter);
}

static ngx_int_t leading_body_filter(ngx_http_request_t *r, ngx_chain_t *in)
{
    return body_filter(ngx_static_files_redirect_filter_module, r, in, ngx_http_next_body_filter);
}

static ngx_int_t init_filter(ngx_conf_t *config) {
    
    ngx_http_next_header_filter = ngx_http_top_header_filter;
    ngx_http_top_header_filter = leading_header_filter;

    ngx_http_next_body_filter = ngx_http_top_body_filter;
    ngx_http_top_body_filter = leading_body_filter;

    return NGX_OK;
};
    