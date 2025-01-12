#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "file_utils.h"
#include "stdlib.h"

unsigned long GetLinesFromFile(char *fileName, char ***lines_Out, char **fileText_Out)
{
    if (access(fileName, R_OK) != 0)
    {
        goto emptyReturn;
    }
    FILE *file = fopen(fileName, "r");
    unsigned long bytes;
    unsigned i, count = 0;
    char *token;
    unsigned long strLen = 0;
    if (file == NULL)
    {
        goto emptyReturn;
    }
    fseek(file, 0, SEEK_END);
    bytes = ftell(file);
    if (bytes == 0ul)
    {
    emptyReturn:
        (*lines_Out) = NULL;
        (*fileText_Out) = NULL;
        return 0;
    }
    fseek(file, 0, SEEK_SET);
    (*fileText_Out) = malloc(bytes + 1);
    fread(*fileText_Out, bytes, 1UL, file);
    (*fileText_Out)[bytes] = '\0';
    fclose(file);
    char temp[bytes + 1];
    strcpy(temp, *fileText_Out);
    token = strtok(temp, "\n");
    while (token != NULL)
    {
        token = strtok(NULL, "\n");
        count++;
    }
    (*lines_Out) = malloc(sizeof(char *) * count);
    token = strtok(*fileText_Out, "\n");
    i = 0;
    while (token != NULL)
    {
        (*lines_Out)[i] = token;
        strLen = strlen(token);
        if (strLen > 0ul && token[strLen - 1] == '\r')
        {
            token[strLen - 1] = '\0';
        }
        token = strtok(NULL, "\n");
        i++;
    }
    return count;
}

void FreeLines(char ***lines, char **fileText)
{
    free(*lines);
    *lines = NULL;
    free(*fileText);
    *fileText = NULL;
}