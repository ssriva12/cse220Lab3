
#include <stdio.h>
#include "common.h"
#include "print.h"
#include "scanner.h"

FILE *init_lister(const char *name, char source_file_name[], char dte[]);
void quit_scanner(FILE *src_file, Token *list);
void add_token_to_list(Token *list, Token *new_token);

int main(int argc, const char * argv[])
{
    Token *token;
    Token *token_list = create_token(); /*This needs to be implemented as a linked list in scanner.h.*/
    char source_name[MAX_FILE_NAME_LENGTH];
    char date[DATE_STRING_LENGTH];
    FILE *source_file = init_lister(argv[1], source_name, date);
    init_scanner(source_file, source_name, date);
    
    do
    {
        token = get_token();
        add_token_to_list(token_list, token);
        print_token(token);
    }
    while (!feof(source_file));/*What is the sentinal value that ends this loop?*/
    
    quit_scanner(source_file, token_list);

    return 0;
}
void add_token_to_list(Token *list, Token *new_token)
{
  /* Add new_token to the list knowing that list is a linked list.*/
  Token *last = list;
  while(last->next != NULL)
    last = last->next;

  last->next = new_token;
}
