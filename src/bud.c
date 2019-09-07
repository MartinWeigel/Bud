// Copyright (C) 2019 Martin Weigel <mail@MartinWeigel.com>
//
// Permission to use, copy, modify, and/or distribute this software for any
// purpose with or without fee is hereby granted, provided that the above
// copyright notice and this permission notice appear in all copies.
//
// THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
// WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
// MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
// ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
// WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
// ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
// OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.

// Version: 2019-02-05
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#define ARGPARSER_IMPLEMENTATION
#include "Argparser.h"

#ifdef _WIN32
#include <windows.h>
static char* HORIZONTAL_LIGN = "-";
static char* CHART_FILLER = "#";
static char* CHART_BORDER_LEFT = "|";
static char* CHART_BORDER_RIGHT = "|";
#else
#include <sys/ioctl.h>
static char* HORIZONTAL_LIGN = "─";
static char* CHART_FILLER = "▆";
static char* CHART_BORDER_LEFT = "▕";
static char* CHART_BORDER_RIGHT = "▏";
#endif


#define max(a,b) (((a) > (b)) ? (a) : (b))
#define min(a,b) (((a) < (b)) ? (a) : (b))
#define abs(a)   (((a) >= 0) ? (a) : -(a))

#define ANSI_COLOR_RED      "\x1b[31m"
#define ANSI_COLOR_GREEN    "\x1b[32m"
#define ANSI_COLOR_RESET    "\x1b[0m"

const int MAX_CHART_SIZE = 100;
const int CHART_OFFSET = 15 + 1 + 9 + 1;
const int BUFFERSIZE = 256;
const char *SEPARATOR_CSV = " \t";
const char *SEPARATOR_CURRENCY = ",.";

// Input variables
int inverse = 0;
int nochart = 0;
int colorOutput = 0;
int noheader = 0;
int nototal = 0;

// Data structure for categories
typedef struct bucket
{
    char *category;
    long totalCents;
    struct bucket *nextBucket;
} bucket;
bucket* buckets;

// Track the positive and negative totals
long positiveTotalCents = 0;
long negativeTotalCents = 0;

char *strdup (const char *s)
{
    char *d = malloc(strlen(s) + 1);
    if (d != NULL) {
        strcpy(d, s);
        return d;
    } else
        return NULL;
}

FILE* chooseInput(int argc, const char **argv)
{
    FILE *input;
    if (argc <= 0) {
        input = stdin;
    } else {
        input = fopen(*argv, "r");

        if (NULL == input) {
            fprintf(stderr, "Unable to open '%s': %s\n", *argv, strerror(errno));
            exit(EXIT_FAILURE);
        }
    }
    return input;
}

void addEntryToBucket(char* category, long cents)
{
    struct bucket* current = buckets;
    // Try to add the money to an existing category
    while (current != NULL) {
        if (strcmp(category, current->category) == 0) {
            current->totalCents = current->totalCents + cents;
            return;
        }
        current = current->nextBucket;
    }

    // No existing entry found, create new one
    struct bucket *newCategory = malloc(sizeof(struct bucket));
    newCategory->category = strdup(category);
    newCategory->totalCents = cents;
    newCategory->nextBucket = buckets;
    buckets = newCategory;
}

void processEntry(unsigned int lineno, char* line)
{
    char* day = strtok(line, SEPARATOR_CSV);
    char* category = strtok(NULL, SEPARATOR_CSV);
    char* euros = strtok(NULL, SEPARATOR_CURRENCY);
    char* cents = strtok(NULL, SEPARATOR_CSV);

    if(day != NULL && category != NULL && euros != NULL && cents != NULL) {
        // Concat cents and euros while respecting the sign
        long total = atoi(euros) * 100;
        if (total >= 0)
            total += atoi(cents);
        else
            total -= atoi(cents);

        // Inverse entry if argument is given
        if(inverse)
            total = -total;

        // Add the entry to a bucket
        addEntryToBucket(category, total);
    } else {
        // Ignore empty lines, but show error otherwise
        if(strspn(line, " \r\n\t") != strlen(line))
            printf("WARNING: Entry ignored. Parsing error in line %d.\n", lineno);
    }
}

// Calculate chartwidth
int calculateChartwidth()
{
#ifdef WIN32
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi);
    int width = csbi.srWindow.Right - csbi.srWindow.Left + 1;
    return min(MAX_CHART_SIZE, width - CHART_OFFSET - 2);
