// shell_info.h

#ifndef SHELL_INFO_H
#define SHELL_INFO_H


// --------------------------------------------------------------- //
// structure  : struct shell_info
// description: Struct that keeps track of current information about
//              the shell during small c shell program
// --------------------------------------------------------------- //
struct shell_info {
  int  background;
  int  exit_status;
  int  output_redirect;
  int  input_redirect;
  char output_filename[256];
  char input_filename[256];
};

void init_shell_info(struct shell_info *info);

#endif
