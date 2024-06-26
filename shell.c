// Shell starter file
// You may make any changes to any part of this file.

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <sys/wait.h>
#include <pwd.h>
#include <errno.h>

#define COMMAND_LENGTH 1024
#define NUM_TOKENS (COMMAND_LENGTH / 2 + 1)
#define HISTORY_DEPTH 10
char history[HISTORY_DEPTH][COMMAND_LENGTH];
const int PBUFF_LEN = 2048;

// Converts a printf style-formatted input string into an
// 'absolute' string that can be used in write()
// Only supports a single "insert number", and assumes
// the appropriate format specifier, "%d", is in template_str
void my_printf(const char* template_str, int num) {
  int BUF_LEN = strlen(template_str) + 8;
  char dst[BUF_LEN];
  memset(dst, 0, BUF_LEN);
  sprintf(dst, template_str, num);
  write(STDOUT_FILENO, dst, BUF_LEN);
}

// Checks if a string is an integer
_Bool is_int(const char* str) {
  if (str == NULL || strlen(str) == 0) {
    return false;
  }
  else {
    for (int i = 0; i < strlen(str); ++i)
    {
      if (isdigit(str[i]) == 0) {
        return false;
      }
    }
    return true;
  }
}

/**
 * Command Input and Processing
 */

/*
 * Tokenize the string in 'buff' into 'tokens'.
 * buff: Character array containing string to tokenize.
 *       Will be modified: all whitespace replaced with '\0'
 * tokens: array of pointers of size at least COMMAND_LENGTH/2 + 1.
 *       Will be modified so tokens[i] points to the i'th token
 *       in the string buff. All returned tokens will be non-empty.
 *       NOTE: pointers in tokens[] will all point into buff!
 *       Ends with a null pointer.
 * returns: number of tokens.
 */
int tokenize_command(char *buff, char *tokens[]) {
  int token_count = 0;
  _Bool in_token = false;
  int num_chars = strnlen(buff, COMMAND_LENGTH);
  for (int i = 0; i < num_chars; i++) {
    switch (buff[i]) {
    // Handle token delimiters (ends):
    case ' ':
    case '\t':
    case '\n':
      buff[i] = '\0';
      in_token = false;
      break;

    // Handle other characters (may be start)
    default:
      if (!in_token) {
        tokens[token_count] = &buff[i];
        token_count++;
        in_token = true;
      }
    }
  }
  tokens[token_count] = NULL;
  return token_count;
}

/**
 * Read a command from the keyboard into the buffer 'buff' and tokenize it
 * such that 'tokens[i]' points into 'buff' to the i'th token in the command.
 * Return the number of tokens.
 * buff: Buffer allocated by the calling code. Must be at least
 *       COMMAND_LENGTH bytes long.
 * tokens[]: Array of character pointers which point into 'buff'. Must be at
 *       least NUM_TOKENS long. Will strip out up to one final '&' token.
 *       tokens will be NULL terminated (a NULL pointer indicates end of
 * tokens). in_background: pointer to a boolean variable. Set to true if user
 * entered an & as their last token; otherwise set to false.
 */
int read_command(char *buff, char *tokens[], _Bool *in_background) {
  *in_background = false;

  // Read input
  int length = read(STDIN_FILENO, buff, COMMAND_LENGTH - 1);

  if ((length < 0) && (errno != EINTR)) {
    perror("Unable to read command from keyboard. Terminating.\n");
    exit(-1);
  }

  // Null terminate and strip \n.
  buff[length] = '\0';
  if (buff[strlen(buff) - 1] == '\n') {
    buff[strlen(buff) - 1] = '\0';
  }

  // Tokenize (saving original command string)
  int token_count = tokenize_command(buff, tokens);
  if (token_count == 0) {
    return token_count;
  }

  // Extract if running in background:
  if (token_count > 0 && strcmp(tokens[token_count - 1], "&") == 0) {
    *in_background = true;
    // tokens[token_count - 1] = 0;
  }
  return token_count;
}

/*
 * Same as `read_command` but for
 * if buffer already correctly populated
*/
int read_buff_cmd(char *buff, char *tokens[], _Bool *in_background) {
  *in_background = false;

  // Null terminate and strip \n.
  // buff[strlen(buff) - 1] = '\0';
  if (buff[strlen(buff) - 1] == '\n') {
    buff[strlen(buff) - 1] = '\0';
  }

  // Tokenize (saving original command string)
  int token_count = tokenize_command(buff, tokens);
  if (token_count == 0) {
    return token_count;
  }

  // Extract if running in background:
  if (token_count > 0 && strcmp(tokens[token_count - 1], "&") == 0) {
    *in_background = true;
    // tokens[token_count - 1] = 0;
  }
  return token_count;
}

