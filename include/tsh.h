#ifndef _SIMPLE_SHELL_H
#define _SIMPLE_SHELL_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <iostream>
#include <list>
#include <vector>
#include <map>
#include <map>
#include <functional>
#include <list>
#include <cstring>

#ifdef DEBUGMODE
#define debug(msg) \
    std::cout <<"[" << __FILE__ << ":" << __LINE__ << "] " << msg << std::endl;
#else
#define debug(msg)
#endif

using namespace std;

#define MAX_LINE 81

class Process {
 public:
  Process(int _pipe_in, int _pipe_out);
  ~Process();

  void add_token(char *tok);
  char *cmdTokens[25];

  bool pipe_in;
  bool pipe_out;

  int pipe_fd[2];
  int i;
};

void run();
void display_prompt();
void cleanup(list<Process *> &process_list, char *input_line);
char *read_input();
void parse_input(char *input_line, list<Process *> &process_list);
bool run_commands(list<Process *> &command_list);
bool isQuit(Process *process);

#endif
