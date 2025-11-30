#pragma once

#include <stddef.h>

/* Start editing the given file. This clears the screen, loads any existing
 * contents into a buffer, and displays them. ESC will save + exit.
 */
void editor_start(const char* filename);

/* Handle a single key press while the editor is active. */
void editor_handle_key(char c);

/* Returns non-zero if the editor is currently active. */
int editor_is_active(void);