/*
 * Converts an array of tokens to a single newline-terminated string.
*/
void tokens_to_inbuf(char* inbuf, char* tokens[], int num_tokens, _Bool in_bkgnd) {
  // empty destination
  memset(inbuf, 0, COMMAND_LENGTH);
  // if in background, manually add '&' back in
  // if (in_bkgnd) {
  //   char tmp = '&';
  //   char* ptmp= &tmp;
  //   strcpy(tokens[num_tokens-1], ptmp);
  // }
  char tmp = ' ';
  char* ptmp = &tmp;
  for (int i=0; i < num_tokens-1; ++i) {
    //strcat(destination,source);
    strcat(inbuf, tokens[i]);
    strcat(inbuf, ptmp);
  }
  // don't want space after last one
  if (num_tokens > 0) {
    strcat(inbuf, tokens[num_tokens-1]);
  }
  // put newline back on
  strcat(inbuf, "\n");
}

/*
 * Add the command string in `cmd_buf` to history.
 * Updates `cmd_count`
*/
void append_history(char history[HISTORY_DEPTH][COMMAND_LENGTH], int* cmd_count, char cmd_buf[COMMAND_LENGTH]) {
  (*cmd_count) ++;
  // because of precrement
  if ((*cmd_count)-1 >= HISTORY_DEPTH)
  {
    // pull everything one towards front
    for (int i = 1; i < HISTORY_DEPTH; ++i) {
      memcpy(history[i-1], history[i], COMMAND_LENGTH);
    }
    memcpy(history[HISTORY_DEPTH-1], cmd_buf, COMMAND_LENGTH);
  }
  // the first 10 appends
  else
  {
    // newest at biggest index
    memcpy(history[*(cmd_count)-1], cmd_buf, COMMAND_LENGTH);
  }
  // write(STDOUT_FILENO, "appending to history:\n\t", 24);
  // write(STDOUT_FILENO, cmd_buf, COMMAND_LENGTH);
  // write(STDOUT_FILENO, "\n", 1);
}

void print_history(char history[HISTORY_DEPTH][COMMAND_LENGTH], int cmd_count) {
  if (cmd_count <= 10) {
    // only up to cmd_count
    for (int i = cmd_count-1; i >= 0; --i) {
      // print descending from current count
      my_printf("%d\t", i);
      write(STDOUT_FILENO, history[i], COMMAND_LENGTH);
    }
  }
  else {
    for (int i = HISTORY_DEPTH-1; i >= 0; --i) {
      // print descending from current count
      my_printf("%d\t", cmd_count - (HISTORY_DEPTH-1 - i));
      write(STDOUT_FILENO, history[i], COMMAND_LENGTH);
    }
  }
}

/*
 * Fetch the nth command in history into buff.
 * If requested out of range, return error -2
 * If no previous command, 2
 * If success, 0.
 */
int fetch_prev_cmd(char history[HISTORY_DEPTH][COMMAND_LENGTH], int cmd_count, char buff[COMMAND_LENGTH], int n) {
  // my_printf("cmd_cnt = %i. ", cmd_count);
  // my_printf("requested #%i.\n", n);
  if (cmd_count < 1) {
    write(STDOUT_FILENO, "Error: No Previous Command was found! Errno:-3\n", 48);
     return -3;
  }
  int arr_ind;
  if (cmd_count >= HISTORY_DEPTH) {
    arr_ind = HISTORY_DEPTH - cmd_count-1 + n;
  }
  else {
    arr_ind = n;
  }
  // check if in range
  if (0 <= arr_ind && arr_ind < 10)
  {
    strcpy(buff, history[arr_ind]);
    return 0;
  }
  write(STDOUT_FILENO, "Error: User requested command that was out of bounds! Errno:-2\n", 64);
  return -2; 
}

/*
 * Print the messages for
 * 'help' without any arguments
*/
void disp_help_menu() {
  write(STDOUT_FILENO, "\n", 1);
  write(STDOUT_FILENO, "'help' is a builtin command that describes other builtin commands, including internal ones:\n", 93);
  write(STDOUT_FILENO, "- 'cd' changes the current working directory.\n", 47);
  write(STDOUT_FILENO, "- 'pwd' displays the current working directory.\n", 48);
  write(STDOUT_FILENO, "- 'exit' terminates the shell.\n", 32);
}

