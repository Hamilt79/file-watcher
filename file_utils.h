#ifndef file_utils_h
#define file_utils_h
#include <stdbool.h>

bool DoesFileExist(char *fileName);

unsigned long GetLinesFromFile(char *fileName, char*** lines_Out, char **fileText_Out);

void FreeLines(char ***lines, char **fileText);

#endif