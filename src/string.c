#include <string.h>
#include <ctype.h>
#include <stdlib.h>

#include <clean/string.h>
#include <clean/log.h>

void split_by_whitespace(const char *str, char ***words_buffer_out, size_t *num_words_out)
{
    int words_buffer_capacity = 10;

    char **words = calloc(words_buffer_capacity, sizeof(*words));
    ASSERT_NOT_NULL(words);

    int num_words = 0;
    for (char c = *str; c != '\0'; c = *str) {
        if (isspace(c)) {
            ++str;
            continue;
        }

        if (num_words >= words_buffer_capacity) {
            debug("more than %d arguments supplied to the command line... resizing\n", words_buffer_capacity);
            
            const size_t new_capacity = words_buffer_capacity * 1.5;
            char **new_buf = realloc(words, sizeof(*words) * new_capacity);
            ASSERT_NOT_NULL(new_buf);

            words = new_buf;
            words_buffer_capacity = new_capacity;
        }

        // find the end of the current word
        int len = 0;
        for (const char *eow = str; *eow != '\0' && !isspace(*eow); ++eow) {
            ++len;
        }

        char *token = strndup(str, len);
        ASSERT_NOT_NULL(token);

        str += len;

        words[num_words++] = token;
    }

    *words_buffer_out = words;
    *num_words_out = num_words;
}
