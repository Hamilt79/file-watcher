#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "file_utils.h"
#include "stdlib.h"

bool does_file_exist(char *fileName)
{
    return (access(fileName, R_OK) == 0);
}

size_t get_lines_from_file(char *pathToFile, char ***lines_Out, char **fileText_Out)
{
    if (!does_file_exist(pathToFile))
    {
        goto emptyReturn;
    }
    FILE *file = fopen(pathToFile, "r");
    size_t bytes;
    unsigned i, count = 0;
    char *token;
    size_t strLen = 0;
    if (file == NULL)
    {
        goto emptyReturn;
    }
    // Go to end of file
    fseek(file, 0, SEEK_END);
    // Get the bytes in file
    bytes = ftell(file);
    if (bytes == 0ul)
    {
        fclose(file);
    emptyReturn:
        (*lines_Out) = NULL;
        (*fileText_Out) = NULL;
        return 0;
    }
    // Go back to beginning of file
    fseek(file, 0, SEEK_SET);
    // Allocate bytes for file text and null terminator
    (*fileText_Out) = malloc(bytes + 1);
    fread(*fileText_Out, bytes, 1UL, file);
    // Add in null terminator
    (*fileText_Out)[bytes] = '\0';
    // Close file
    fclose(file);
    // Makes temp string to tokenize
    char temp[bytes + 1];
    // Copies file text into temp
    strcpy(temp, *fileText_Out);
    token = strtok(temp, "\n");
    // Getting count
    while (token != NULL)
    {
        token = strtok(NULL, "\n");
        count++;
    }
    // Allocating memory for an array of size count
    (*lines_Out) = malloc(sizeof(char *) * count);
    token = strtok(*fileText_Out, "\n");
    i = 0;
    while (token != NULL)
    {
        (*lines_Out)[i] = token;
        strLen = strlen(token);
        // Removing ending new line
        if (strLen > 0ul && token[strLen - 1] == '\r')
        {
            token[strLen - 1] = '\0';
        }
        token = strtok(NULL, "\n");
        i++;
    }
    return count;
}

void free_lines(char ***lines, char **fileText)
{
    free(*lines);
    *lines = NULL;
    free(*fileText);
    *fileText = NULL;
}