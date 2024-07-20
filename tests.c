#include "byos.h"
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>

// Helper function to check file content
int check_file_content(const char *filename, const char *expected_content) {
  FILE *file = fopen(filename, "r");
  if (!file) {
    perror("fopen");
    return 1;
  }

  char buffer[1024];
  size_t n = fread(buffer, 1, sizeof(buffer) - 1, file);
  buffer[n] = '\0';

  fclose(file);
  return strcmp(buffer, expected_content) == 0 ? 0 : 1;
}

// Helper function to clear the contents of a file
void clear_file(const char *filename) {
  FILE *file = fopen(filename, "w");
  if (file) {
    fclose(file);
  }
}

int main(void) {
  int r;

  // Test 1: Redirect stdout to a file
  clear_file("test1.txt");
  struct cmd test1 = {.redir_stdout = "test1.txt",
                      .type = ECHO,
                      .data = {.echo = {"Hello, World!\n"}}};
  r = interp(&test1);
  if (r == 0 && check_file_content("test1.txt", "Hello, World!\n") == 0) {
    printf("Test 1 passed\n");
  } else {
    printf("Test 1 failed\n");
  }

  // Test 2: Restore original stdout after redirection
  clear_file("test2.txt");
  struct cmd test2_commands[2] = {{.redir_stdout = "test2.txt",
                                   .type = ECHO,
                                   .data = {.echo = {"Redirection\n"}}},
                                  {.redir_stdout = NULL,
                                   .type = ECHO,
                                   .data = {.echo = {"Original stdout\n"}}}};
  struct cmd test2 = {.redir_stdout = NULL,
                      .type = LIST,
                      .data = {.list = {2, test2_commands}}};
  r = interp(&test2);
  if (r == 0 && check_file_content("test2.txt", "Redirection\n") == 0) {
    printf("Test 2 passed\n");
  } else {
    printf("Test 2 failed\n");
  }

  // Test 3: Ensure FD_CLOEXEC works
  clear_file("test3.txt");
  char *ls_args[] = {"ls", NULL};
  struct cmd test3_commands[2] = {
      {.redir_stdout = "test3.txt",
       .type = FORX,
       .data = {.forx = {"ls", ls_args}}},
      {.redir_stdout = NULL, .type = ECHO, .data = {.echo = {"After ls\n"}}}};
  struct cmd test3 = {.redir_stdout = NULL,
                      .type = LIST,
                      .data = {.list = {2, test3_commands}}};
  r = interp(&test3);
  if (r == 0 && check_file_content("test3.txt", "After ls\n") != 0) {
    printf("Test 3 passed\n");
  } else {
    printf("Test 3 failed\n");
  }

  // Additional comprehensive test
  clear_file("f1");
  clear_file("f2");
  char *lsl[] = {"ls", "-l", NULL};
  char *catf1[] = {"cat", "f1", NULL};
  struct cmd innerarray[3] = {
      {.redir_stdout = NULL, .type = FORX, .data = {.forx = {"ls", lsl}}},
      {.redir_stdout = "f1", .type = ECHO, .data = {.echo = {"B\n"}}},
      {.redir_stdout = NULL, .type = FORX, .data = {.forx = {"cat", catf1}}}};
  struct cmd outerarray[3] = {
      {.redir_stdout = NULL, .type = ECHO, .data = {.echo = {"A\n"}}},
      {.redir_stdout = "f2", .type = LIST, .data = {.list = {3, innerarray}}},
      {.redir_stdout = NULL, .type = ECHO, .data = {.echo = {"C\n"}}}};
  struct cmd example1 = {
      .redir_stdout = NULL, .type = LIST, .data = {.list = {3, outerarray}}};

  r = interp(&example1);
  printf("return value = %d\n", r);
  // Expected outermost stdout: A\nC\n
  // Expected return value: 0
  // Expected f1: B\n
  // Expected f2: <output of ls -l, followed by:> B\n

  if (check_file_content("f1", "B\n") == 0) {
    printf("f1 content is correct\n");
  } else {
    printf("f1 content is incorrect\n");
  }

  FILE *f2 = fopen("f2", "r");
  if (f2) {
    char line[1024];
    int found_ls = 0, found_B = 0;
    while (fgets(line, sizeof(line), f2)) {
      if (strstr(line, "B")) {
        found_B = 1;
      }
      if (strstr(line,
                 "total")) { // assuming 'total' is part of the ls -l output
        found_ls = 1;
      }
    }
    fclose(f2);
    if (found_ls && found_B) {
      printf("f2 content is correct\n");
    } else {
      printf("f2 content is incorrect\n");
    }
  } else {
    printf("Failed to open f2\n");
  }

  return 0;
}
