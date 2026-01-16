#ifndef UTILS_H
#define UTILS_H

#include <linux/string.h>

#define streq(a, b)	(strcmp((a), (b)) == 0)
#define strstarts(s, prefix)	(strncmp((s), (prefix), strlen(prefix)) == 0)
#define strprefixeq(a, n, b)	(strlen(b) == (n) && (memcmp(a, b, n) == 0))
static inline bool strends(const char *str, const char *suffix)
{
	unsigned int len, suffix_len;

	len = strlen(str);
	suffix_len = strlen(suffix);
	if (len < suffix_len)
		return false;
	return streq(str + len - suffix_len, suffix);
}

#endif