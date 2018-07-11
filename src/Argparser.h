// Copyright (C) 2012-2015 Yecheng Fu <cofyc.jackson at gmail dot com>
// Copyright (C) 2018 Martin Weigel <mail@MartinWeigel.com>
// License: MIT
#pragma once
#include <stdbool.h>

//******************************************************************************
// Type definitions
//******************************************************************************
/**
 * Argparser context: Should be created by calling `Argparser_new()`.
 */
typedef struct Argparser Argparser;
/**
 * An option: Should be created using macros, e.g. ARGPARSER_OPT_BOOL(...).
 */
typedef struct ArgparserOption ArgparserOption;
/**
 * Callback-method definition used in ArgparserOption.
 * The callback is called after an ArgparserOption value is set.
 * Callbacks should be passed to macros, e.g. ARGPARSER_OPT_BOOL_CALLBACK(...).
 */
typedef void Argparser_callback(Argparser* self, const ArgparserOption* option);


//******************************************************************************
// Public functions
//******************************************************************************
/**
 * Allocates the memory structure for Argparser.
 */
Argparser* Argparser_new();
/**
 * Initializes Argparser with the given options.
 * @options:
 *      Array with all options. Options should be created using the macros below.
 */
void Argparser_init(Argparser* self, ArgparserOption* options);
/**
 * Optional method to set the usage-text in the help message.
 */
void Argparser_setUsage(Argparser* self, const char* usage);
/**
 * Optional method to set the text above the options in the help message.
 */
void Argparser_setDescription(Argparser* self, const char* description);
/**
 * Optional method to set an additional text below the options in the help message.
 */
void Argparser_setEpilog(Argparser* self, const char* epilog);
/**
 * The parser can stop parsing when first non-option occurs.
 * @stop:
 *      false: the parser parses all arguments (default).
 *      true: the parser skips options after first argument.
 *            E.g., `--opt1 arg --opt2` will be interpreted as `--opt1 -- arg --opt2`.
 *            Hence, it does not parse `--opt2` and returns it in `argv`.
 */
void Argparser_setStopAtNonOption(Argparser* self, bool stop);
/**
 * Parses the given command line arguments.
 * @return:
 *      Remaining amount of arguments in `argv`, usually set to `argc`.
 */
int Argparser_parse(Argparser* self, int argc, const char **argv);
/**
 * Default error-handler for parsing errors, exits program when called.
 * Should be called from callbacks if parsing errors occurs, e.g. if value is out-of-range.
 */
void Argparser_exitDueToError(Argparser* self, const ArgparserOption* option, const char* reason);
/**
 * Invalidates data inside the given Argparser context.
 */
void Argparser_clear(Argparser* self);
/**
 * Deletes the given Argparser context and frees its memory.
 */
void Argparser_delete(Argparser* self);


//******************************************************************************
// Public macros for creation of ArgparserOption
//******************************************************************************
#define ARGPARSER_OPT_BOOL(shortName, longName, valuePtr, description) \
    { ARGPARSER_TYPE_BOOLEAN, shortName, longName, valuePtr, description }
#define ARGPARSER_OPT_BOOL_CALLBACK(shortName, longName, valuePtr, description, callback) \
    { ARGPARSER_TYPE_BOOLEAN, shortName, longName, valuePtr, description, callback }

#define ARGPARSER_OPT_INT(shortName, longName, valuePtr, description) \
    { ARGPARSER_TYPE_INTEGER, shortName, longName, valuePtr, description }
#define ARGPARSER_OPT_INT_CALLBACK(shortName, longName, valuePtr, description, callback) \
    { ARGPARSER_TYPE_INTEGER, shortName, longName, valuePtr, description, callback }

#define ARGPARSER_OPT_FLOAT(shortName, longName, valuePtr, description) \
    { ARGPARSER_TYPE_FLOAT, shortName, longName, valuePtr, description }
