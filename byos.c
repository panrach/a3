#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

#include "byos.h"

int interp(const struct cmd *c) {
  int original_stdout_fd = -1;
  int out_fd = -1;

  // Handle stdout redirection
  if (c->redir_stdout) {
    out_fd = open(c->redir_stdout, O_WRONLY | O_CREAT | O_TRUNC, 0666);
    if (out_fd < 0) {
      perror("open");
      return 1; // Failed to open file for redirection
    }

    // Set the FD_CLOEXEC flag
    if (fcntl(out_fd, F_SETFD, FD_CLOEXEC) < 0) {
      perror("fcntl");
      close(out_fd);
      return 1; // Failed to set FD_CLOEXEC
    }

    original_stdout_fd = dup(STDOUT_FILENO);
    if (original_stdout_fd < 0) {
      perror("dup");
      close(out_fd);
      return 1;
    }

    if (dup2(out_fd, STDOUT_FILENO) < 0) {
      perror("dup2");
      close(out_fd);
      return 1; // Failed to redirect stdout
    }
    close(out_fd);
  }

  int return_value = 0;
  
  switch (c->type) {
  case ECHO: {
    size_t len = strlen(c->data.echo.arg);
    if (write(STDOUT_FILENO, c->data.echo.arg, len) != len) {
      perror("write");
      return_value = 1; // Writing failed
    }
    break;
  }

  case FORX: {
    pid_t pid = fork();
    if (pid == 0) {
      // child exits and exit status is 127
      execvp(c->data.forx.pathname, c->data.forx.argv);
      perror("execvp");
      exit(127);
    } else if (pid > 0) {
      int status;
      waitpid(pid, &status, 0);
      // If the child exits, the return value is the child’s exit status.
      if (WIFEXITED(status)) {
        return_value = WEXITSTATUS(status);
      }
      // If the child is killed by signal, the return value is 128+signal
      else if (WIFSIGNALED(status)) {
        return_value = WEXITSTATUS(status) + 128;
      }
    }
    break;
  }

  case LIST: {
    if (c->data.list.n == 0) {
      return_value = 0; // No commands to execute
    } else {
      for (int i = 0; i < c->data.list.n; i++) {
        return_value = interp(&c->data.list.cmds[i]);
      }
    }
    break;
  }
  }

  // Restore original stdout if redirected
  if (original_stdout_fd >= 0) {
    if (dup2(original_stdout_fd, STDOUT_FILENO) < 0) {
      perror("dup2");
      return 1; // Failed to restore original stdout
    }
    close(original_stdout_fd);
  }

  return return_value;
}