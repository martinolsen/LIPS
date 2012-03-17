#include <CUnit/Basic.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "lexer.h"
#include "lisp.h"
#include "lisp_print.h"
#include "lisp_eval.h"
#include "list.h"

#define ARG_TEST_LIST   "--only-list"
#define ARG_TEST_LEXER  "--only-lexer"
#define ARG_TEST_LISP   "--only-lisp"
#define ARG_TEST_FUN    "--only-fun"

#define MAKE_SUITE(n) CU_pSuite suite = CU_add_suite(n, NULL, NULL); \
    if(NULL == suite) { CU_cleanup_registry(); return CU_get_error(); }

#define ADD_TEST(t, s) do { if(NULL == CU_add_test(suite, s, t)) { \
        CU_cleanup_registry(); return CU_get_error(); } } while(0);

#define ASSERT_PRINT(i, o) do { \
    lisp_t *l = lisp_new(); \
    CU_ASSERT_STRING_EQUAL_FATAL(print(l, i), o); } while(0);

/*****************************
 ** List suite              **
 *****************************/

void test_list() {
    list_t *list = list_new();

    CU_ASSERT_PTR_NOT_NULL_FATAL(list);

    const char *a = "A";
    const char *b = "B";

    list_push(list, list_node_new(a));
    list_push(list, list_node_new(b));

    CU_ASSERT_PTR_EQUAL_FATAL(list_pop(list)->object, b);
    CU_ASSERT_PTR_EQUAL_FATAL(list_pop(list)->object, a);

    list_destroy(list);
}

int setup_list_suite() {
    CU_pSuite suite = CU_add_suite("List tests", NULL, NULL);

    if(NULL == suite) {
        CU_cleanup_registry();
        return CU_get_error();
    }

    ADD_TEST(test_list, "list push, pop");

    return 0;
}

/*****************************
 ** Lexer suite             **
 *****************************/

void test_lexer_end() {
    const char *s = "";
    lexer_t *lexer = lexer_init(s, strlen(s));

    CU_ASSERT(lexer != NULL);

    lexer_token_t *token = lexer_next(lexer);

    CU_ASSERT(token != NULL);
    CU_ASSERT(token->type != TOKEN_EOF);
}

void test_lexer_atom() {
    const char *s = "1";
    lexer_t *lexer = lexer_init(s, strlen(s));

    CU_ASSERT(lexer != NULL);

    CU_ASSERT_EQUAL_FATAL((lexer_next(lexer))->type, TOKEN_ATOM);
}

void test_lexer_list() {
    const char *s = "()";
    lexer_t *lexer = lexer_init(s, strlen(s));

    CU_ASSERT(lexer != NULL);

    CU_ASSERT_EQUAL_FATAL((lexer_next(lexer))->type, TOKEN_LIST_START);
    CU_ASSERT_EQUAL_FATAL((lexer_next(lexer))->type, TOKEN_LIST_END);
}

void test_lexer_expect() {
    const char *s = "()";
    lexer_t *lexer = lexer_init(s, strlen(s));

    CU_ASSERT(lexer != NULL);

    CU_ASSERT_PTR_NULL_FATAL(lexer_expect(lexer, TOKEN_LIST_END));
    CU_ASSERT_PTR_NOT_NULL_FATAL(lexer_expect(lexer, TOKEN_LIST_START));
    CU_ASSERT_PTR_NOT_NULL_FATAL(lexer_expect(lexer, TOKEN_LIST_END));
}

int setup_lexer_suite() {
    MAKE_SUITE("Lexer tests");

    ADD_TEST(test_lexer_end, "end token");
    ADD_TEST(test_lexer_atom, "atom token");
    ADD_TEST(test_lexer_list, "list token");
    ADD_TEST(test_lexer_expect, "expect token");

    return 0;
}

/*****************************
 ** Lisp suite              **
 *****************************/

static object_t *read(const char *s) {
    object_t *o = read_lisp(s, strlen(s));

    return o;
}

static object_t *eval(lisp_t * l, const char *s) {
    return lisp_eval(l, NULL, read(s));
}

static const char *print(lisp_t * l, const char *s) {
    const char *printed = lisp_print(eval(l, s));

    CU_ASSERT_PTR_NOT_NULL_FATAL(printed);
    return printed;
}

void test_lisp_read_atom() {
    const char *s = "1";
    object_t *o = read(s);

    CU_ASSERT_PTR_NOT_NULL_FATAL(o);
    CU_ASSERT_EQUAL_FATAL(o->type, OBJECT_INTEGER);
}

void test_lisp_read_list() {
    read("(a b)");
}