#define ARGPARSER_OPT_FLOAT_CALLBACK(shortName, longName, valuePtr, description, callback) \
    { ARGPARSER_TYPE_FLOAT, shortName, longName, valuePtr, description, callback }

#define ARGPARSER_OPT_STRING(shortName, longName, valuePtr, description) \
    { ARGPARSER_TYPE_STRING, shortName, longName, valuePtr, description }
#define ARGPARSER_OPT_STRING_CALLBACK(shortName, longName, valuePtr, description, callback) \
    { ARGPARSER_TYPE_STRING, shortName, longName, valuePtr, description, callback }

#define ARGPARSER_OPT_GROUP(description)     { ARGPARSER_TYPE_GROUP, 0, NULL, NULL, description, NULL }
#define ARGPARSER_OPT_END()                  { ARGPARSER_TYPE_END, 0, NULL, NULL, 0, NULL }

void Argparser_exitForHelp(Argparser* self, const ArgparserOption* option);
#define ARGPARSER_OPT_HELP()       \
    ARGPARSER_OPT_BOOL_CALLBACK('h', "help", NULL, "show this help message and exit", Argparser_exitForHelp)



//******************************************************************************
// Definition of data structures
// Please only modify them through public functions
//******************************************************************************
enum ArgparserOptionType {
    ARGPARSER_TYPE_END,
    ARGPARSER_TYPE_GROUP,
    ARGPARSER_TYPE_BOOLEAN,
    ARGPARSER_TYPE_INTEGER,
    ARGPARSER_TYPE_FLOAT,
    ARGPARSER_TYPE_STRING,
};

typedef struct ArgparserOption {
    enum ArgparserOptionType type;
    const char shortName;
    const char *longName;
    void *value;
    const char *help;
    Argparser_callback *callback;
} ArgparserOption;

typedef struct Argparser {
    bool valid;
    const ArgparserOption *options;
    const char *usage;
    const char *description;
    const char *epilog;
    bool stopAtNonOption;
    // Internal variables
    int argc;
    const char **argv;
    const char **out;
    int cpidx;
} Argparser;



//******************************************************************************
// Implementation section. Include using:
// #define ARGPARSER_IMPLEMENTATION
// #include "Argparser.h"
//******************************************************************************
#ifdef ARGPARSER_IMPLEMENTATION

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <errno.h>

Argparser* Argparser_new()
{
    Argparser* self = malloc(sizeof(Argparser));
    return self;
}

void Argparser_init(Argparser* self, ArgparserOption* options)
{
    memset(self, 0, sizeof(*self));
    self->options = options;
    self->valid = true;
}

void Argparser_clear(Argparser* self)
{
    self->valid = false;
}

void Argparser_delete(Argparser* self)
{
    free(self);
}

void Argparser_setUsage(Argparser* self, const char* usage)
{
    assert(self->valid);
    self->usage = usage;
}

void Argparser_setDescription(Argparser* self, const char* description)
{
    assert(self->valid);
    self->description = description;
}

void Argparser_setEpilog(Argparser* self, const char* epilog)
{
    assert(self->valid);
    self->epilog = epilog;
}

void Argparser_setStopAtNonOption(Argparser* self, bool stop)
{
    assert(self->valid);
    self->stopAtNonOption = stop;
}

