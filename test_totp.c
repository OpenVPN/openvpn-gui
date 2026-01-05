#include <windows.h>
#include <stdint.h>
#include <stdio.h>
#include <wchar.h>

#include "totp.h"

typedef struct
{
    uint64_t unix_time;
    DWORD digits;
    TOTP_ALG alg;
    const WCHAR *expected;
} totp_case_t;

static int failures = 0;

static void expect_true(BOOL ok, const WCHAR *testname)
{
    if (!ok)
    {
        fwprintf(stderr, L"[FAIL] %s: function returned FALSE\n", testname);
        failures++;
    }
}

static void expect_wstr_eq(const WCHAR *got, const WCHAR *exp, const WCHAR *testname)
{
    if (wcscmp(got, exp) != 0)
    {
        fwprintf(stderr, L"[FAIL] %s: expected '%s', got '%s'\n", testname, exp, got);
        failures++;
    }
}

static void run_rfc6238_sha1_vectors(void)
{
    // RFC 6238 Appendix B:
    // secret = ASCII "12345678901234567890"
    // step = 30, digits = 8
    // times: 59, 1111111109, 1111111111, 1234567890, 2000000000, 20000000000
    // expected (SHA1): 94287082, 07081804, 14050471, 89005924, 69279037, 65353130
    // см. таблицу в RFC 6238 :contentReference[oaicite:2]{index=2}

    static const BYTE secret[] = "12345678901234567890";
    const DWORD secret_len = (DWORD)(sizeof(secret) - 1);

    static const totp_case_t cases[] = {
        { 59ULL,         8, TOTP_ALG_SHA1, L"94287082" },
        { 1111111109ULL, 8, TOTP_ALG_SHA1, L"07081804" },
        { 1111111111ULL, 8, TOTP_ALG_SHA1, L"14050471" },
        { 1234567890ULL, 8, TOTP_ALG_SHA1, L"89005924" },
        { 2000000000ULL, 8, TOTP_ALG_SHA1, L"69279037" },
        { 20000000000ULL,8, TOTP_ALG_SHA1, L"65353130" },
    };

    for (size_t i = 0; i < _countof(cases); i++)
    {
        WCHAR out[32] = L"";
        WCHAR testname[128];

        swprintf(testname, _countof(testname),
                 L"RFC6238/SHA1 t=%llu", (unsigned long long)cases[i].unix_time);

        BOOL ok = TotpGenerate(secret,
                               secret_len,
                               cases[i].unix_time,
                               30,                 // step_seconds
                               cases[i].digits,    // digits (8 per RFC)
                               cases[i].alg,
                               out,
                               _countof(out));

        expect_true(ok, testname);
        if (ok)
        {
            expect_wstr_eq(out, cases[i].expected, testname);
        }

        SecureZeroMemory(out, sizeof(out));
    }
}

static void run_negative_tests(void)
{
    static const BYTE secret[] = "12345678901234567890";
    WCHAR out[8] = L"";

    {
        BOOL ok = TotpGenerate(secret, (DWORD)(sizeof(secret) - 1),
                               59, 30, 8, TOTP_ALG_SHA1,
                               out, 8 /* недостаточно для 8 цифр + '\0' */);
        if (ok)
        {
            fwprintf(stderr, L"[FAIL] negative: expected FALSE for too small out buffer\n");
            failures++;
        }
    }

    // step_seconds = 0 -> FALSE
    {
        BOOL ok = TotpGenerate(secret, (DWORD)(sizeof(secret) - 1),
                               59, 0, 8, TOTP_ALG_SHA1,
                               out, _countof(out));
        if (ok)
        {
            fwprintf(stderr, L"[FAIL] negative: expected FALSE for step_seconds=0\n");
            failures++;
        }
    }

    SecureZeroMemory(out, sizeof(out));
}

int wmain(void)
{
    run_rfc6238_sha1_vectors();
    run_negative_tests();

    if (failures == 0)
    {
        wprintf(L"[PASS] All TOTP tests passed.\n");
        return 0;
    }
    else
    {
        wprintf(L"[FAIL] %d test(s) failed.\n", failures);
        return 1;
    }
}