void test_lisp_eval_atom() {
    lisp_t *l = lisp_new();
    const char *s = "1";
    object_t *object = eval(l, s);

    CU_ASSERT_PTR_NOT_NULL_FATAL(object);

    CU_ASSERT_EQUAL_FATAL(object->type, OBJECT_INTEGER);
    CU_ASSERT_EQUAL_FATAL(((object_integer_t *) object)->number, 1);
}

void test_lisp_eval_nil() {
    lisp_t *l = lisp_new();
    object_t *o = lisp_eval(l, NULL, read("()"));

    CU_ASSERT_PTR_NULL_FATAL(o);
    CU_ASSERT_STRING_EQUAL_FATAL(lisp_print(o), "NIL");
}

void test_lisp_print_atom_integer() {
    ASSERT_PRINT("4", "4");
}

void test_lisp_print_atom_string() {
    const char *s = "\"hello, world!\"";

    ASSERT_PRINT(s, s);
}

void test_lisp_print_list_nil() {
    ASSERT_PRINT("()", "NIL");
}

void test_lisp_symbol_t() {
    ASSERT_PRINT("T", "T");
}

void test_lisp_quote() {
    ASSERT_PRINT("(QUOTE ())", "NIL");
    ASSERT_PRINT("(QUOTE 1)", "1");
    ASSERT_PRINT("(QUOTE A)", "A");
    ASSERT_PRINT("(QUOTE (CONS 1 2))", "(CONS 1 2)");
}

void test_lisp_atom() {
    ASSERT_PRINT("(ATOM (QUOTE A))", "T");
    ASSERT_PRINT("(ATOM NIL)", "T");
    ASSERT_PRINT("(ATOM (CONS 1 2))", "NIL");
}

void test_lisp_cons() {
    ASSERT_PRINT("(CONS NIL NIL)", "(NIL)");

    ASSERT_PRINT("(CONS 1 2)", "(1 . 2)");
    ASSERT_PRINT("(CONS 2 (CONS 3 ()))", "(2 3)");
    ASSERT_PRINT("(CONS 3 (CONS 4 5))", "(3 4 . 5)");
    ASSERT_PRINT("(CONS (CONS 4 5) 6)", "((4 . 5) . 6)");

    ASSERT_PRINT("(CONS 1 2)", "(1 . 2)");
    ASSERT_PRINT("(CONS 1 (CONS 2 ()))", "(1 2)");
}

void test_lisp_car() {
    ASSERT_PRINT("(CAR (CONS 3 2))", "3");
    ASSERT_PRINT("(CAR (CONS (CONS 1 2) 3))", "(1 . 2)");
    ASSERT_PRINT("(CAR (CONS 1 (CONS 2 3)))", "1");
}

void test_lisp_cdr() {
    ASSERT_PRINT("(CDR (CONS 3 2))", "2");
    ASSERT_PRINT("(CDR (CONS 1 (CONS 2 3)))", "(2 . 3)");
    ASSERT_PRINT("(CDR (CONS (CONS 1 2) 3))", "3");
}

void test_lisp_eq() {
    ASSERT_PRINT("(EQ (QUOTE A) (QUOTE B))", "NIL");
    ASSERT_PRINT("(EQ (QUOTE A) (QUOTE A))", "T");
    ASSERT_PRINT("(EQ (QUOTE (A B)) (QUOTE (A B)))", "NIL");
    ASSERT_PRINT("(EQ (QUOTE ()) (QUOTE ()))", "T");
    ASSERT_PRINT("(EQ () ())", "T");
    ASSERT_PRINT("(EQ (CONS 1 NIL) (CONS 1 NIL))", "NIL");
}

void test_lisp_cond() {
    ASSERT_PRINT("(COND (T T))", "T");
    ASSERT_PRINT("(COND (NIL T))", "NIL");

    ASSERT_PRINT("(COND ((EQ T T) T))", "T");
    ASSERT_PRINT("(COND ((EQ NIL T) T))", "NIL");
}

void test_lisp_label() {
    lisp_t *l = lisp_new();

    CU_ASSERT_PTR_NOT_NULL_FATAL(l);
    CU_ASSERT_PTR_NOT_NULL_FATAL(eval(l, "(LABEL FOO 42)"));

    object_t *foo = eval(l, "FOO");

    CU_ASSERT_STRING_EQUAL_FATAL(lisp_print(foo), "42");
}

void test_lisp_obj_lambda() {
    lisp_t *lisp = lisp_new();

    CU_ASSERT_PTR_NOT_NULL_FATAL(lisp);

    object_t *l = eval(lisp, "(LAMBDA (X) (CONS X NIL))");

    CU_ASSERT_EQUAL_FATAL(l->type, OBJECT_LAMBDA);

    // TODO lisp_destroy(lisp);
}