#else
    struct winsize w;
    ioctl(0, TIOCGWINSZ, &w);
    return min(MAX_CHART_SIZE, w.ws_col - CHART_OFFSET - 2);
#endif
}


// Prints a chart if not deactivated
void printChart(int chartWidth, float percentage, char* in, char out)
{
    float charStep = 100.0 / chartWidth;
    percentage = min(100, percentage);

    printf("%s", CHART_BORDER_LEFT);
    for(int i=1; i <= chartWidth; i++) {
        if(percentage >= charStep * i) {
            printf("%s", in);
        } else {
            printf("%c", out);
        }
    }
    printf("%s", CHART_BORDER_RIGHT);
}

void printChartOrPercent(int chartWidth, float percentage)
{
    if(nochart) {
        printf("%8.2f", percentage);
    } else {
        printChart(chartWidth, percentage, CHART_FILLER, ' ');
    }
}

void printLine(int size)
{
    for(int i=0; i<size; i++)
        printf("%s", HORIZONTAL_LIGN);
    printf("\n");
}

void printBuckets(void)
{
    int chartwidth = calculateChartwidth();
    int totalwidth = (nochart ? CHART_OFFSET + 8 : CHART_OFFSET + chartwidth + 2);

    if(!noheader) {
        printf("%-15.15s %9s %8s\n", "CATEGORY", "EXPENSE", "PERCENT");
        printLine(totalwidth);
    }

    // Print all buckets
    struct bucket* current = buckets;
    while (current != NULL) {
        // Select line color if not deactivated
        if(colorOutput) {
            if(current->totalCents > 0)
                printf(ANSI_COLOR_GREEN);
            if(current->totalCents < 0)
                printf(ANSI_COLOR_RED);
        }

        printf("%-15.15s %9.2f ", current->category, current->totalCents / 100.0);
        float percentage = abs((current->totalCents * 100.0) / positiveTotalCents);
        printChartOrPercent(chartwidth, percentage);
        printf("\n");
        current = current->nextBucket;
    }

    if(!nototal) {
        if(colorOutput)
            printf(ANSI_COLOR_RESET);
        printLine(totalwidth);

        long total = positiveTotalCents + negativeTotalCents;
        printf("%-15.15s %9.2f ", "TOTAL", total / 100.0);
        float percentage = abs(negativeTotalCents * 100.0 / positiveTotalCents);
        printChartOrPercent(chartwidth, percentage);
        printf("\n");
    }

    // Make sure to reset all color settings
    if(colorOutput)
        printf(ANSI_COLOR_RESET);
}

void calculateTotals()
{
    positiveTotalCents = 0;
    negativeTotalCents = 0;

    struct bucket* current = buckets;
    while (current != NULL) {
        if(current->totalCents >= 0)
            positiveTotalCents += current->totalCents;
        else
            negativeTotalCents += current->totalCents;
        current = current->nextBucket;
    }
}

int main(int argc, const char **argv)
{
    // Parse arguments
    Argparser* argparser = Argparser_new();
    Argparser_init(argparser, (ArgparserOption[]) {
        ARGPARSER_OPT_HELP(),
        ARGPARSER_OPT_BOOL('c', "color", &colorOutput, "display with colors"),
        ARGPARSER_OPT_BOOL('i', "inverse", &inverse, "inverse the sign of all input"),
        ARGPARSER_OPT_BOOL(0, "nochart", &nochart, "hide the chart"),
        ARGPARSER_OPT_BOOL(0, "noheader", &noheader, "hide the header"),
        ARGPARSER_OPT_BOOL(0, "nototal", &nototal, "hide the total"),
        ARGPARSER_OPT_END(),
    });
    Argparser_setUsage(argparser, "bud [--inverse] [--noheader] [--color] [--nochart] [--nototal] FILE\n");
    Argparser_setDescription(argparser, "Bud is a simple budget manager based on plain text files.\nIf no input FILE is given, it reads from STDIN.\n");
    argc = Argparser_parse(argparser, argc, argv);
    Argparser_clear(argparser);
    Argparser_delete(argparser);

    // Choose if reading from file or stdin
    FILE *input = chooseInput(argc, argv);

    // Process all lines of the file
    unsigned int lineno = 1;
    char buffer[BUFFERSIZE];
    while (fgets(buffer, BUFFERSIZE, input)) {
        processEntry(lineno, buffer);
        lineno++;
    }

    calculateTotals();
    printBuckets();
    return 0;
}
