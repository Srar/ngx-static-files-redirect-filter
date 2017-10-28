#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>

#include <pcre.h>

#include <time.h>

#include "ngx_static_files_redirect_tools.h"

ngx_chain_t* get_test_response(ngx_pool_t* pool) {
    ngx_str_t str = ngx_string("<h1>my first nginx c++ module.</h1>\n");
    return get_response_from_ngx_str(pool, &str);
}

ngx_chain_t* get_response_from_ngx_str(ngx_pool_t* pool, ngx_str_t *str) {
    ngx_buf_t *buffer = ngx_create_temp_buf(pool, str -> len);
    if(buffer == NULL) {
        return NULL;
    }
    ngx_memcpy(buffer -> pos, str ->data, str -> len);
    buffer -> memory = 1;
    buffer -> last = buffer -> pos + str -> len;
    buffer -> last_buf = 1;

    ngx_chain_t *out = (ngx_chain_t *)ngx_palloc(pool, sizeof(ngx_chain_t));
    out -> buf = buffer;
    out -> next = NULL; 
    
    return out;
}

ngx_int_t get_size_of_chain(ngx_chain_t *chain) {
    ngx_int_t   size    = 0;
    ngx_chain_t *nchain = chain;
    do {
        size += ngx_buf_size(nchain -> buf);
    } while((nchain = nchain -> next) != NULL);
    return size;
}

ngx_int_t is_last_chain(ngx_chain_t *chain) {
    ngx_int_t last = 0;
    ngx_chain_t *nchain = chain;
    do {
        if(nchain -> buf -> last_buf) {
            last = 1;
            break;
        }
    } while((nchain = nchain -> next) != NULL);
    return last;
}

ngx_str_t* link_chains(ngx_pool_t* pool, ngx_chain_t *chain) {
    ngx_uint_t offset = 0;
    ngx_str_t *str   = ngx_palloc(pool, sizeof(ngx_str_t));
    str -> len       = get_size_of_chain(chain);
    str -> data      = ngx_palloc(pool, str -> len + 1);  
    do {
        int buffer_size = ngx_buf_size(chain -> buf);
        for(int i = 0; i < buffer_size; i++) {
            str -> data[offset] = chain -> buf -> pos[i];
            offset++;
        }
    } while((chain = chain -> next) != NULL);
    str -> data[str -> len] = '\0';
    return str;
}

ngx_regex_compile_t* create_regex_from_string(ngx_pool_t* pool, char* regex, ngx_int_t utf8) {
    ngx_str_t ngx_string_regex = ngx_string(regex);
    return generator_regex(pool, &ngx_string_regex, utf8);
}

ngx_regex_compile_t* generator_regex(ngx_pool_t* pool, ngx_str_t* pattern, ngx_int_t utf8) {
    ngx_regex_compile_t *rc = ngx_palloc(pool, sizeof(ngx_regex_compile_t));
    rc -> pattern = *pattern;
    rc -> pool = pool;
    if(utf8) {
        rc -> options = PCRE_UTF8 | PCRE_MULTILINE;
    } else {
        rc -> options = PCRE_MULTILINE;
    }
    ngx_int_t compile_result = ngx_regex_compile(rc);
    if(compile_result != NGX_OK) {
        return NULL;
    }
    return rc;
}

ngx_static_redicect_regex_search_result_t* regex_search(ngx_pool_t* pool, ngx_regex_t* re, ngx_str_t* str) {
    ngx_int_t elements_count = 0;
    ngx_static_redicect_regex_search_result* elements;
    ngx_static_redicect_regex_search_result_t *response = ngx_palloc(pool, sizeof(ngx_static_redicect_regex_search_result_t));
    ngx_list_t* elements_list = ngx_list_create(pool, 12, sizeof(ngx_static_redicect_regex_search_result));
    if(elements_list == NULL) return NULL;

    int ovector[30]; 
    int result = 0;
    int regex_exec_offset = 0;
    while((result = pcre_exec(
        re -> code, re -> extra, 
        (char*)str -> data, str -> len, 
        regex_exec_offset, 
        0, 
        ovector, 30)) > 0)
    {
        ngx_static_redicect_regex_search_result* element = ngx_list_push(elements_list);
        element -> start  = ovector[0];
        element -> end    = ovector[1];
        element -> len    = ovector[1] - ovector[0];
        element -> str    = create_ngx_string(pool, substring(pool, (char*)str -> data, (int)element -> start, (int)element -> end - 1)); 
        elements_count++;
        regex_exec_offset = ovector[1];
    }

    elements = ngx_palloc(pool, sizeof(ngx_static_redicect_regex_search_result) * elements_count);
    if(elements_count == 0) {
        response -> len = 0;
        response -> array  = elements;
        return response;
    }
    
    int to_array_counter = 0;
    ngx_list_part_t *elements_part = &elements_list -> part;
    do {
        ngx_static_redicect_regex_search_result* elts = elements_part -> elts;
        for(int i = 0; i < (int)elements_part -> nelts; i++) {
            elements[to_array_counter] = elts[i];
            to_array_counter++;
        } 
    }while((elements_part = elements_part -> next) != NULL);
   
    response -> len = to_array_counter;
    response -> array  = elements;

    return response;
}

