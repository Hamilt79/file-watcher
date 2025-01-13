#ifndef FILE_UTILS_H_
#define FILE_UTILS_H_
#include <stdbool.h>

/**
* Check if file exists.
*
*   \param[in] numbers: a pointer to an array of unsigned integers
*     
*   \returns true(1) if file exists, false(0) if not
**/
bool does_file_exist(char *fileName);

/**
* Used to break up the text in a file into an array of strings.
* Also removes new line characters if they exist.
*
*   \param[in] pathToFile: path of file
*   \param[out] lines_Out: char array to put lines into
*   \param[out] fileText_Out: the backing string that the lines are composed of
*     
*   \returns size of \param[out] lines_Out array.
**/
size_t get_lines_from_file(char *pathToFile, char*** lines_Out, char **fileText_Out);

/**
* Frees the memory given and sets the pointers NULL.
*
*   \param[in] lines: address of string array.
*   \param[in] fileText: address of file's text string
**/
void free_lines(char ***lines, char **fileText);

#endif // FILE_UTILS_H_