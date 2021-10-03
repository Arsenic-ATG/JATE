#include "headers/includes.h"
#include "headers/func_prototype.h"
#include "headers/global_var.h"
#include "headers/macros.h"

/************************** main() function *************************/
int main(int argc, char *argv[])
{
  enable_raw_mode();
  init_editor();
  if (argc >= 2)
    editor_open(argv[1]);

  editor_set_status_message ("HELP: Ctrl-S = save | Ctrl-Q = quit");

  while (1)
    {
      editor_refresh_screen();
      editor_process_keypress();
    }

  return 0;
}
