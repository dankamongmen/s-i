/*
 * Small infix evaluator, originally by Rune Holm <runeholm@student.matnat.uio.no>,
 * adapted for d-i by Steinar H. Gunderson <sgunderson@bigfoot.com>. Used with
 * permission.
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <assert.h>

#include "autopartkit.h"

static const char *origstr;
static const char *str;
static int token;

/* forward declarations, as it is called recursively */
static double expr(void);

static void eval_error(void)
{
    autopartkit_error(1, "Could not parse expression '%s', "
		      "error at position %d\n",
		      origstr, str - origstr);
}

static void match(int expected_token)
{
    if(token == expected_token)
        token = *str++; 
    else
        eval_error();
}

static double factor(void)
{
    char buf[256];
    int buflen = 0;
    
    double temp;
    if (token == '(') {
        match('(');
        temp = expr();
        match(')');
    } else if (isdigit(token) || token == '.' || token == '-') {
        do {
            buf[buflen++] = token;
            buf[buflen] = 0;
            token = *str++;
        } while (buflen < 255 && (isdigit(token) || token == '.'));

        if(!sscanf(buf, "%lf", &temp)) eval_error();
    } else if (token == '$') {
        token = *str++;
       	while (buflen < 255 && isalpha(token)) {
            buf[buflen++] = token;
            buf[buflen] = 0;
            token = *str++;
        };

        /* add more variables here as we need them */
        if (strcmp(buf, "RAMSIZE") == 0) {
	    assert(get_ram_size());
            temp = get_ram_size();
        } else {
            autopartkit_error(1, "Unknown variable $%s\n", buf);
        }
    } else {
        eval_error();
    }
    
    return temp;
}

static double term(void)
{
    double temp = factor();
    while ((token == '*') || (token == '/'))
        switch(token) {
        case '*': 
            match('*');
            temp *= factor();
            break;
        case '/':
            match('/');
            temp /= factor();
            break;
    }
    return temp;
}

static double expr(void)
{
    double temp = term();
    while ((token == '+') || (token == '-')) {
        switch(token) {
        case '+': 
            match('+');
            temp += term();
            break;
        case '-':
            match('-');
            temp -= term();
            break;
        }
    }
    return temp;
}

double evaluate(const char *expression)
{
    str = origstr = expression;
    token = *str++;
    return expr();
}
