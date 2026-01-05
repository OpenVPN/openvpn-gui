#pragma once

#include <Windows.h>
#include <bcrypt.h>
#include <stdint.h>
#include <strsafe.h>

typedef enum TOTP_ALG
{
    TOTP_ALG_SHA1 = 1,
} TOTP_ALG;

uint64_t GetUnixTimeNow(void);

BOOL TotpGenerate(const BYTE *secret,
                  DWORD secret_len,
                  uint64_t unix_time,
                  DWORD step_seconds,
                  DWORD digits,
                  TOTP_ALG alg,
                  WCHAR *out,
                  size_t out_cch);

BOOL Base32DecodeW(const WCHAR *in, BYTE *out, size_t out_cap, DWORD *out_len);

BOOL GenerateTotpOtpFromBase32SecretW(_In_ const WCHAR *secret_b32,
                                      _Out_writes_(otp_cch) WCHAR *otp_out,
                                      size_t otp_cch);