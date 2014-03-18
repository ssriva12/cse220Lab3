
#include <stdio.h>
#include "scanner.h"

/*******************
 Static functions needed for the scanner
 You need to design the proper parameter list and 
 return types for functions with ???.
 ******************/
static char get_char();
static int skip_comment();
static int skip_blanks(char *string);
static TokenCode get_word(char *word);
static double get_number();
static void get_string(char *res);
static TokenCode get_special(char *res);
static void downshift_word(char *word);
static BOOLEAN is_reserved_word(char *word, TokenCode* code);

typedef enum
{
    LETTER, DIGIT, QUOTE, SPECIAL, EOF_CODE
}
CharCode;

/*********************
 Static Variables for Scanner
 Must be initialized in the init_scanner function.
 *********************/
static FILE *src_file;
static char src_name[MAX_FILE_NAME_LENGTH];
static char todays_date[DATE_STRING_LENGTH];
static CharCode char_table[256];  /* The character table*/
static char source_line[MAX_SOURCE_LINE_LENGTH];
static int line_index;


typedef struct
{
    char *string;
    TokenCode token_code;
}
RwStruct;

const RwStruct rw_table[9][10] = {
    {{"do",DO},{"if",IF},{"in",IN},{"of",OF},{"or",OR},{"to",TO},{NULL,0}}, /*Reserved words of size 2*/
    {{"and",AND},{"div",DIV},{"end",END},{"for",FOR},{"mod",MOD},{"nil",NIL},{"not",NOT},{"set",SET},{"var",VAR},{NULL,0}}, /*Reserved words of size 3*/
    {{"case",CASE},{"else",ELSE},{"file",FFILE},{"goto",GOTO},{"then",THEN},{"type",TYPE},{"with",WITH},{NULL,0}}, /*Reserved words of size 4*/
    {{"array",ARRAY},{"begin",BEGIN},{"const",CONST},{"label",LABEL},{"until",UNTIL},{"while",WHILE},{NULL,0}}, /* Reserved words of size 5*/
    {{"downto",DOWNTO}, {"packed",PACKED},{"record",RECORD}, {"repeat",REPEAT},{NULL,0}},  /* Reserved words of size 6*/
    {{"program", PROGRAM},{NULL,0}}, /* Reserved words of size 7*/
    {{"function", FUNCTION},{NULL,0}}, /* Reserved words of size 8*/
    {{"procedure", PROCEDURE},{NULL,0}}  /* Reserved words of size 9*/
};

/*Special characters table*/
const RwStruct sc_table[] = {
    {"^",UPARROW},{"*",STAR},{"(",LPAREN},{")",RPAREN},{"-",MINUS},{"+",PLUS},{"=",EQUAL},{"[",LBRACKET},
    {"]",RBRACKET},{":",COLON},{";",SEMICOLON},{"<",LT},{">",GT},{",",COMMA},{".",PERIOD},{"/",SLASH},{NULL,0},
    {":=",COLONEQUAL},{"<=",LE},{">=",GE},{"<>",NE},{"..",DOTDOT},{NULL,0}
};

Token* create_token()
{
  Token* new_token = (Token*)malloc(sizeof(Token));
  new_token->code = NO_TOKEN;
  new_token->string = (char*)malloc(MAX_TOKEN_STRING_LENGTH);
  *new_token->string = '\0';
  new_token->next = NULL;
  return new_token;
}

int free_token( Token* token )
{
  if(token->next != NULL)
    return -1;

  free(token->string);
  free(token);

  return 0;
}

void init_char_table()
{
  unsigned int i;
  const char specials[] = "^*()-+=[]:;<>,./";

  for(i = 0; i<256; ++i)
    char_table[i] = EOF;


  for(i = 0; i<sizeof(specials)-1; ++i)
    char_table[specials[i]] = SPECIAL;

  for (i = 'A'; i<='Z';++i)
    char_table[i] = LETTER;
  for (i = 'a'; i<='z';++i)
    char_table[i] = LETTER;
  /*Mark underscore as letter*/
  char_table['_'] = LETTER;

  for (i = '0'; i<='9';++i)
    char_table[i] = DIGIT;

  char_table['\''] = QUOTE;

  char_table[EOF] = EOF_CODE;

}

void init_scanner(FILE *source_file, char source_name[], char date[])
{
    src_file = source_file;
    strcpy(src_name, source_name);
    strcpy(todays_date, date);
    *source_line = '\0';
    line_index = 0;
    /*******************
     initialize character table, this table is useful for identifying what type of character 
     we are looking at by setting our array up to be a copy the ascii table.  Since C thinks of 
     a char as like an int you can use ch in get_token as an index into the table.
     *******************/
    init_char_table();
}

