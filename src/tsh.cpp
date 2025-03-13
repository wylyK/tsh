
#include <tsh.h>

using namespace std;

/**
 * @brief
 * Helper function to print the PS1 pormpt.
 *
 * Linux has multiple promt levels
 * (https://wiki.archlinux.org/title/Bash/Prompt_customization): PS0 is
 * displayed after each command, before any output. PS1 is the primary prompt
 * which is displayed before each command. PS2 is the secondary prompt displayed
 * when a command needs more input (e.g. a multi-line command). PS3 is not very
 * commonly used
 */
void display_prompt() { cout << "$ " << flush; }

/**
 * @brief Cleans up allocated resources to prevent memory leaks.
 *
 * This function deletes all elements in the provided list of Process objects,
 * clears the list, and frees the memory allocated for the input line.
 *
 * @param process_list A pointer to a list of Process pointers to be cleaned up.
 * @param input_line A pointer to the dynamically allocated memory for user
 * input. This memory is freed to avoid memory leaks.
 */
void cleanup(list<Process *> &process_list, char *input_line) {
  for (Process *p : process_list) {
    delete p;
  }
  process_list.clear();
  free(input_line);
  input_line = nullptr;
}

/**
 * @brief Main loop for the shell, facilitating user interaction and command
 * execution.
 *
 * This function initializes a list to store Process objects, reads user input
 * in a loop, and executes the entered commands. The loop continues until the
 * user decides to quit.
 *
 * @note The function integrates several essential components:
 *   1. Displaying the shell prompt to the user.
 *   2. Reading user input using the read_input function.
 *   3. Parsing the input into a list of Process objects using parse_input.
 *   4. Executing the commands using run_commands.
 *   5. Cleaning up allocated resources to prevent memory leaks with the cleanup
 * function.
 *   6. Breaking out of the loop if the user enters the quit command.
 *   7. Continuously prompting the user for new commands until an exit condition
 * is met.
 */
void run() {
  list<Process *> process_list;
  char *input_line;
  bool is_quit = false;

  while (!is_quit) {
    display_prompt();
    if (!(input_line = read_input())) break;
    parse_input(input_line, process_list);
    is_quit = run_commands(process_list);
    cleanup(process_list, input_line);
  } 
}

/**
 * @brief Reads input from the standard input (stdin) in chunks and dynamically
 *        allocates memory to store the entire input.
 *
 * This function reads input from the standard input or file in chunks of size MAX_LINE.
 * It dynamically allocates memory to store the entire input, resizing the
 * memory as needed. The input is stored as a null-terminated string. The
 * function continues reading until a newline character is encountered or an
 * error occurs.
 *
 * @return A pointer to the dynamically allocated memory containing the input
 * string. The caller is responsible for freeing this memory when it is no
 * longer needed. If an error occurs or EOF is reached during input, the
 * function returns NULL.
 */
char *read_input() {
  char *input = NULL;
  char tempbuf[MAX_LINE];
  size_t inputlen = 0, templen = 0;

  while (fgets(tempbuf, MAX_LINE, stdin)) {
    templen = strlen(tempbuf);
    char *tempinput = (char*) realloc(input, inputlen + templen + 1);
    if (!tempinput) {
      free(input);
      return NULL;
    }
    input = tempinput;
    strcpy(input + inputlen, tempbuf);
    if (tempbuf[templen - 1] == '\n') {
      return input;
    }
    inputlen += templen;
  }

  return input;
}

/**
 * @brief
 * removes the new line char of the end in cmd. 
 */
void senetize(char *cmd) {
  char* tail = cmd + strlen(cmd) - 1;
  if (*tail == '\n') *tail = '\0';
}


/**
 * @brief Parses the given command string and populates a list of Process objects.
 *
 * This function takes a command string and a reference to a list of Process
 * pointers. It tokenizes the command based on the delimiters "|; " and creates
 * a new Process object for each token. The created Process objects are added to
 * the provided process_list. Additionally, it sets pipe flags for each Process
 * based on the presence of pipe delimiters '|' in the original command string.
 *
 * @param cmd The command string to be parsed.
 * @param process_list A reference to a list of Process pointers where the
 * created Process objects will be stored.
 */

bool is_delim(char c) {
  return (c == ' ' || c == ';' || c == '|' || c == '\0');
}

void parse_input(char *cmd, list<Process *> &process_list) {
  int pipe_in_val = 0;
  Process *currProcess = nullptr;

  list<char*> curr_tokens;
  char *curr_tok = NULL;
  char *curr_char = (char*) malloc(strlen(cmd) + 1);
  strcpy(curr_char, cmd);
  senetize(curr_char);

  bool stop = false;
  while (true) {
    stop = !*curr_char;
    if (is_delim(*curr_char)) {
      char delim = *curr_char;
      if (curr_tok) {
        *curr_char = '\0';
        curr_tokens.push_back(curr_tok);
        curr_tok = NULL;
      }

      if (delim != ' ' && !curr_tokens.empty()) {
        int pipe_out_val = delim == '|' ? 1 : 0;
        currProcess = new Process(pipe_in_val, pipe_out_val);
        pipe_in_val = pipe_out_val;
        for (char *token : curr_tokens) currProcess->add_token(token);
        curr_tokens.clear();
        process_list.push_back(currProcess);
      }
    } else if (!curr_tok) {
      curr_tok = curr_char;
    }

    if (stop) break;
    curr_char++;
  }
}

