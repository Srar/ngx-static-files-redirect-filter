#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>

#include <pcre.h>

#ifndef _NGX_STATIC_REDIRECT_TOOLS_INCLUDE_
#define _NGX_STATIC_REDIRECT_TOOLS_INCLUDE_

typedef struct {
    ngx_int_t start;
    ngx_int_t end;
    ngx_int_t len;

    ngx_str_t *str;
} ngx_static_redicect_regex_search_result;

typedef struct {
    ngx_int_t len;
    ngx_static_redicect_regex_search_result *array;
} ngx_static_redicect_regex_search_result_t;

ngx_chain_t* get_test_response(ngx_pool_t* pool);
ngx_chain_t* get_response_from_ngx_str(ngx_pool_t* pool, ngx_str_t *str);

ngx_int_t get_size_of_chain(ngx_chain_t *chain);
ngx_str_t* link_chains(ngx_pool_t* pool, ngx_chain_t *chain);
ngx_int_t is_last_chain(ngx_chain_t *chain);

ngx_regex_compile_t* generator_regex(ngx_pool_t* pool, ngx_str_t* regex, ngx_int_t utf8);
ngx_regex_compile_t* create_regex_from_string(ngx_pool_t* pool, char* regex, ngx_int_t utf8);

ngx_static_redicect_regex_search_result_t* regex_search(ngx_pool_t* pool, ngx_regex_t* re, ngx_str_t* str);

char* substring(ngx_pool_t* pool, char* src, int start, int end);
ngx_str_t* ngx_substring(ngx_pool_t* pool, ngx_str_t* src, ngx_int_t start, ngx_int_t end);

ngx_str_t* get_ngx_string_malloc(char* string);
ngx_str_t* create_ngx_string(ngx_pool_t *pool, char* string);
ngx_str_t* ngx_append(ngx_pool_t *pool, ngx_str_t* a, ngx_str_t* b);
ngx_str_t* ngx_copy_str(ngx_pool_t *pool, ngx_str_t* src);
ngx_buf_t *copy_str_to_buf(ngx_pool_t *pool, ngx_str_t *str, ngx_int_t start, ngx_int_t end, ngx_flag_t last_buf);

void add_buf_to_last_of_chain(ngx_pool_t *pool, ngx_chain_t *chain, ngx_buf_t *buf);
void add_str_to_last_of_chain(ngx_pool_t *pool, ngx_chain_t *chain, char* str, ngx_int_t start, ngx_int_t end, ngx_flag_t last);
    
#endif