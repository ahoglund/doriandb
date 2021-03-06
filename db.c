#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


/* TYPES */

const uint32_t COLUMN_USERNAME_SIZE = 32;
const uint32_t COLUMN_EMAIL_SIZE = 255;

struct Row_t {
  uint32_t id;
  /* + 1 is for string termination char */
  char username[COLUMN_USERNAME_SIZE + 1];
  char email[COLUMN_EMAIL_SIZE + 1];
};

typedef struct Row_t Row;

struct InputBuffer_t {
  char* buffer;
  size_t buffer_length;
  ssize_t input_length;
};

typedef struct InputBuffer_t InputBuffer;

enum MetaCommandResult_t {
  META_COMMAND_SUCCESS,
  META_COMMAND_FAIL
};

typedef enum MetaCommandResult_t MetaCommandResult;

MetaCommandResult execute_meta_command(InputBuffer* input_buffer) {
  if(strcmp(input_buffer->buffer, ".exit") == 0) {
    printf("Bye!\n");
    exit(EXIT_SUCCESS);
  } else {
    return META_COMMAND_SUCCESS;
  }
};

enum PrepareResult_t {
  PREPARE_SUCCESS,
  PREPARE_FAIL,
  PREPARE_STR_TOO_LONG,
  PREPARE_NEGATIVE_ID,
  PREPARE_SYNTAX_FAIL,
  PREPARE_NOOP
};

typedef enum PrepareResult_t PrepareResult;

enum ExecuteResult_t { EXECUTE_SUCCESS, EXECUTE_TABLE_FULL };

typedef enum ExecuteResult_t ExecuteResult;

enum StatementType_t {
  SELECT,
  INSERT
};

typedef enum StatementType_t StatementType;

struct Statement_t {
  StatementType type;
  Row row_to_insert;
};

typedef struct Statement_t Statement;

InputBuffer* new_input_buffer() {
  InputBuffer* input_buffer = malloc(sizeof(InputBuffer));
  input_buffer->buffer = NULL;
  input_buffer->buffer_length = 0;
  input_buffer->input_length = 0;

  return input_buffer;
};

#define size_of_attribute(Struct, Attribute) sizeof(((Struct*)0)->Attribute)

const uint32_t ID_SIZE = size_of_attribute(Row, id);
const uint32_t USERNAME_SIZE = size_of_attribute(Row, username);
const uint32_t EMAIL_SIZE = size_of_attribute(Row, email);
const uint32_t ID_OFFSET = 0;
const uint32_t USERNAME_OFFSET = ID_OFFSET + ID_SIZE;
const uint32_t EMAIL_OFFSET = USERNAME_OFFSET + USERNAME_SIZE;
const uint32_t ROW_SIZE = ID_SIZE + USERNAME_SIZE + EMAIL_SIZE;

const uint32_t PAGE_SIZE = 4096;
const uint32_t TABLE_MAX_PAGES = 100;
const uint32_t ROWS_PER_PAGE = PAGE_SIZE / ROW_SIZE;
const uint32_t TABLE_MAX_ROWS = ROWS_PER_PAGE * TABLE_MAX_PAGES;

struct Table_t {
  void* pages[TABLE_MAX_PAGES];
  uint32_t num_rows;
};

typedef struct Table_t Table;

/* FUNCTIONS */

void print_prompt() {
  printf("db> ");
};

void read_input(InputBuffer* input_buffer) {
  ssize_t bytes_read =
    getline(&(input_buffer->buffer), &(input_buffer->buffer_length), stdin);

  if(bytes_read <= 0) {
    printf("Error reading input\n");
    exit(EXIT_FAILURE);
  };

  input_buffer->input_length = bytes_read - 1;
  input_buffer->buffer[bytes_read - 1] = 0;
};

PrepareResult prepare_insert(InputBuffer* input_buffer, Statement* statement) {
  statement->type = INSERT;

  char* keyword = strtok(input_buffer->buffer, " ");
  char* id_str  = strtok(NULL, " ");
  char* name    = strtok(NULL, " ");
  char* email   = strtok(NULL, " ");

  if(id_str == NULL || name == NULL || email == NULL) {
    return PREPARE_FAIL;
  };

  int id = atoi(id_str);
  if(id < 0) {
    return PREPARE_NEGATIVE_ID;
  };

  if(strlen(name) > COLUMN_USERNAME_SIZE) {
    return PREPARE_STR_TOO_LONG;
  };

  if(strlen(email) > COLUMN_EMAIL_SIZE) {
    return PREPARE_STR_TOO_LONG;
  };

  statement->row_to_insert.id = id;
  strcpy(statement->row_to_insert.username, name);
  strcpy(statement->row_to_insert.email, email);

  return PREPARE_SUCCESS;
};