void Argparser_usage(Argparser* self)
{
    // print usage
    if (self->usage)
        fprintf(stdout, "Usage: %s\n", self->usage);

    // print description
    if (self->description)
        fprintf(stdout, "%s\n", self->description);

    const struct ArgparserOption *options;

    // figure out best width
    size_t usage_opts_width = 0;
    size_t len;
    options = self->options;
    for (; options->type != ARGPARSER_TYPE_END; options++) {
        len = 0;
        if (options->shortName) {
            len += 2;
        }
        if (options->shortName && options->longName) {
            len += 2;           // separator ", "
        }
        if (options->longName) {
            len += strlen(options->longName) + 2;
        }

        if (options->type == ARGPARSER_TYPE_INTEGER) {
            len += strlen("=<int>");
        } else if (options->type == ARGPARSER_TYPE_FLOAT) {
            len += strlen("=<float>");
        } else if (options->type == ARGPARSER_TYPE_STRING) {
            len += strlen("=<string>");
        }
        len = (len + 3) - ((len + 3) & 3);
        if (usage_opts_width < len) {
            usage_opts_width = len;
        }
    }
    usage_opts_width += 4;      // 4 spaces prefix

    options = self->options;
    for (; options->type != ARGPARSER_TYPE_END; options++) {
        size_t pos = 0;
        int pad    = 0;
        if (options->type == ARGPARSER_TYPE_GROUP) {
            fputc('\n', stdout);
            fprintf(stdout, "%s", options->help);
            fputc('\n', stdout);
            continue;
        }
        pos = fprintf(stdout, "    ");
        if (options->shortName) {
            pos += fprintf(stdout, "-%c", options->shortName);
        }
        if (options->longName && options->shortName) {
            pos += fprintf(stdout, ", ");
        }
        if (options->longName) {
            pos += fprintf(stdout, "--%s", options->longName);
        }
        
        if (options->type == ARGPARSER_TYPE_INTEGER) {
            pos += fprintf(stdout, "=<int>");
        } else if (options->type == ARGPARSER_TYPE_FLOAT) {
            pos += fprintf(stdout, "=<float>");
        } else if (options->type == ARGPARSER_TYPE_STRING) {
            pos += fprintf(stdout, "=<string>");
        }

        if (pos <= usage_opts_width) {
            pad = usage_opts_width - pos;
        } else {
            fputc('\n', stdout);
            pad = usage_opts_width;
        }
        fprintf(stdout, "%*s%s\n", pad + 2, "", options->help);
    }

    // print epilog
    if (self->epilog)
        fprintf(stdout, "\n%s\n", self->epilog);
}

void Argparser_exitForHelp(Argparser* self, const ArgparserOption* option)
{ 
    Argparser_usage(self);
    exit(0);
}

void Argparser_exitDueToUnknownOption(Argparser* self)
{
    fprintf(stderr, "error: unknown option `%s`\n", self->argv[0]);
    Argparser_usage(self);
    exit(1);
}

void Argparser_exitDueToError(Argparser* self, const ArgparserOption* option, const char* reason)
{
    fprintf(stderr, "error: option `-%c`/`--%s` %s\n", option->shortName, option->longName, reason);
    exit(1);
}

void Argparser_parseValue(Argparser* self, const ArgparserOption* option, const char *optvalue)
{
    const char *s = NULL;
    if (option->value) {
        switch (option->type) {
        case ARGPARSER_TYPE_BOOLEAN:
            if (optvalue) {
                if(strlen(optvalue) == 1 && optvalue[0] == '1') {
                    *(bool *)option->value = true;
                } else if(strlen(optvalue) == 1 && optvalue[0] == '0') {
                    *(bool *)option->value = false;
                } else {
                    Argparser_exitDueToError(self, option, "expects no value, 0, or 1");
                }
            } else 
                *(bool *)option->value = true;
            break;

        case ARGPARSER_TYPE_STRING:
            if (optvalue) {
                *(const char **)option->value = optvalue;
            } else {
                Argparser_exitDueToError(self, option, "requires a value");
            }
            break;

        case ARGPARSER_TYPE_INTEGER:
            errno = 0; 
            if (optvalue && strlen(optvalue) > 0) {
                *(int *)option->value = strtol(optvalue, (char **)&s, 0);
            } else {
                Argparser_exitDueToError(self, option, "requires a value");
            }
            if (errno) 
                Argparser_exitDueToError(self, option, strerror(errno));
            if (s[0] != '\0')
                Argparser_exitDueToError(self, option, "expects an integer value");
            break;

        case ARGPARSER_TYPE_FLOAT:
            errno = 0; 
            if (optvalue && strlen(optvalue) > 0) {
                *(float *)option->value = strtof(optvalue, (char **)&s);
            } else {
                Argparser_exitDueToError(self, option, "requires a value");
            }
            if (errno) 
                Argparser_exitDueToError(self, option, strerror(errno));
            if (s[0] != '\0')
                Argparser_exitDueToError(self, option, "expects a numerical value");
            break;
        default:
            assert(0);
        }
    }

    if (option->callback) {
        option->callback(self, option);
    }
}

