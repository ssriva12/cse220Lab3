
#ifndef Lab3_scanner_h
#define Lab3_scanner_h

#include "common.h"
#include "print.h"

Token* create_token();
/*Release free token ("next" must be NULL)*/
int free_token(Token* token);

void init_scanner();
void init_char_table();
Token* get_token();

#endif
