#ifndef LEVENSHTEIN_H
#define LEVENSHTEIN_H

#include "../game/q_shared.h"

// `levenshtein.h` - levenshtein
// MIT licensed.
// Copyright (c) 2015 Titus Wormer <tituswormer@gmail.com>

// Returns a size_t, depicting the difference between `a` and `b`.
// See <https://en.wikipedia.org/wiki/Levenshtein_distance> for more information.
#ifdef __cplusplus
extern "C" {
#endif

size_t
levenshtein(const char *a, const char *b);

size_t
levenshtein_n (const char *a, const size_t length, const char *b, const size_t bLength);

#ifdef __cplusplus
}
#endif

#endif // LEVENSHTEIN_H
