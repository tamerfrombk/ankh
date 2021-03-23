#pragma once

#include <stddef.h>

void split_by_whitespace(const char *str, char ***words_buffer_out, size_t *num_words_out);
