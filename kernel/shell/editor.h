#pragma once

#include <stddef.h>

/* Start editing the given file. This clears the screen, loads any existing
 * contents into a buffer, and displays them. ESC will save + exit.
 */
void editor_start(const char* filename);
void editor_handle_key(char c);
int editor_is_active(void);
