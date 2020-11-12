// shell_info.c

#include <string.h>
#include "src/shell_info.h"


// --------------------------------------------------------------- //
// function   : init_shell_info(..)
// parameters : struct shell_info *info
// description: Initialized the values of shell_info to 0 or empty
// --------------------------------------------------------------- //
void init_shell_info(struct shell_info *info) {
  info->background = 0;
  info->input_redirect = 0;
  info->output_redirect = 0;
  memset(info->input_filename, 0, sizeof(info->input_filename));
  memset(info->output_filename, 0, sizeof(info->output_filename));
}
