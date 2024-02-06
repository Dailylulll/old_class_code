#include <string.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

#include "../include/sstring.h"

#define INT_MAX 32767

bool string_valid(const char *str, const size_t length)
{
    if(str == NULL || length < 0) return false;
    else {
        for(size_t i = 0; i <= length; ++i) {
            if(*(str + i) == '\0' ) return true;
        }
        return false;
    }
}

char *string_duplicate(const char *str, const size_t length)  // almost forgot null terminator
{
    if(str == NULL || length < 0) return NULL;
    else {
        if(string_valid(str, length) == false) return NULL;
        char * new_str = malloc(length + 1);
        if(new_str == NULL) return NULL;
        memcpy(new_str, str, length + 1);
        new_str[length + 1] = '\0';
        if(strcmp(new_str, str) == 0) return new_str;
        else return NULL;
    }
}

bool string_equal(const char *str_a, const char *str_b, const size_t length)
{
    if(str_a == NULL || str_b == NULL || length <= 0) return false;
    else {
        if(strcmp(str_a, str_b) == 0) return true;
        else return false;
    }
}

int string_length(const char *str, const size_t length)
{
    int count = 0;
    if(str == NULL || length <= 0) return -1;
    else {
        for(int i = 0; i < length; i++){
            if(*(str + i) == '\0') return count;
            else count++;
        }
    }
    return count;
}

int string_tokenize(const char *str, const char *delims, const size_t str_length, char **tokens, const size_t max_token_length, const size_t requested_tokens)
{
    int i = 0;
    if(str == NULL || delims == NULL || str_length <= 0 || tokens == NULL || max_token_length <= 0 || requested_tokens <= 0) {
        return 0;
    } else {
        if(string_valid(str, str_length) == false) return -1;
        for(int j = 0; j < requested_tokens; ++j) {
            if(tokens[j] == NULL) return -1;
        }
        char * new_str = string_duplicate(str, strlen(str));
        char * ptr = strtok(new_str, delims);
        do {
            if(ptr != NULL) {
                strcpy(tokens[i], ptr);
                i++;
            }
            ptr = strtok(NULL, delims);
        } while(i < requested_tokens && ptr != NULL);
    }
    return i;
}  // must add max_token_length check, and token length check

bool string_to_int(const char *str, int *converted_value)  // assume base 10 representation?
{
    if(str == NULL || converted_value == NULL) return false;
    else {
        char * new_string = string_duplicate(str, strlen(str));
        char * token = strtok(new_string, " ");
        if(strlen(token) > sizeof(int)) return false;
        *converted_value = atoi(token);
        return true;
    }
} 