/*
 * Prints the current working directory.
*/
void printwd() {
  char buff[PBUFF_LEN];
  memset(buff, 0, PBUFF_LEN);
  getcwd(buff, PBUFF_LEN);
  write(STDOUT_FILENO, buff, PBUFF_LEN);
}

/**
 * Main and Execute Commands
 */
int main(int argc, char *argv[]) {
  // user input stuff
  char input_buffer[COMMAND_LENGTH];
  char* tokens[NUM_TOKENS];
  char store_buffer[COMMAND_LENGTH];
  int cmd_count = 0;
  
  // keep track of the last working directory
  char prevwd[COMMAND_LENGTH];
  getcwd(prevwd, COMMAND_LENGTH);

  // for waitpid() reporting
  int* status = NULL;

  while (true) {
    // Get command
    // Use write because we need to use read() to work with
    // signals, and read() is incompatible with printf().

    // Set up signal handler
    struct sigaction sig_handler;
    sig_handler.sa_handler = disp_help_menu;
    sig_handler.sa_flags = 0;
    sigemptyset(&sig_handler.sa_mask);
    sigaction(SIGINT, &sig_handler, NULL);

    // Prompt user
    printwd();
    write(STDOUT_FILENO, "$ ", strlen("$ "));

    _Bool in_background = false;
    int num_tokens = read_command(input_buffer, tokens, &in_background);

    if (num_tokens == 0) {
      continue;
    }
    // if '!' command, replace with
    // requested command from history
    // before all other actions
    else if (strcmp(tokens[0], "!!") == 0) {
      // run with prev one
      // write(STDOUT_FILENO, "\n", 1);
      // my_printf("fetching # %i\n", cmd_count-1);
      // error
      if (0 < fetch_prev_cmd(history, cmd_count, input_buffer, cmd_count-1)) {
        // /\ already prints error
        continue;
      }
      write(STDOUT_FILENO, input_buffer, strlen(input_buffer));
      // write(STDOUT_FILENO, "\n", 1);
      num_tokens = read_buff_cmd(input_buffer, tokens, &in_background);
    }
    else if (tokens[0][0] == '!') {
      // char* int_test = &tokens[0][0];
      if (is_int(&(tokens[0][1])))
      {
        // write(STDOUT_FILENO, "\n", 1);
        // my_printf("fetching # %i\n", atoi(&(tokens[0][1])));
        if (0 < fetch_prev_cmd(history, cmd_count, input_buffer, atoi(&(tokens[0][1])))) {
          continue;
        }
        write(STDOUT_FILENO, input_buffer, strlen(input_buffer));
        // write(STDOUT_FILENO, "\n", 1);
        num_tokens = read_buff_cmd(input_buffer, tokens, &in_background);
        // write(STDOUT_FILENO,"tokens[0] = ", 12);
        // write(STDOUT_FILENO, tokens[0], strlen(tokens[0]));
        // write(STDOUT_FILENO, "\n", 1);
      }
    }

    // add to history
    tokens_to_inbuf(store_buffer, tokens, num_tokens, in_background);
    // Note: \/ increments cmd_count
    append_history(history, &cmd_count, store_buffer);

    // internal commands -- don't need to split off into a child process
    if (strcmp(tokens[0], "history") == 0) {
      if (num_tokens > 1)
      {
        write(STDOUT_FILENO, "Error: user entered extraneous input. Aborting.\n", 49);
        continue;
      }
      print_history(history, cmd_count);
    }
    else if (strcmp(tokens[0], "exit") == 0) {
      if (num_tokens > 1)
      {
        write(STDOUT_FILENO, "Error: user entered extraneous input. Aborting.\n", 49);
        continue;
      }
      exit(0);
    }
    else if (strcmp(tokens[0], "pwd") == 0) {
     if (num_tokens > 1)
      {
        write(STDOUT_FILENO, "Error: user entered extraneous input. Aborting.\n", 49);
        continue;
      }
      printwd();
      write(STDOUT_FILENO, "\n", 1);
    }
    else if (strcmp(tokens[0], "cd") == 0) {
      if (num_tokens != 2)
      {
        write(STDOUT_FILENO, "Error: user must enter 'cd' and one argument. Aborting.\n", 57);
        continue;
      }
      // extra preprocessing for better `cd`
      if (tokens[1][0] == '~')
      {
        struct passwd* user_info = getpwuid(getuid());
        int cur_path_len = strlen(tokens[1]);
        char* home_dir = user_info->pw_dir;
        // just a tilde
        if (cur_path_len == 1) {
          tokens[1] = home_dir;
        }
        else if (cur_path_len > 1) {
          char final_path[cur_path_len + strlen(home_dir)];
          // copy out the latter part of the path
          strcpy(final_path, home_dir);
          // subbing out the leading "~"
          strcat(final_path, tokens[1] + 1);
          strcpy(tokens[1], final_path);
        }
        write(STDOUT_FILENO, "Path now ", 9);
        write(STDOUT_FILENO, tokens[1], strlen(tokens[1]));
        write(STDOUT_FILENO, "\n", 1);
      }
      else if (strcmp(tokens[1], "-") == 0) {
        // go back
        tokens[1] = prevwd;
      }
      // copy out the old working dir before switching
      char prevbuf[COMMAND_LENGTH];
      getcwd(prevbuf, COMMAND_LENGTH);

      if (chdir(tokens[1]) < 0) {
        write(STDOUT_FILENO, "Error: chdir failed to change to given path.\n", 46);
      }
      else {
        strcpy(prevwd, prevbuf);
      }
    }
    else if (strcmp(tokens[0], "help") == 0) {
      if (num_tokens > 2)
      {
        write(STDOUT_FILENO, "Error: user entered extraneous input. Aborting.\n", 49);
        continue;
      }
      else if (num_tokens == 2)
      {
        if (strcmp(tokens[1], "cd") == 0) {
          write(STDOUT_FILENO, "'cd' is a builtin command for changing the current working directory.\n", 71);
        }
        else if (strcmp(tokens[1], "pwd") == 0) {
          write(STDOUT_FILENO, "'pwd' is a builtin command for displaying the current working directory.\n", 74);
        }
        else if (strcmp(tokens[1], "exit") == 0) {
          write(STDOUT_FILENO, "'exit' is a builtin command to terminate the shell.\n", 53);
        }
        else {
          write(STDOUT_FILENO, "'", 1);
          write(STDOUT_FILENO, tokens[1], strlen(tokens[1]));
          write(STDOUT_FILENO, "'", 1);
          write(STDOUT_FILENO, " is an external command or application.\n", 41);
        }
      }
      // just `help`
      else {
        disp_help_menu();
      }
    }

    // not internal command
    else if (errno != EINTR)
    {
      // my_printf("Parent pid = %d\n", getpid());
      // fork the command into the right process
      pid_t child_pid = fork();
      // my_printf("spawned %d\n", child_pid);

      if (child_pid < 0) {
        write(STDOUT_FILENO, "Error: fork failed.\n", 16);
        // return 1;
        exit(-1);
      }
      // in child process
      else if (child_pid == 0)
      {
        // don't pass & to exec
        if (in_background) {
          tokens[num_tokens-1] = 0;
        }
        // load in the program for this command
        if (execvp(tokens[0], tokens) < 0) {
          write(STDOUT_FILENO, "Error: '", 8);
          write(STDOUT_FILENO, tokens[0], strlen(tokens[0]));
          write(STDOUT_FILENO, "' not found.\n", 14);
          exit(1);
        }
      }
      // in parent process
      else
      {
        if (!in_background) {
          waitpid(child_pid, status, 0); 
        }
      }
      // kill zombies
      pid_t killed;
      do {
        killed = waitpid(-1, NULL, WNOHANG);
      } while (killed > 0);
    }

    /**
     * Steps For Basic Shell:
     * 1. Fork a child process
     * 2. Child process invokes execvp() using results in token array.
     * 3. If in_background is false, parent waits for
     *    child to finish. Otherwise, parent loops back to
     *    read_command() again immediately.
     */
  }

    // DEBUG: Dump out arguments:
    // for (int i = 0; tokens[i] != NULL; i++) {
    //   write(STDOUT_FILENO, "   Token: ", strlen("   Token: "));
    //   write(STDOUT_FILENO, tokens[i], strlen(tokens[i]));
    //   write(STDOUT_FILENO, "\n", strlen("\n"));
    // }
    // if (in_background) {
    //   write(STDOUT_FILENO, "Run in background.", strlen("Run in background."));
    // }
    // for (int i = 0; tokens[i] != NULL; i++) {
    //   write(STDOUT_FILENO, "   Token: ", strlen("   Token: "));
    //   write(STDOUT_FILENO, tokens[i], strlen(tokens[i]));
    //   write(STDOUT_FILENO, "\n", strlen("\n"));
    // }
  
  return 0;
}
