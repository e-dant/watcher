#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

void phelp(FILE* file)
{
  char* help[] = {
    "Synopsis:",
    "  portable-destructive-rename [--help | <path-from> <path-to>]",
    "",
    "Example:",
    "  portable-destructive-rename a b",
    "",
    "Description:",
    "  A portable, simplified mv-like program.",
    "  Behaves like GNU mv -T.",
    "",
    "  If the source and destination paths are",
    "  on different filesystems, we'll link the",
    "  source to the destination, then unlink",
    "  the source from its original filesystem.",
  };
  unsigned long len = sizeof(help) / sizeof(char*);
  for (unsigned long i = 0; i < len; i++) fprintf(file, "%s\n", help[i]);
}

int main(int argc, char* argv[])
{
  char* a = argc > 1 ? argv[1] : "";
  char* b = argc > 2 ? argv[2] : "";

  if (strcmp(a, "--help") == 0)
    return (phelp(stdout), 0);

  else if (rename(a, b) == 0)
    return 0;

  else if (errno == EXDEV)
    if (link(a, b))
      return (perror("link"), 1);
    else if (unlink(a))
      return (perror("unlink"), 1);
    else
      return 0;

  else
    return (perror("rename"), 1);
}
