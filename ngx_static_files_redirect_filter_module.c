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
char *ngx_conf_set_host_ramdom(ngx_conf_t *cf, ngx_command_t *cmd, void *conf);

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
        ngx_string("static_redirect_new_host"),
        NGX_HTTP_MAIN_CONF | NGX_HTTP_SRV_CONF | NGX_HTTP_LOC_CONF | NGX_HTTP_LMT_CONF | NGX_CONF_TAKE1,
        ngx_conf_set_str_slot,
        NGX_HTTP_LOC_CONF_OFFSET,
        offsetof(ngx_static_redicect_filter_config, new_host),
        NULL,
    },
    {
        ngx_string("static_redirect_new_host_ramdom"),
        NGX_HTTP_MAIN_CONF | NGX_HTTP_SRV_CONF | NGX_HTTP_LOC_CONF | NGX_HTTP_LMT_CONF | NGX_CONF_TAKE2,
        ngx_conf_set_host_ramdom,
        NGX_HTTP_LOC_CONF_OFFSET,
        0,
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
        ngx_string("static_redirect_take_src_host"),
        NGX_HTTP_MAIN_CONF | NGX_HTTP_SRV_CONF | NGX_HTTP_LOC_CONF | NGX_HTTP_LMT_CONF | NGX_CONF_TAKE1,
        ngx_conf_set_flag_slot,
        NGX_HTTP_LOC_CONF_OFFSET,
        offsetof(ngx_static_redicect_filter_config, take_src_host),
        NULL,
    },
    {
        ngx_string("static_redirect_take_src_requesting_path"),
        NGX_HTTP_MAIN_CONF | NGX_HTTP_SRV_CONF | NGX_HTTP_LOC_CONF | NGX_HTTP_LMT_CONF | NGX_CONF_TAKE1,
        ngx_conf_set_flag_slot,
        NGX_HTTP_LOC_CONF_OFFSET,
        offsetof(ngx_static_redicect_filter_config, take_requesting_path),
        NULL,
    },
    {
        ngx_string("static_redirect_base64_src_host"),
        NGX_HTTP_MAIN_CONF | NGX_HTTP_SRV_CONF | NGX_HTTP_LOC_CONF | NGX_HTTP_LMT_CONF | NGX_CONF_TAKE1,
        ngx_conf_set_flag_slot,
        NGX_HTTP_LOC_CONF_OFFSET,
        offsetof(ngx_static_redicect_filter_config, base64_src_host),
        NULL,
    },
    {
        ngx_string("static_redirect_base64_src_requesting_path"),
        NGX_HTTP_MAIN_CONF | NGX_HTTP_SRV_CONF | NGX_HTTP_LOC_CONF | NGX_HTTP_LMT_CONF | NGX_CONF_TAKE1,
        ngx_conf_set_flag_slot,
        NGX_HTTP_LOC_CONF_OFFSET,
        offsetof(ngx_static_redicect_filter_config, base64_requesting_path),
        NULL,
    },
    {
        ngx_string("static_redirect_base64_src_url"),
        NGX_HTTP_MAIN_CONF | NGX_HTTP_SRV_CONF | NGX_HTTP_LOC_CONF | NGX_HTTP_LMT_CONF | NGX_CONF_TAKE1,
        ngx_conf_set_flag_slot,
        NGX_HTTP_LOC_CONF_OFFSET,
        offsetof(ngx_static_redicect_filter_config, base64_src_url),
        NULL,
    },
    {
        ngx_string("static_redirect_buffer_size_limit"),
        NGX_HTTP_MAIN_CONF | NGX_HTTP_SRV_CONF | NGX_HTTP_LOC_CONF | NGX_HTTP_LMT_CONF | NGX_CONF_TAKE1,
        ngx_conf_set_size_slot,
        NGX_HTTP_LOC_CONF_OFFSET,
        offsetof(ngx_static_redicect_filter_config, buffer_size_limit),
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
    cf -> enable                 = NGX_CONF_UNSET;
    cf -> utf8_content           = NGX_CONF_UNSET;
    cf -> take_src_host          = NGX_CONF_UNSET;
    cf -> take_requesting_path   = NGX_CONF_UNSET;
    cf -> base64_src_host        = NGX_CONF_UNSET;
    cf -> base64_src_url         = NGX_CONF_UNSET;
    cf -> base64_requesting_path = NGX_CONF_UNSET;
    cf -> ramdom_domain_minmum   = NGX_CONF_UNSET;
    cf -> ramdom_domain_maxmum   = NGX_CONF_UNSET;
    cf -> buffer_size_limit      = NGX_CONF_UNSET;
    return cf;
}

