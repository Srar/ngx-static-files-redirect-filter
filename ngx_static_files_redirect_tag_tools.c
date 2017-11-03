#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>

#include <pcre.h>

#include "ngx_static_files_redirect_tools.h"
#include "ngx_static_files_redirect_tag_tools.h"

ngx_int_t char_indexOf(ngx_str_t* str, char c, ngx_int_t offset) {
    if (offset < 0) return -1;
    ngx_int_t i;
    for(i = offset; i < (ngx_int_t)str -> len; i++) {
        if (str -> data[i] == c) return i;
    }
    return -1;
}

ngx_int_t char_last_indexOf(ngx_str_t* str, char c) {
    ngx_int_t i;
    for(i = str -> len - 1; i >= 0 ; i--) {
        if (str -> data[i] == c) return i;
    }
    return -1;
}

ngx_int_t str_indexOf(ngx_str_t* s, ngx_str_t* t) {
    u_char* address = (u_char*)ngx_strstr(s -> data, t -> data);
    if (address == NULL) return -1;
    return address - s -> data;
}

void calculate_str_len(ngx_str_t *str) {
    str -> len = ngx_strlen(str -> data);
}

ngx_str_t* redirect_static_file(ngx_pool_t *pool, ngx_flag_t https, ngx_str_t *host, ngx_str_t *path, ngx_str_t *tag_url) {
    ngx_flag_t is_front_http  = 0;
    ngx_flag_t is_front_https = 0;
    char* front_http = ngx_strstr(tag_url -> data, "http://");
    char* front_https = ngx_strstr(tag_url -> data, "https://");
    is_front_http = front_http != NULL && front_http == (char*)tag_url -> data;
    is_front_https = front_https != NULL && front_https == (char*)tag_url -> data;

    // printf("is http: %d, is https: %d\n", is_front_http, is_front_https);

    ngx_int_t offset            = 0;
    static ngx_str_t *http_str  = NULL;
    static ngx_str_t *https_str = NULL;
    if (http_str == NULL)  {
        http_str  = get_ngx_string_malloc("http://");
    }
    if (https_str == NULL) {
        https_str = get_ngx_string_malloc("https://");
    }

    ngx_str_t *ngx_str_new_url = ngx_palloc(pool, sizeof(ngx_str_t));
    ngx_str_new_url -> len  = (https_str -> len + host -> len + tag_url -> len) * 2;
    ngx_str_new_url -> data = ngx_palloc(pool, ngx_str_new_url -> len);

    char* new_url = (char*)ngx_str_new_url -> data;

    ngx_str_t *parsed_path = ngx_substring(pool, path, 0, char_last_indexOf(path, '/') + 1);

    // memcpy((char*)new_url + offset, rhost -> data, rhost -> len);
    // offset += rhost -> len;

    if (is_front_http == 0 && is_front_https == 0) {
        if (tag_url -> data[0] != '/') {
            memcpy((char*)new_url + offset, parsed_path -> data, parsed_path -> len);
            offset += parsed_path -> len;

            /* <img src="public/imgs/header.jpg"> */
            if (tag_url -> data[0] != '.' && tag_url -> data[1] != '/') {
                memcpy((char*)new_url + offset, tag_url -> data, tag_url -> len);
                offset += tag_url -> len;
                new_url[offset] = '\0';
                calculate_str_len(ngx_str_new_url);
                return ngx_str_new_url;
            } 

            /* <img src="./public/imgs/header.jpg"> */
            if (tag_url -> data[0] == '.' && tag_url -> data[1] == '/') {
                memcpy((char*)new_url + offset, tag_url -> data + 2, tag_url -> len - 2);
                offset += tag_url -> len - 2;
                new_url[offset] = '\0';
                calculate_str_len(ngx_str_new_url);
                return ngx_str_new_url;
            }
        }

        /* skip <script src="//cdn.staticfile.org/jquery/2.2.1/jquery.min.js"></script>  */
        if (tag_url -> data[0] == '/' && tag_url -> data[1] == '/') {
            return NULL;
        }

        /* <img src="/public/imgs/header.jpg"> */
        if (tag_url -> data[0] == '/') {
            memcpy((char*)new_url + offset, tag_url -> data, tag_url -> len);
            offset += tag_url -> len;
            new_url[offset] = '\0';
            calculate_str_len(ngx_str_new_url);
            return ngx_str_new_url;
        }
    }

    if (is_front_http || is_front_https) {
        ngx_int_t i;
        ngx_int_t limit_flags[3] = {0, 0, 0};
        for(i = 0; i < (ngx_int_t)tag_url -> len; i++) {
            if (tag_url -> data[i] != '/') continue;
            if (limit_flags[0] == 0) {
                limit_flags[0] = i;
                continue;
            }
            if (limit_flags[1] == 0) {
                limit_flags[1] = i;
                continue;
            }
            if (limit_flags[2] == 0) {
                limit_flags[2] = i;
                continue;
            }
        }
        // printf("{%d, %d, %d}\n", limit_flags[0], limit_flags[1], limit_flags[2]);

        ngx_str_t *tag_url_domain = ngx_substring(pool, tag_url, limit_flags[1] + 1, limit_flags[2] - 1);
        if (!(ngx_strcmp(host -> data, tag_url_domain -> data) == 0)) return NULL;
        
        ngx_int_t tag_url_length  = tag_url -> len - limit_flags[2];        
        memcpy((char*)new_url + offset, tag_url -> data + limit_flags[2], tag_url_length);
        offset += tag_url_length;
        new_url[offset] = '\0';
        calculate_str_len(ngx_str_new_url);
        return ngx_str_new_url;
    }

    return NULL;
}


ngx_static_redicect_regex_search_result* search_html_tag_property(ngx_pool_t* pool, ngx_str_t* str, ngx_str_t* property) {
    ngx_static_redicect_regex_search_result* result = ngx_palloc(pool, sizeof(ngx_static_redicect_regex_search_result));
    ngx_int_t property_offset = str_indexOf(str, property);
    if (property_offset == -1) return NULL;
    // printf("str: %s property_offset: %d\n", str -> data, property_offset);
    
    ngx_int_t open_flag_offset = char_indexOf(str, '"', property_offset);
    if (open_flag_offset == -1) return NULL;
    
    ngx_int_t close_flag_offset = char_indexOf(str, '"', open_flag_offset + 1);
    if (close_flag_offset == -1) return NULL;

    // printf("%s\n", str -> data);
    // printf("property offset: %d open flag offset: %d close flag offset: %d\n", property_offset, open_flag_offset, close_flag_offset);

    result -> start  = open_flag_offset + 1;
    result -> end    = close_flag_offset - 1;
    result -> len    = result -> end - result -> start;
    result -> str    = ngx_substring(pool, str, result -> start, result -> end + 1);
    return result;  
}