/**
 * @brief Check if the given command represents a quit request.
 *
 * This function compares the first token of the provided command with the
 * string "quit" to determine if the command is a quit request.
 *
 * @param p A pointer to a Process structure representing the command.
 *
 * @return A boolean indicating whether the command is a quit request (the first token is "quit").
 */
bool isQuit(Process *p) {
  return strcmp(p->cmdTokens[0], "quit") == 0;
}

/**
 * @brief Execute a list of commands using processes and pipes.
 *
 * This function takes a list of processes and executes them sequentially,
 * connecting their input and output through pipes if needed. It handles forking
 * processes, creating pipes, and waiting for child processes to finish.
 *
 * @param command_list A list of Process pointers representing the commands to
 * execute. Each Process object contains information about the command, such as
 *                     command tokens, pipe settings, and file descriptors.
 *
 * @return A boolean indicating whether a quit command was encountered during
 * execution. If true, the execution was terminated prematurely due to a quit
 * command; otherwise, false.
 *
 * @details
 * The function iterates through the provided list of processes and performs the
 * following steps:
 * 1. Check if a quit command is encountered. If yes, terminate execution.
 * 2. Create pipes and fork a child process for each command.
 * 3. In the parent process, close unused pipes, wait for child processes to
 * finish if necessary, and continue to the next command.
 * 4. In the child process, set up pipes for input and output, execute the
 * command using execvp, and handle errors if the command is invalid.
 * 5. Cleanup final process and wait for all child processes to finish.
 *
 * @note
 * - The function uses Process objects, which contain information about the
 * command and pipe settings.
 * - It handles sequential execution of commands, considering pipe connections
 * between them.
 * - The function exits with an error message if execvp fails to execute the
 * command.
 * - Make sure to properly manage file descriptors, close unused pipes, and wait
 * for child processes.
 * - The function returns true if a quit command is encountered during
 * execution; otherwise, false.
 */
bool run_commands(list<Process *> &command_list) {
  bool is_quit = false;
  int i = 0;
  int j = 0;
  int size = command_list.size();
  pid_t pids[size];
  Process *prev = nullptr;

  for (Process *curr : command_list) {
    int *curr_fd = curr->pipe_fd;
    int *prev_fd = prev ? prev->pipe_fd : NULL;
    bool curr_in = curr->pipe_in;
    bool curr_out = curr->pipe_out;

    if (isQuit(curr)) {
      is_quit = true;
      if (prev) {
        close(prev_fd[0]);
        close(prev_fd[1]);
      }
      break;
    }

    int pipe_failed;
    if ((curr_out && ((pipe_failed = pipe(curr_fd))))
    || (pids[i] = fork()) < 0 ) {
      if (curr_in && prev) {
        close(prev_fd[0]);
        close(prev_fd[1]);
      }

      if (curr_out && !pipe_failed) {
        close(curr_fd[0]);
        close(curr_fd[1]);
      }

      perror(pipe_failed ? "pipe failed" : "fork failed");
      exit(EXIT_FAILURE);
    }

    if (pids[i] == 0) {
      if (curr_in && prev) {
        close(prev_fd[1]);
        dup2(prev_fd[0], STDIN_FILENO);
        close(prev_fd[0]);
      }

      if (curr_out) {
        close(curr_fd[0]);
        dup2(curr_fd[1], STDOUT_FILENO); 
        close(curr_fd[1]);
      }

      execvp(curr->cmdTokens[0], curr->cmdTokens);
      if (errno == ENOENT) fprintf(stderr, "tsh: command not found: %s\n", curr->cmdTokens[0]);
      else perror("exec failed");
      exit(EXIT_FAILURE);
    }

    if (curr_in && prev) {
      close(prev_fd[0]);
      close(prev_fd[1]);
    }

    if (!curr_out) {
      while (j <= i) {
        waitpid(pids[i], nullptr, 0);
        j++;
      }
    }

    prev = curr;
    i++;
  }

  return is_quit;
}

/**
 * @brief Constructor for Process class.
 *
 * Initializes a Process object with the provided command string, input pipe,
 * and output pipe.
 *
 * @param _pipe_in The input pipe descriptor.
 * @param _pipe_out The output pipe descriptor.
 */
Process::Process(int _pipe_in, int _pipe_out) {
  pipe_in = _pipe_in;
  pipe_out = _pipe_out;
  i = 0;
}

/**
 * @brief Destructor for Process class.
 */
Process::~Process() {}

/**
 * @brief add a pointer to a command or flags to cmdTokens
 * 
 * @param tok 
 */
void Process::add_token(char *tok) {
  if (i <= 23) {
    cmdTokens[i] = tok;
    cmdTokens[++i] = NULL;
  }
}