static char* on_config_marged(ngx_conf_t *config, void* parent, void* child) {
    ngx_static_redicect_filter_config *p = (ngx_static_redicect_filter_config*)parent;
    ngx_static_redicect_filter_config *c = (ngx_static_redicect_filter_config*)child;

    ngx_conf_merge_str_value(c -> new_host, p -> new_host, "");
    ngx_conf_merge_str_value(c -> split_tag, p -> split_tag, "");
    ngx_conf_merge_value(c -> enable, p -> enable, 0);
    ngx_conf_merge_value(c -> take_src_host, p -> take_src_host, 1);
    ngx_conf_merge_value(c -> take_requesting_path, p -> take_requesting_path, 0);
    ngx_conf_merge_value(c -> base64_src_host, p -> base64_src_host, 0);
    ngx_conf_merge_value(c -> base64_src_url, p -> base64_src_url, 0);
    ngx_conf_merge_value(c -> base64_requesting_path, p -> base64_requesting_path, 0);
    ngx_conf_merge_value(c -> ramdom_domain_minmum, p -> ramdom_domain_minmum, 0);
    ngx_conf_merge_value(c -> ramdom_domain_maxmum, p -> ramdom_domain_maxmum, 3);
    ngx_conf_merge_value(c -> buffer_size_limit, p -> buffer_size_limit, 0);
    ngx_conf_merge_value(c -> utf8_content, p -> utf8_content, 1);

    if(c -> new_host.len == 4042253880) {
        c -> new_host.len = 0;
    }

    if(c -> split_tag.len == 4042253880) {
        c -> split_tag.len = 0;
    }

    if(c -> enable) {
        if(c -> html_regex == NULL) {
            c -> html_regex = create_regex_from_string(config -> pool, "<\\s*(img|link|script)(\\s+[a-zA-Z\\-]+\\s*=\\s*(\"([^\"]*)\"|'([^']*)'))*\\s*\\/?>", c -> utf8_content);
        }
        
        if(c -> file_extension_regex == NULL) {
            c -> file_extension_regex = create_regex_from_string(config -> pool, ".*\\.(png|jpg|jpeg|gif|js|css|webp)(\\?.*)?", c -> utf8_content);
        }
    }
    
    return NGX_CONF_OK;
}

char *ngx_conf_set_host_ramdom(ngx_conf_t *cf, ngx_command_t *cmd, void *conf) {
    ngx_static_redicect_filter_config *p = conf;
    ngx_str_t *values = (ngx_str_t *)cf -> args -> elts;

    if(cf -> args -> nelts != 3) {
        return "错误的参数个数";
    }

    p -> ramdom_domain_minmum = ngx_atoi(values[1].data, values[1].len);
    p -> ramdom_domain_maxmum = ngx_atoi(values[2].data, values[2].len);

    if(p -> ramdom_domain_minmum == NGX_ERROR || p -> ramdom_domain_maxmum == NGX_ERROR) {
        return "错误的参数类型";
    }

    if(!(p -> ramdom_domain_minmum >= 0 && p -> ramdom_domain_minmum <= 9)) {
        return "参数的值必须在0~9之间";
    }

    if(!(p -> ramdom_domain_maxmum >= 0 && p -> ramdom_domain_maxmum <= 9)) {
        return "参数的值必须在0~9之间";
    }

    if(p -> ramdom_domain_minmum > p -> ramdom_domain_maxmum) {
        return "错误的参数类型";
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
    
