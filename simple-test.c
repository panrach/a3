#include <stdio.h>
#include "byos.h"

int test1() {
  // {
  //   echo A
  //   { ls -l ; echo B > f1 ; cat f1 ; } > f2
  //   echo C
  // }
  char *lsl[] = { "ls", "-l", NULL };
  char *catf1[] = { "cat", "f1", NULL };
  struct cmd innerarray[3] = {
    { .redir_stdout = NULL,
      .type = FORX,
      .data = { .forx = { "ls", lsl } }
    },
    { .redir_stdout = "f1",
      .type = ECHO,
      .data = { .echo = { "B\n" } }
    },
    { .redir_stdout = NULL,
      .type = FORX,
      .data = { .forx = { "cat", catf1 } }
    }
  };
  struct cmd outerarray[3] = {
    { .redir_stdout = NULL,
      .type = ECHO,
      .data = { .echo = { "A\n" } }
    },
    { .redir_stdout = "f2",
      .type = LIST,
      .data = { .list = { 3, innerarray } }
    },
    { .redir_stdout = NULL,
      .type = ECHO,
      .data = { .echo = { "C\n" } }
    }
  };
  struct cmd example1 = {
    .redir_stdout = NULL,
    .type = LIST,
    .data = { .list = { 3, outerarray } }
  };
  return interp(&example1);
}


int main(void)
{
  int r;

  r = test1();
  printf("return value = %d\n", r);
  // Expected outermost stdout:
  // A
  // C
  // Expected return value: 0
  // Expected f1:
  // B
  // Expected f2:
  // <output of ls -l, followed by:>
  // B   # because of "cat f1"

  return 0;
}
