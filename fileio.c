
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "util.h"

struct _file_ptr_ {
    FILE* fp;
    Str* fname;
    int line_no;
    int col_no;
    int ch;
    struct _file_ptr_* next;
};

static struct _file_ptr_* file_stack = NULL;

/**
 * @brief Open an input file and push it on the stack.
 *
 * @param fname
 */
void open_input_file(const char* fname) {

    struct _file_ptr_* ptr = _ALLOC_T(struct _file_ptr_);
    ptr->fp = fopen(fname, "r");
    if(ptr->fp == NULL)
        RAISE(FILE_ERROR, "File Error: cannot open input file: %s: %s\n", fname,
              strerror(errno));

    ptr->fname = create_string(fname);
    ptr->line_no = 1;
    ptr->col_no = 1;
    ptr->next = NULL;
    ptr->ch = fgetc(ptr->fp);

    if(file_stack != NULL)
        ptr->next = file_stack;
    file_stack = ptr;
}

/**
 * @brief Close an input file and pop it off of the stack.
 *
 */
static void close_input_file() {

    if(file_stack != NULL) {
        fclose(file_stack->fp);
        struct _file_ptr_* tmp = file_stack;
        file_stack = tmp->next;
        destroy_string(tmp->fname);
        _FREE(tmp);
    }
}

/**
 * @brief Return the current char or EOF if there is no more input.
 *
 * @return int
 */
int get_char() {

    if(file_stack != NULL)
        return file_stack->ch;
    else
        return EOF;
}

/**
 * @brief Consume a character from the input stream. If the file ends, then
 * try to close it.
 *
 * @return int
 */
int consume_char() {

    if(file_stack != NULL) {
        if(file_stack->ch == '\n') {
            file_stack->line_no++;
            file_stack->col_no = 1;
        }
        else
            file_stack->col_no++;

        file_stack->ch = fgetc(file_stack->fp);
        if(file_stack->ch == EOF) {
            close_input_file();
            // return the current character from the previous file.
            return get_char();
        }

        return file_stack->ch;
    }
    else
        return EOF;
}

/**
 * @brief Return the line number of the current file. Returns -1 if there
 * is not currently open file.
 *
 * @return int
 */
int get_line_no() {

    if(file_stack != NULL)
        return file_stack->line_no;
    else
        return -1;
}

/**
 * @brief Return the column number of the current file or -1 if there is
 * no file currently open.
 *
 * @return int
 */
int get_col_no() {

    if(file_stack != NULL)
        return file_stack->col_no;
    else
        return -1;
}

/**
 * @brief Return the current file name as it was opened.
 *
 * @return STR
 */
const char* get_fname() {

    if(file_stack != NULL)
        return raw_string(file_stack->fname);
    else
        return NULL;
}

FPTR open_output_file(const char* fname) {

    struct _file_ptr_* ptr = _ALLOC_T(struct _file_ptr_);
    ptr->fp = fopen(fname, "w");
    if(ptr->fp == NULL)
        RAISE(FILE_ERROR, "File Error: cannot open output file: %s: %s\n",
              fname, strerror(errno));

    ptr->fname = create_string(fname);
    ptr->line_no = 1;
    ptr->col_no = 1;
    ptr->next = NULL;
    ptr->ch = 0;

    return (FPTR)ptr;
}

void close_output_file(FPTR fp) {

    struct _file_ptr_* ptr = (struct _file_ptr_*)fp;
    fclose(ptr->fp);
    destroy_string(ptr->fname);
    _FREE(fp);
}

void emit_buf(FPTR h, void* buf, unsigned int size) {

    fwrite(buf, 1, size, ((struct _file_ptr_*)h)->fp);
}

void emit_fmt(FPTR h, const char* fmt, ...) {

    struct _file_ptr_* ptr = (struct _file_ptr_*)h;
    va_list args;

    va_start(args, fmt);
    vfprintf(ptr->fp, fmt, args);
    va_end(args);
}

void emit_Str(FPTR h, Str* str) {

    struct _file_ptr_* ptr = (struct _file_ptr_*)h;
    fprintf(ptr->fp, "%s", raw_string(str));
}

void emit_str(FPTR h, const char* str) {

    struct _file_ptr_* ptr = (struct _file_ptr_*)h;
    fprintf(ptr->fp, "%s", str);
}
