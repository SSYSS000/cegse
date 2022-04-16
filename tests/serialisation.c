#include "tests.h"
#include "../serialisation.c"

char buffer[32768];
struct file_context tctx = {{0}, {0}, 78};
struct serialiser ts = {&tctx, buffer, 0};
struct parser tp = {&tctx, buffer, sizeof(buffer), 0};

static void reset(void)
{
	memset(&tp, 0, sizeof(tp));
	memset(&ts, 0, sizeof(ts));
	tp.ctx = ts.ctx = &tctx;
	tp.buf = ts.buf = buffer;
	tp.buf_sz = sizeof(buffer);
}

/* Serialise and deserialise a bstring. */
static int sd_bstr(void)
{
	char original[] = "Hello, ääö.";
	char result[sizeof(original)];

	serialise_bstr(original, &ts);
	if (parse_bstr(result, sizeof(result), &tp) == -1) {
		ptest_error("unexpected end-of-data\n");
		goto out_failure;
	}

	if (strncmp(original, result, strlen(original)) != 0) {
		ptest_error(
			"strings do not match.\n"
			"expected: %s\n"
			"got: %s\n", original, result);
		goto out_failure;
	}

	reset();
	return TEST_SUCCESS;
out_failure:
	reset();
	return TEST_FAILURE;
}
REGISTER_TEST(sd_bstr);
