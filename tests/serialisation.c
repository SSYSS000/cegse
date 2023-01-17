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

/* Serialise and deserialise unsigned and signed integers and a float. */
static int sd_numbers(void)
{
	struct numbers {
		uint32_t _u8;
		uint32_t _u16;
		uint32_t _u32;
		u64 _u64;
		i8 _i8;
		int32_t _i16;
		int32_t _i32;
		i64 _i64;
		float _f32;
	} __attribute__((packed)) /* important, because memcmp is used */
	result, original = {
		134u,
		24584u,
		151019528u,
		0x2040002009006008u,
		-53,
		6392,
		537944072,
		-0x400040020106008,
		39566.52f
	};

	reset();
	serialise_u8 (original._u8, &ts);
	serialise_u16(original._u16, &ts);
	serialise_u32(original._u32, &ts);
	serialise_u64(original._u64, &ts);
	serialise_i8 (original._i8, &ts);
	serialise_i16(original._i16, &ts);
	serialise_i32(original._i32, &ts);
	serialise_i64(original._i64, &ts);
	serialise_f32(original._f32, &ts);
	parse_u8 (&result._u8, &tp);
	parse_u16(&result._u16, &tp);
	parse_u32(&result._u32, &tp);
	parse_u64(&result._u64, &tp);
	parse_i8 (&result._i8, &tp);
	parse_i16(&result._i16, &tp);
	parse_i32(&result._i32, &tp);
	parse_i64(&result._i64, &tp);
	parse_f32(&result._f32, &tp);

	if (tp.eod) {
		ptest_error("unexpected end-of-data\n");
		return TEST_FAILURE;
	}

	if (memcmp(&original, &result, sizeof(original)) != 0) {
		ptest_error(
		"numbers do not match.\n"
		"zero value means something's broke:\n"
		" u8: %d\n"
		"u16: %d\n"
		"uint32_t: %d\n"
		"u64: %d\n"
		" i8: %d\n"
		"i16: %d\n"
		"int32_t: %d\n"
		"i64: %d\n"
		"float: %d\n",
		original._u8 == result._u8,
		original._u16 == result._u16,
		original._u32 == result._u32,
		original._u64 == result._u64,
		original._i8 == result._i8,
		original._i16 == result._i16,
		original._i32 == result._i32,
		original._i64 == result._i64,
		original._f32 == result._f32);
		return TEST_FAILURE;
	}

	return TEST_SUCCESS;
}
REGISTER_TEST(sd_numbers);

/* Serialise and deserialise some vsvals of different sizes. */
static int sd_vsval(void)
{
	uint32_t i, expected, result;

	reset();
	for (i = 0u; i < 8u; ++i) {
		expected = 12345u*i*i*i;

		 /* 'expected' should wrap if greater than VSVAL_MAX */
		serialise_vsval(expected, &ts);
		if (parse_vsval(&result, &tp) == -1) {
			ptest_error("unexpected end-of-data\n");
			return TEST_FAILURE;
		}

		expected &= 0x3FFFFF;
		if (result != expected) {
			ptest_error("vsval mismatch. expected %u, got %u.\n",
				(unsigned)expected, (unsigned)result);
			return TEST_FAILURE;
		}
	}

	return TEST_SUCCESS;
}
REGISTER_TEST(sd_vsval);

/* Serialise and deserialise a reference id. */
static int sd_ref_id(void)
{
	uint32_t result, expected = REF_REGULAR(0x000BA0CB);

	reset();
	serialise_ref_id(expected, &ts);
	if (parse_ref_id(&result, &tp) == -1) {
		ptest_error("unexpected end-of-data\n");
		return TEST_FAILURE;
	}

	if (result != expected) {
		ptest_error("reference id mismatch. expected 0x%x, got 0x%x.\n",
			(unsigned)expected, (unsigned)result);
		return TEST_FAILURE;
	}

	return TEST_SUCCESS;
}
REGISTER_TEST(sd_ref_id);

/* Serialise and deserialise a bstring. */
static int sd_bstr(void)
{
	char original[] = "Hello, ääö.";
	char result[sizeof(original)];

	reset();
	serialise_bstr(original, &ts);
	if (parse_bstr(result, sizeof(result), &tp) == -1) {
		ptest_error("unexpected end-of-data\n");
		return TEST_FAILURE;
	}

	if (strncmp(original, result, strlen(original)) != 0) {
		ptest_error(
			"strings do not match.\n"
			"expected: %s\n"
			"got: %s\n", original, result);
		return TEST_FAILURE;
	}

	return TEST_SUCCESS;
}
REGISTER_TEST(sd_bstr);
