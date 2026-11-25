#include "divoom_port_time.h"
#include "divoom_build_config.h"

#include <ctype.h>
#include <stddef.h>
#include <string.h>

static int divoom_port_parse_digits(const char **buf, int min_len, int max_len, int *out)
{
	int val = 0;
	int len = 0;
	const char *p = *buf;

	while (len < max_len && *p >= '0' && *p <= '9') {
		val = val * 10 + (*p - '0');
		p++;
		len++;
	}
	if (len < min_len)
		return -1;

	*out = val;
	*buf = p;
	return 0;
}

static void divoom_port_skip_space(const char **p)
{
	while (**p && isspace((unsigned char)**p))
		(*p)++;
}

char *divoom_port_strptime(const char *buf, const char *fmt, struct tm *tm)
{
	const char *bp;
	const char *fp;
	int c;
	int value;

	if (!buf || !fmt || !tm)
		return NULL;

	bp = buf;
	fp = fmt;
	divoom_port_skip_space(&bp);

	while ((c = *fp++) != 0) {
		if (isspace((unsigned char)c)) {
			divoom_port_skip_space(&bp);
			continue;
		}
		if (c != '%') {
			if (*bp++ != c)
				return NULL;
			continue;
		}

		c = *fp++;
		switch (c) {
		case '%':
			if (*bp++ != '%')
				return NULL;
			break;
		case 'Y':
			if (divoom_port_parse_digits(&bp, 4, 4, &value) != 0)
				return NULL;
			tm->tm_year = value - 1900;
			break;
		case 'y':
			if (divoom_port_parse_digits(&bp, 2, 2, &value) != 0)
				return NULL;
			if (value < 69)
				value += 100;
			tm->tm_year = value;
			break;
		case 'm':
			if (divoom_port_parse_digits(&bp, 1, 2, &value) != 0)
				return NULL;
			if (value < 1 || value > 12)
				return NULL;
			tm->tm_mon = value - 1;
			break;
		case 'd':
		case 'e':
			if (c == 'e' && *bp == ' ')
				bp++;
			if (divoom_port_parse_digits(&bp, 1, 2, &value) != 0)
				return NULL;
			if (value < 1 || value > 31)
				return NULL;
			tm->tm_mday = value;
			break;
		case 'H':
			if (divoom_port_parse_digits(&bp, 1, 2, &value) != 0)
				return NULL;
			if (value < 0 || value > 23)
				return NULL;
			tm->tm_hour = value;
			break;
		case 'M':
			if (divoom_port_parse_digits(&bp, 1, 2, &value) != 0)
				return NULL;
			if (value < 0 || value > 59)
				return NULL;
			tm->tm_min = value;
			break;
		case 'S':
			if (divoom_port_parse_digits(&bp, 1, 2, &value) != 0)
				return NULL;
			if (value < 0 || value > 60)
				return NULL;
			tm->tm_sec = value;
			break;
		default:
			return NULL;
		}
	}

	return (char *)bp;
}

#if DIVOOM_BUILD_RTOS_DEVICE
char *strptime(const char *s, const char *fmt, struct tm *tm)
{
	return divoom_port_strptime(s, fmt, tm);
}
#endif
