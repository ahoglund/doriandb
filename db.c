#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* TYPES */

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
  PREPARE_FAIL
};

typedef enum PrepareResult_t PrepareResult;

enum StatementType_t {
  SELECT,
  INSERT
};

typedef enum StatementType_t StatementType;

struct Statement_t {
  StatementType type;
};

typedef struct Statement_t Statement;

InputBuffer* new_input_buffer() {
  InputBuffer* input_buffer = malloc(sizeof(InputBuffer));
  input_buffer->buffer = NULL;
  input_buffer->buffer_length = 0;
  input_buffer->input_length = 0;

  return input_buffer;
};


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


PrepareResult prepare_statement(InputBuffer* input_buffer, Statement* statement) {
  if(strncmp(input_buffer->buffer, "select", 6) == 0) {
    statement->type = SELECT;
    return PREPARE_SUCCESS;
  };

  if (strncmp(input_buffer->buffer, "insert", 6) == 0) {
    statement->type = INSERT;
    return PREPARE_SUCCESS;
  };

  return PREPARE_FAIL;
};

void execute_statement(Statement* statement) {
  switch(statement->type) {
    case(SELECT):
      printf("select here\n");
      break;
    case(INSERT):
      printf("insert here\n");
      break;
  }
}

/* MAIN */

int main(int argc, char* argv[]) {
  InputBuffer* input_buffer = new_input_buffer();
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
      case(PREPARE_FAIL):
        printf("Unrecognized statement: '%s'.\n", input_buffer->buffer);
        continue;
    };

    execute_statement(&statement);
    printf("Executed.\n");
  };
};
