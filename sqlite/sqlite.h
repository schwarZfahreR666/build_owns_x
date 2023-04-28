#ifndef COMP

#define COMP

#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>

/*********************************结构定义*********************************/

/*硬编码定义一个行的数据结构，sqlite是行数据库，存取数据以行为单位*/
#define COLUMN_USERNAME_SIZE 32
#define COLUMN_EMAIL_SIZE 255

typedef struct {
  uint32_t id;
  char username[COLUMN_USERNAME_SIZE];
  char email[COLUMN_EMAIL_SIZE];
} Row;
/**************************************/
/*定义row的紧凑表示*/
#define size_of_attribute(Struct, Attribute) sizeof(((Struct*)0)->Attribute)

const uint32_t ID_SIZE = size_of_attribute(Row, id);
const uint32_t USERNAME_SIZE = size_of_attribute(Row, username);
const uint32_t EMAIL_SIZE = size_of_attribute(Row, email);
const uint32_t ID_OFFSET = 0;

const uint32_t USERNAME_OFFSET = ID_OFFSET + ID_SIZE;
const uint32_t EMAIL_OFFSET = USERNAME_OFFSET + USERNAME_SIZE;
const uint32_t ROW_SIZE = ID_SIZE + USERNAME_SIZE + EMAIL_SIZE;
/*************************************************/
/*Pager结构用于访问cache和文件，对应一个文件（一个表），也即若干页*/
#define TABLE_MAX_PAGES 100
typedef struct {
  int file_descriptor;
  uint32_t file_length;
  void* pages[TABLE_MAX_PAGES];
} Pager;
/**************************************/
/*定义page结构*/
const uint32_t PAGE_SIZE = 4096;  //4k一页
const uint32_t ROWS_PER_PAGE = PAGE_SIZE / ROW_SIZE;
const uint32_t TABLE_MAX_ROWS = ROWS_PER_PAGE * TABLE_MAX_PAGES;

typedef struct {
  uint32_t num_rows;
  Pager* pager;
} Table;
//因为页在内存中离散存储，为了方便换入换出页，row不会跨页存储
/*************************************************/
/*定义游标结构*/
typedef struct {
  Table* table;
  uint32_t row_num; //使用行号标识一个表中的一行
  bool end_of_table;  // Indicates a position one past the last element
} Cursor;
/*****************************************************/

/*定义一个字节buffer结构，但其中不包含实际的缓冲区，只包含指针*/
typedef struct {
  char* buffer;
  size_t buffer_length;
  ssize_t input_length;
} InputBuffer;
/*****************************************************/
/*命令相关*/
//meta语句执行结果枚举
typedef enum {
  META_COMMAND_SUCCESS,
  META_COMMAND_UNRECOGNIZED_COMMAND
} MetaCommandResult;
//解析语句结果
typedef enum { PREPARE_SUCCESS, PREPARE_NEGATIVE_ID, PREPARE_STRING_TOO_LONG, PREPARE_SYNTAX_ERROR, PREPARE_UNRECOGNIZED_STATEMENT} PrepareResult;
//语句类型枚举
typedef enum { STATEMENT_INSERT, STATEMENT_SELECT } StatementType;
//执行结果
typedef enum { EXECUTE_SUCCESS, EXECUTE_TABLE_FULL } ExecuteResult;
//语句结构
typedef struct {
  StatementType type;
  Row row_to_insert;  // only used by insert statement
} Statement;
/************************************************************/

/*************************************************************************/

/*******************************函数定义***********************************/
//命令行交互
InputBuffer* new_input_buffer();
void print_prompt();
void read_input(InputBuffer* input_buffer);
void close_input_buffer(InputBuffer* input_buffer);
//命令处理
MetaCommandResult do_meta_command(InputBuffer* input_buffer,Table *table);
PrepareResult prepare_insert(InputBuffer* input_buffer, Statement* statement);
PrepareResult prepare_statement(InputBuffer* input_buffer,Statement* statement);

ExecuteResult execute_select(Statement* statement, Table* table);
ExecuteResult execute_insert(Statement* statement, Table* table);
ExecuteResult execute_statement(Statement* statement, Table* table);
//row管理
void serialize_row(Row* source, void* destination);
void deserialize_row(void* source, Row* destination);
void* row_slot(Table* table, uint32_t row_num);
void print_row(Row* row);
//db管理
Table* db_open(const char* filename);
void db_close(Table* table);
Pager* pager_open(const char* filename);
void* get_page(Pager* pager, uint32_t page_num);
void pager_flush(Pager* pager, uint32_t page_num, uint32_t size);
//cursor管理
Cursor* table_start(Table* table);
Cursor* table_end(Table* table);
void* cursor_value(Cursor* cursor);
void cursor_advance(Cursor* cursor);
/*************************************************************************/

#endif