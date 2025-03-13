#include <fcntl.h>
#include <gtest/gtest.h>
#include <unistd.h>
#include <fstream>
#include <iostream>
#include <string>
#include <ctime>
#include <tsh.h>

using namespace std;


void write_line(const string &filename, const char *msg) {
  std::ofstream outfile(filename);
  outfile << msg;
  outfile.close();
}

struct RedirectionContext {
  FILE *original_stdin;
  FILE *redirected_file;
  int file_descriptor;
};

RedirectionContext setup_stdin_redirection(const char *filename) {
  RedirectionContext context = {nullptr, nullptr, -1};

  context.file_descriptor = open(filename, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
  if (context.file_descriptor < 0) {
    perror("Failed to open file");
    return context;
  }

  context.redirected_file = fopen(filename, "r");
  if (!context.redirected_file) {
    perror("Failed to open file as FILE*");
    close(context.file_descriptor);
    return context;
  }

  context.original_stdin = stdin;   // Store the original stdin
  stdin = context.redirected_file;  // Redirect stdin
  dup2(context.file_descriptor, STDIN_FILENO);

  return context;
}

void restore_stdin_redirection(RedirectionContext &context,
                               const char *filename) {
  if (context.original_stdin) {
    stdin = context.original_stdin;  // Restore the original stdin
  }

  if (context.redirected_file) {
    fclose(context.redirected_file);
  }

  if (context.file_descriptor >= 0) {
    close(context.file_descriptor);
  }

  if (filename) {
    remove(filename);  // Cleanup the file
  }
}

// test quit
TEST(ShellTest, Quit) {
  Process p(0, 0);
  p.add_token((char *)"quit");

  EXPECT_TRUE(isQuit(&p)) << "passing quit should return true" << endl;
}

// // test exit
TEST(ShellTest, NotQuit) {
  Process p(0, 0);
  p.add_token((char *)"exit");

  EXPECT_FALSE(isQuit(&p)) << "passing quit should return true" << endl;
}

TEST(ShellTest, TestSimpleRun) {
  std::string expected_output = "$ hello\n$ ";
  char input_string[1092] = "echo hello";

  const char *filename = "hello.txt";
  write_line(filename, input_string);

  testing::internal::CaptureStdout();
  RedirectionContext context = setup_stdin_redirection(filename);

  // run your test
  run();

  std::string output = testing::internal::GetCapturedStdout();
  restore_stdin_redirection(context, filename);

  EXPECT_TRUE(output == expected_output) << "Your Output\n"
                                         << output << "\nexpected outputs:\n"
                                         << expected_output;
}

int main(int argc, char **argv) {
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
