#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>

#include <pcre.h>

#ifndef _NGX_STATIC_REDIRECT_TAG_TOOLS_INCLUDE_
#define _NGX_STATIC_REDIRECT_TAG_TOOLS_INCLUDE_

#include "ngx_static_files_redirect_tools.h"

ngx_str_t* redirect_static_file(ngx_pool_t *pool, ngx_flag_t https, ngx_str_t *host, ngx_str_t *path, ngx_str_t *tag_url);
ngx_static_redicect_regex_search_result* search_html_tag_property(ngx_pool_t* pool, ngx_str_t* str, ngx_str_t* property);

#endif