PrepareResult prepare_statement(InputBuffer* input_buffer, Statement* statement) {
  if(strcmp(input_buffer->buffer, "") == 0) {
    return PREPARE_NOOP;
  };

  if(strncmp(input_buffer->buffer, "select", 6) == 0) {
    statement->type = SELECT;
    return PREPARE_SUCCESS;
  };

  if (strncmp(input_buffer->buffer, "insert", 6) == 0) {
    return prepare_insert(input_buffer, statement);
  };

  return PREPARE_FAIL;
};

void* row_slot(Table* table, uint32_t row_num) {
  uint32_t page_num = row_num / ROWS_PER_PAGE;
  void* page = table->pages[page_num];
  if(!page) {
    page = table->pages[page_num] = malloc(PAGE_SIZE);
  };

  uint32_t row_offset = row_num % ROWS_PER_PAGE;
  uint32_t byte_offset = row_offset % ROW_SIZE;
  return page + byte_offset;
};


void serialize_row(Row* source, void* destination) {
  memcpy(destination + ID_OFFSET, &(source->id), ID_SIZE);
  memcpy(destination + USERNAME_OFFSET, &(source->username), USERNAME_SIZE);
  memcpy(destination + EMAIL_OFFSET, &(source->email), EMAIL_SIZE);
};

void deserialize_row(void* source, Row* destination) {
  memcpy(&(destination->id), source + ID_OFFSET, ID_SIZE);
  memcpy(&(destination->username), source + USERNAME_OFFSET, USERNAME_SIZE);
  memcpy(&(destination->email), source + EMAIL_OFFSET, EMAIL_SIZE);
};

void print_row(Row* row) {
  printf("%d, %s, %s\n", row->id, row->username, row->email);
};

ExecuteResult execute_select(Statement* statement, Table* table) {
  Row row;
  for(uint32_t i = 0; i < table->num_rows; i++) {
    deserialize_row(row_slot(table, i), &row);
    print_row(&row);
  };

  return EXECUTE_SUCCESS;
};

ExecuteResult execute_insert(Statement* statement, Table* table) {
  if(table->num_rows >= TABLE_MAX_ROWS) {
    return EXECUTE_TABLE_FULL;
  };

  Row* row_to_insert = &(statement->row_to_insert);

  serialize_row(row_to_insert, row_slot(table, table->num_rows));
  table->num_rows += 1;

  return EXECUTE_SUCCESS;
};

ExecuteResult execute_statement(Statement* statement, Table* table) {
  switch(statement->type) {
    case(SELECT):
      return execute_select(statement, table);
    case(INSERT):
      return execute_insert(statement, table);
  };
};

Table* new_table() {
  Table* table = malloc(sizeof(Table));
  table->num_rows = 0;

  return table;
};

/* MAIN */

int main(int argc, char* argv[]) {
  InputBuffer* input_buffer = new_input_buffer();
  Table* table = new_table();
  while(true) {
    print_prompt();
    read_input(input_buffer);

    if(strncmp(input_buffer->buffer, ".", 1) == 0) {
      switch(execute_meta_command(input_buffer)) {
        case (META_COMMAND_SUCCESS):
          continue;
        case (META_COMMAND_FAIL):
          printf("Unrecognized command: '%s'.\n", input_buffer->buffer);
          continue;
      }
    };

    Statement statement;
    switch(prepare_statement(input_buffer, &statement)) {
      case(PREPARE_SUCCESS):
        break;
      case(PREPARE_NOOP):
        continue;
      case(PREPARE_NEGATIVE_ID):
        printf("Id cannot be negative.\n");
        continue;
      case(PREPARE_STR_TOO_LONG):
        printf("String is too long.\n");
        continue;
      case(PREPARE_SYNTAX_FAIL):
        printf("Syntax error, could not parse statement: '%s'.\n", input_buffer->buffer);
        continue;
      case(PREPARE_FAIL):
        printf("Unrecognized statement: '%s'.\n", input_buffer->buffer);
        continue;
    };

    switch(execute_statement(&statement, table)) {
      case(EXECUTE_SUCCESS):
        printf("Executed.\n");
        break;
      case(EXECUTE_TABLE_FULL):
        printf("Error: Table full.\n");
        break;
    };
  };
};
