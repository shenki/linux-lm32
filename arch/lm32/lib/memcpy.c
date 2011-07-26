#include <linux/types.h>

void *memcpy_(void *to, const void *from, size_t n)
{
	const unsigned char *c_from = from;
	unsigned char *c_to = to;

	while (n--)
		*c_to++ = *c_from++;
	return((void *) to);
}

void *memcpy(void *to, const void *from, size_t n)
{
	unsigned long *l_to;
	unsigned long *l_from;
	const unsigned char *c_from = from;
	unsigned char *c_to = to;
	unsigned long to_align;
	unsigned long from_align;

	to_align = (unsigned long)to & 3;
	from_align = (unsigned long)from & 3;

	if (to_align == from_align && n >= 8 * 4) {
		n -= to_align;
		while (to_align--)
			*c_to++ = *c_from++;

		l_to = (unsigned long *)c_to;
		l_from = (unsigned long *)c_from;

		while (n >= 8 * 4) {
			l_to[0] = l_from[0];
			l_to[1] = l_from[1];
			l_to[2] = l_from[2];
			l_to[3] = l_from[3];
			l_to[4] = l_from[4];
			l_to[5] = l_from[5];
			l_to[6] = l_from[6];
			l_to[7] = l_from[7];
			l_to += 8;
			l_from += 8;
			n -= 8 * 4;
		}
		c_to = (unsigned char *)l_to;
		c_from = (unsigned char *)l_from;
	}

	while (n--)
		*c_to++ = *c_from++;
	return((void *) to);
}
