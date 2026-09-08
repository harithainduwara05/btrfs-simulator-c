#include <stdio.h>
#include "../includes/fs_core.h"

int main() {
  raid1_print_status();
  printf("Run program Properly");
  char text[20] = "Hello World!\0";
  printf("\nchecksum is:- %d",calculate_checksum((const uint8_t *)text,sizeof(text)));
  return 0;
}
