#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "repl.h"
#include "lisp.h"
#include "lisp_eval.h"
#include "lisp_print.h"

#define BUFSZ 1024

struct source {
    size_t len;
    const char *text;
};

static struct source *read_source(void);

repl_t *repl_init(const char *prompt) {
    repl_t *repl = calloc(1, sizeof(repl_t));

    repl->lisp = lisp_new();
    repl->prompt = prompt;

    return repl;
}

void repl_destroy(repl_t * repl) {
    free(repl);
}

void repl_run(repl_t * repl) {
    struct source *src = read_source();

    object_t *robj = lisp_read(src->text, src->len);

    object_t *eobj = lisp_eval(repl->lisp, NULL, robj);

    printf("res: %s\n", lisp_print(eobj));
}

static struct source *read_source(void) {
    struct source *src = calloc(1, sizeof(struct source));

    if(src == NULL) {
        perror("calloc");
        exit(EXIT_FAILURE);
    }

    src->text = calloc(BUFSZ, sizeof(char));
    src->len = BUFSZ;

    fread((void *) src->text, sizeof(char), BUFSZ, stdin);

    return src;
}
