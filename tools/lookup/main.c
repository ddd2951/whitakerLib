/* Usage: whitaker-lookup <word> */

#include <stdio.h>

#include <whitaker.h>

static WhitakerResult result;

int main(int argc, char* argv[]) {
  if (argc < 2) {
    fprintf(stderr, "usage: whitaker-lookup <word>\n");
    return 1;
  }
  if (whitaker_init() != WHITAKER_OK) {
    fprintf(stderr, "cannot initialize libwhitaker\n");
    return 1;
  }
  if (whitaker_analyze(argv[1], &result) != WHITAKER_OK) {
    fprintf(stderr, "error analyzing '%s'\n", argv[1]);
    return 1;
  }
  if (result.count == 0) {
    printf("'%s': not found\n", argv[1]);
    return 0;
  }
  for (int i = 0; i < result.count; ++i) {
    const WhitakerMatch* m = &result.matches[i];
    printf("[%d] orth=%s  pos=%s  inflection=%s\n    %s\n", i, m->orth, m->pos,
           m->inflection, m->meaning);
    for (uint8_t a = 0; a < m->addon_count; ++a)
      printf("    addon[%u] kind=%d spelling=%s\n        %s\n", a,
             (int)m->addons[a].kind, m->addons[a].spelling,
             m->addons[a].meaning);
  }
  return 0;
}