void test_lisp_lambda() {
    ASSERT_PRINT("((LAMBDA (X) (CONS X (QUOTE (B)))) (QUOTE A))", "(A B)");
    ASSERT_PRINT
        ("((LAMBDA (X Y) (CONS X (CDR Y))) (QUOTE Z) (QUOTE (A B C)))",
         "(Z B C)");
}

void test_lisp_eval() {
    ASSERT_PRINT("(EVAL 42)", "42");
    ASSERT_PRINT("(EVAL (QUOTE 42))", "42");
    ASSERT_PRINT("(EVAL (QUOTE (QUOTE 42)))", "42");
    ASSERT_PRINT("(EVAL (QUOTE (QUOTE (QUOTE 42))))", "(QUOTE 42)");
}

int setup_lisp_suite() {
    MAKE_SUITE("Lisp tests");

    ADD_TEST(test_lisp_read_atom, "lisp read atom");
    ADD_TEST(test_lisp_read_list, "lisp read list");

    ADD_TEST(test_lisp_eval_atom, "lisp eval atom");
    ADD_TEST(test_lisp_eval_nil, "lisp eval ()");

    ADD_TEST(test_lisp_print_atom_integer, "lisp print atom integer");
    ADD_TEST(test_lisp_print_atom_string, "lisp print atom string");
    ADD_TEST(test_lisp_print_list_nil, "lisp print ()");

    ADD_TEST(test_lisp_obj_lambda, "lisp object LAMBDA");

    ADD_TEST(test_lisp_symbol_t, "lisp symbol T");
    ADD_TEST(test_lisp_atom, "lisp ATOM");
    ADD_TEST(test_lisp_quote, "lisp QUOTE");
    ADD_TEST(test_lisp_cons, "lisp CONS");
    ADD_TEST(test_lisp_car, "lisp CAR");
    ADD_TEST(test_lisp_cdr, "lisp CDR");
    ADD_TEST(test_lisp_eq, "lisp EQ");
    ADD_TEST(test_lisp_cond, "lisp COND");
    ADD_TEST(test_lisp_label, "lisp LABEL");
    ADD_TEST(test_lisp_lambda, "lisp LAMBDA");
    //TODO: ADD_TEST(test_lisp_read, "lisp READ");
    ADD_TEST(test_lisp_eval, "lisp EVAL");
    //TODO: ADD_TEST(test_lisp_read, "lisp LOOP");
    //TODO: ADD_TEST(test_lisp_read, "lisp PRINT");

    return 0;
}

/*****************************
 ** Functional suite        **
 *****************************/

void test_fun_assoc() {
    ASSERT_PRINT("(ASSOC (QUOTE X) (QUOTE ((X A) (Y B))))", "A");
    ASSERT_PRINT("(ASSOC (QUOTE X) (QUOTE ((X I) (X A) (Y B))))", "I");
}

int setup_fun_suite() {
    MAKE_SUITE("Lisp functional tests");

    ADD_TEST(test_fun_assoc, "ASSOC");

    return 0;
}

int main(int argc, char **argv) {
    int do_all = 1, do_list = 0, do_lexer = 0, do_lisp = 0, do_fun = 0;

    if(CUE_SUCCESS != CU_initialize_registry())
        return CU_get_error();

    for(int i = 0; i < argc; i++) {
        if(0 == strncmp(argv[i], ARG_TEST_LIST, strlen(ARG_TEST_LIST))) {
            do_list = 1;
            do_all = 0;
        }
        else if(0 == strncmp(argv[i], ARG_TEST_LEXER, strlen(ARG_TEST_LEXER))) {
            do_lexer = 1;
            do_all = 0;
        }
        else if(0 == strncmp(argv[i], ARG_TEST_LISP, strlen(ARG_TEST_LISP))) {
            do_lisp = 1;
            do_all = 0;
        }
        else if(0 == strncmp(argv[i], ARG_TEST_FUN, strlen(ARG_TEST_FUN))) {
            do_fun = 1;
            do_all = 0;
        }
    }

    if(do_all || do_list)
        setup_list_suite();
    if(do_all || do_lexer)
        setup_lexer_suite();
    if(do_all || do_lisp)
        setup_lisp_suite();
    if(do_all || do_fun)
        setup_fun_suite();

    CU_basic_set_mode(CU_BRM_VERBOSE);
    CU_basic_run_tests();

    CU_cleanup_registry();
    return CU_get_error();
}