BOOLEAN get_source_line(char source_buffer[])
{
    char print_buffer[MAX_SOURCE_LINE_LENGTH + 9];
/*    char source_buffer[MAX_SOURCE_LINE_LENGTH];  //I've moved this to a function parameter.  Why did I do that?*/
    static int line_number = 0;

    if (fgets(source_buffer, MAX_SOURCE_LINE_LENGTH, src_file) != NULL)
    {
        ++line_number;
        sprintf(print_buffer, "%4d: %s", line_number, source_buffer);
        print_line(print_buffer, src_name, todays_date);
        return (TRUE);
    }
    else
        return (FALSE);
}
Token* get_token()
{
    char ch; /*This can be the current character you are examining during scanning.*/

    Token* new_token = create_token();  /*I am missing the most important variable in the function, what is it?  Hint: what should I return?*/

    /*No need for token string 'cause of we can store it in the initialized token structure*/

/*    char token_string[MAX_TOKEN_STRING_LENGTH]; //Store your token here as you build it.*/
/*    char *token_ptr = token_string; //write some code to point this to the beginning of token_string*/

    /*1.  Skip past all of the blanks*/
    line_index += skip_blanks(source_line + line_index);
    skip_comment();
    /*2.  figure out which case you are dealing with LETTER, DIGIT, QUOTE, EOF, or special, by examining ch*/
    ch = get_char();

    /*3.  Call the appropriate function to deal with the cases in 2.*/
    switch(char_table[ch])
    {
    case EOF_CODE:
      new_token->code = END_OF_FILE;
      break;
    case LETTER:
      new_token->code = get_word(new_token->string);
      break;
    case DIGIT:
      sprintf(new_token->string, "%g", get_number());
      new_token->code = NUMBER;
      break;
    case QUOTE:
      get_string(new_token->string);
      new_token->code = STRING;
      break;
    case SPECIAL: 
      new_token->code = get_special(new_token->string);
      break;
    default:
      ++line_index;
      break;
    }

    return new_token; /*What should be returned here?*/
}
static char get_char()
{
    /*
     If at the end of the current line (how do you check for that?),
     we should call get source line.  If at the EOF (end of file) we should
     set the character ch to EOF and leave the function.
     */
    
    while(source_line[line_index] == '\0')
    {
      if(get_source_line(source_line) == FALSE) 
        return EOF;
      /*skip blanks for the new line*/
      line_index = skip_blanks(source_line);
    }

    /* Write some code to set the character ch to the next character in the buffer */
    return source_line[line_index];
}
static int skip_blanks(char *string)
{
    char *beg_str = string;
    /*
     Write some code to skip past the blanks in the program and return a pointer
     to the first non blank character
     */
    while(isspace(*string))
        ++string;

    return string - beg_str;
}
static int skip_comment()
{
  char ch = get_char();
    /*
     Write some code to skip past the comments in the program and return a pointer
     to the first non blank character.  Watch out for the EOF character.
     */
    if(ch == '{')
    {
      while(ch != '}' && ch != EOF)
      {
        ++line_index;
        ch = get_char();
      }
      if(ch == '}')
        ++line_index;
    }
    return line_index;
}
static TokenCode get_word(char *word)
{
    /*
     Write some code to Extract the word
     */
    TokenCode code;
    int read_count = 0;
    char* word_begin = source_line + line_index;
    while(char_table[source_line[line_index]] == LETTER || char_table[source_line[line_index]] == DIGIT)
        ++line_index;
    read_count = source_line + line_index-word_begin;
    strncpy(word, word_begin, read_count);
    word[read_count] = '\0';
    /*Downshift the word, to make it lower case*/
    downshift_word(word);
    /*
     Write some code to Check if the word is a reserved word.
     if it is not a reserved word its an identifier.
     */
    if(!is_reserved_word(word, &code))
      return IDENTIFIER;

    return code;
}
static double get_number()
{
    /*
     Write some code to Extract the number and convert it to a literal number.
     */
  int n = 0;
  float val = .0;
  sscanf(source_line+line_index, "%f%n", &val, &n);
  line_index+=n;
  return val;
}
static void get_string(char* res)
{
    int cur_index = ++line_index;
    int length = 0;
    /*
     Write some code to Extract the string
     */
    while(source_line[line_index] != '\'' && source_line[line_index] != 0)
        ++line_index;
    length = line_index-cur_index;
    strncpy(res, source_line + cur_index, length);
    res[length] = '\0';
    ++line_index;
}
static TokenCode get_special(char *res)
{
    /*
     Write some code to Extract the special token.  Most are single-character
     some are double-character.  Set the token appropriately.
     */
    TokenCode ret = NO_TOKEN;
    const RwStruct* iterator;
    int cur_index = line_index;
    int length;

    iterator = sc_table;
    while(iterator->token_code != NO_TOKEN)
    {
      if(iterator->string[0] == source_line[line_index])
      {
        ++line_index;
        if(iterator->string[1] != '\0')
          if(iterator->string[1] == source_line[line_index+1])
          {
            ++line_index;
            ret = iterator->token_code;
            break;
          }

        ret = iterator->token_code;
        length = line_index-cur_index;
        strncpy(res, source_line + cur_index, length);
        res[length] = '\0';
        break;
      }
      ++iterator;
    }


    return ret;
}
static void downshift_word(char *word)
{
    /*Make all of the characters in the incoming word lower case.*/
    while(*word != 0)
    {
        *word = tolower(*word);
        ++word;
    }
}
static BOOLEAN is_reserved_word(char *word, TokenCode* code)
{
    /*
     Examine the reserved word table and determine if the function input is a reserved word.
     */
    int wordLen = strlen(word);
    const RwStruct* iterator;
    *code = NO_TOKEN;

    if(wordLen < 2 || wordLen > 9)
        return FALSE;

    iterator = rw_table[wordLen-2];/*the 0-indexed rw array has strings with size 2*/

    while(iterator->token_code != NO_TOKEN)
    {
        if(strcmp(word, iterator->string) == 0)
        {
            *code = iterator->token_code;
            return TRUE;
        }
      ++iterator;
    }

    return FALSE;
}