ngx_str_t* create_ngx_string(ngx_pool_t *pool, char* string) {
    ngx_int_t length = strlen(string);
    // printf("string length: %d \n", (int)length);
    ngx_str_t *str = ngx_palloc(pool, sizeof(ngx_str_t));
    str -> len  = length;
    str -> data = ngx_palloc(pool, length + 1);
    ngx_memcpy(str -> data, string, length + 1);
    return str;
}

ngx_str_t* get_ngx_string_malloc(char* string) {
    ngx_str_t *str = malloc(sizeof(ngx_str_t));
    ngx_int_t length = sizeof(string);
    str -> len    = length - 1;
    str -> data   = malloc(length);
    ngx_memcpy(str -> data, string, length);
    return str;
}

char* substring(ngx_pool_t* pool, char* src, int start, int end) {
    int new_length = end - start;
    char* new_str = ngx_palloc(pool, new_length + 1);
    ngx_memcpy(new_str, src + start, new_length);
    new_str[new_length] = '\0';
    return new_str;
}

ngx_str_t* ngx_substring(ngx_pool_t* pool, ngx_str_t* str, ngx_int_t start, ngx_int_t end) {
    // printf("start: %d, end: %d, string length: %d\n", (int)start, (int)end, (int)str -> len);

    if(start < 0) return NULL;
    if(end > (ngx_int_t)str -> len) return NULL;
    if(start > end) return NULL;
    ngx_int_t new_length = end - start;
    if(new_length < 0) return NULL;

    ngx_str_t *new_str = ngx_palloc(pool, sizeof(ngx_str_t));
    new_str -> len  = new_length;
    new_str -> data = ngx_palloc(pool, new_str -> len + 1);
    ngx_memcpy(new_str -> data, str -> data + start, new_str -> len);
    new_str -> data[new_str -> len] = '\0';
    return new_str;
}

ngx_str_t* ngx_append(ngx_pool_t *pool, ngx_str_t* a, ngx_str_t* b) {
    if(b -> len <= 0) return a;
    ngx_str_t *str = ngx_palloc(pool, sizeof(ngx_str_t));
    str -> len  = a -> len + b -> len;
    str -> data = ngx_palloc(pool,  str -> len + 1);
    ngx_memcpy(str -> data, a -> data, a -> len);
    ngx_memcpy(str -> data + a -> len, b -> data, b -> len);
    str -> data[str -> len] = '\0';
    return str;
}

ngx_str_t* ngx_copy_str(ngx_pool_t *pool, ngx_str_t* src) {
    ngx_str_t *str = ngx_palloc(pool, sizeof(ngx_str_t));
    str -> len  = src -> len;
    str -> data = ngx_palloc(pool,  str -> len + 1);
    ngx_memcpy(str -> data, src -> data, src -> len);
    str -> data[str -> len] = '\0';
    return str;
}

void add_str_to_last_of_chain(ngx_pool_t *pool, ngx_chain_t *chain, char* str, ngx_int_t start, ngx_int_t end, ngx_flag_t last) {
    ngx_buf_t *buffer = ngx_create_temp_buf(pool, end - start);
    buffer -> memory        = true;
    buffer -> pos           = (u_char*)str + start;
    buffer -> last          = (u_char*)str + start + end;
    buffer -> last_buf      = last;
    buffer -> last_in_chain = last;
    add_buf_to_last_of_chain(pool, chain, buffer);
}

void add_buf_to_last_of_chain(ngx_pool_t *pool, ngx_chain_t *chain, ngx_buf_t *buf) {
    ngx_chain_t *c = chain;
    if(c -> buf == NULL) {
        c -> buf = buf;
        return;
    }

    while(true) {
        if(c -> next == NULL) break;
        c = c -> next;
    }

    c -> next = ngx_alloc_chain_link(pool);
    c = c -> next;
    c -> buf  = buf;
    c -> next = NULL;
}