void Argparser_parseShortOption(Argparser* self, const ArgparserOption* options)
{
    bool foundOption = false;

    const char *arg = self->argv[0];
    if (strlen(arg) == 2)
    {
        // Single options might carry a value
        for (; options->type != ARGPARSER_TYPE_END; options++) {
            if (options->shortName != 0 && options->shortName == *(self->argv[0]+1)) {
                if (self->argc > 1 && self->argv[1] && self->argv[1][0] != '-') {
                    // Use next argument as a value, remove it from arguments
                    Argparser_parseValue(self, options, self->argv[1]);
                    self->argv++;
                    self->argc--;
                    foundOption = true;
                } else {
                    Argparser_parseValue(self, options, NULL);
                    foundOption = true;
                }
            }
        }
    } else {
        // Compound arguments are always boolean without value
        for (const char* i = arg+1; *i != '\0'; i++) {
            foundOption = false;
            const ArgparserOption* optIterator = options;

            for (; optIterator->type != ARGPARSER_TYPE_END; optIterator++) {
                if (optIterator->shortName == *i) {
                    Argparser_parseValue(self, optIterator, NULL);
                    foundOption = true;
                }
            }
            if (!foundOption) {
                Argparser_exitDueToUnknownOption(self);
            }
        }
    }

    if (!foundOption) {
        Argparser_exitDueToUnknownOption(self);
    }
}

void Argparser_parseLongOption(Argparser* self, const ArgparserOption* options)
{
    bool foundOption = false;

    for (; options->type != ARGPARSER_TYPE_END; options++) {
        if (!options->longName || strlen(options->longName) < 1)
            continue;

        // Check if the option matches current name
        size_t nameLength = strlen(options->longName);
        const char *rest = strncmp(self->argv[0] + 2, options->longName, nameLength) ?
            NULL : self->argv[0] + 2 + nameLength;

        if (rest) {
            // Set the value of the current option
            if (*rest == '=') {
                Argparser_parseValue(self, options, rest+1);
                foundOption = true;
            } else {
                // There is no value; Only accept this for boolean or skip
                if (options->type == ARGPARSER_TYPE_BOOLEAN) {
                    Argparser_parseValue(self, options, NULL);
                    foundOption = true;
                }
            }
        }
    }

    if (!foundOption)
        Argparser_exitDueToUnknownOption(self);
}

int Argparser_parse(Argparser* self, int argc, const char **argv)
{
    assert(self->valid);

    // Skip executable path
    self->argc = argc - 1;
    self->argv = argv + 1;
    self->out  = argv;

    for (; self->argc; self->argc--, self->argv++) {
        const char *arg = self->argv[0];
        if (arg[0] != '-' || !arg[1]) {
            if (self->stopAtNonOption) {
                break;       // Finished, exit loop
            }
            // If it's not option or a single char '-', copy verbatim
            self->out[self->cpidx++] = self->argv[0];
            continue;
        }

        // Check if it is a short option
        if (arg[1] != '-') {
            Argparser_parseShortOption(self, self->options);
        } else {
            // Check if it is a long option
            if (arg[2]) {
                Argparser_parseLongOption(self, self->options);
            } else {
                // It is --, stop argument parsing
                self->argc--;
                self->argv++;
                break;       // Finished, exit loop
            }
        }
    }

    memmove(self->out + self->cpidx, self->argv,
            self->argc * sizeof(*self->out));
    self->out[self->cpidx + self->argc] = NULL;

    return self->cpidx + self->argc;
}

#endif
