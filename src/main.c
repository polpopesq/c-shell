#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char *argv[])
{
  // Flush after every printf
  setbuf(stdout, NULL);
  char buffer[1024];

  while (1)
  {
    printf("$ ");
    fgets(buffer, sizeof(buffer), stdin);
    buffer[strcspn(buffer, "\n")] = '\0';

    printf("%s: command not found\n", buffer);
  }

  return 0;
}
