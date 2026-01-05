#include "totp.h"

static BOOL
Pow10_u32(DWORD digits, uint32_t *out_mod)
{
    if (!out_mod)
        return FALSE;
    if (digits < 1 || digits > 9)
        return FALSE;

    uint32_t m = 1;
    for (DWORD i = 0; i < digits; i++)
        m *= 10;

    *out_mod = m;
    return TRUE;
}

uint64_t
GetUnixTimeNow(void)
{
    FILETIME ft;
    ULARGE_INTEGER uli;
    GetSystemTimeAsFileTime(&ft);

    uli.LowPart = ft.dwLowDateTime;
    uli.HighPart = ft.dwHighDateTime;

    // Unix epoch: 1970-01-01
    const uint64_t EPOCH_DIFF_100NS = 11644473600ULL * 10000000ULL;
    if (uli.QuadPart < EPOCH_DIFF_100NS)
        return 0;

    uint64_t unix_100ns = uli.QuadPart - EPOCH_DIFF_100NS;
    return unix_100ns / 10000000ULL;
}

static BOOL
HmacCng(_In_ LPCWSTR alg_id,
        _In_reads_bytes_(key_len) const BYTE *key,
        DWORD key_len,
        _In_reads_bytes_(data_len) const BYTE *data,
        DWORD data_len,
        _Out_writes_bytes_to_(out_cap, *out_len) BYTE *out,
        DWORD out_cap,
        _Out_ DWORD *out_len)
{
    if (!alg_id || !key || key_len == 0 || !data || !out || !out_len)
        return FALSE;

    *out_len = 0;

    BCRYPT_ALG_HANDLE hAlg = NULL;
    BCRYPT_HASH_HANDLE hHash = NULL;

    PBYTE hashObject = NULL;
    DWORD cbHashObject = 0;
    DWORD cbData = 0;

    DWORD cbHash = 0;

    NTSTATUS st = BCryptOpenAlgorithmProvider(&hAlg, alg_id, NULL, BCRYPT_ALG_HANDLE_HMAC_FLAG);

    if (st < 0)
        goto Cleanup;

    st = BCryptGetProperty(
        hAlg,
        BCRYPT_OBJECT_LENGTH,
        (PUCHAR)&cbHashObject,
        sizeof(cbHashObject),
        &cbData,
        0);
    if (st < 0)
        goto Cleanup;

    st = BCryptGetProperty(hAlg,
                           BCRYPT_HASH_LENGTH,
                           (PUCHAR)&cbHash,
                           sizeof(cbHash),
                           &cbData,
                           0);
    if (st < 0)
        goto Cleanup;

    if (cbHash == 0 || cbHash > out_cap)
        goto Cleanup;

    hashObject = (PBYTE)HeapAlloc(GetProcessHeap(), 0, cbHashObject);
    if (!hashObject)
        goto Cleanup;

    st = BCryptCreateHash(hAlg,
                          &hHash,
                          hashObject,
                          cbHashObject,
                          (PUCHAR)key,
                          key_len,
                          0);
    if (st < 0)
        goto Cleanup;

    st = BCryptHashData(hHash, (PUCHAR)data, data_len, 0);
    if (st < 0)
        goto Cleanup;

    st = BCryptFinishHash(hHash, out, cbHash, 0);
    if (st < 0)
        goto Cleanup;

    *out_len = cbHash;

Cleanup:
    if (hHash)
        BCryptDestroyHash(hHash);
    if (hAlg)
        BCryptCloseAlgorithmProvider(hAlg, 0);

    if (hashObject)
    {
        SecureZeroMemory(hashObject, cbHashObject);
        HeapFree(GetProcessHeap(), 0, hashObject);
    }

    return (*out_len != 0);
}

BOOL
TotpGenerate(const BYTE *secret,
             DWORD secret_len,
             uint64_t unix_time,
             DWORD step_seconds,
             DWORD digits,
             TOTP_ALG alg,
             WCHAR *out,
             size_t out_cch)
{
    if (!secret || secret_len == 0 || !out || out_cch == 0)
        return FALSE;

    if (step_seconds == 0)
        return FALSE;

    if (digits < 6 || digits > 9)
        return FALSE;

    if (out_cch < (size_t)digits + 1)
        return FALSE;

    uint64_t counter = unix_time / (uint64_t)step_seconds;

    BYTE msg[8];
    msg[0] = (BYTE)((counter >> 56) & 0xFF);
    msg[1] = (BYTE)((counter >> 48) & 0xFF);
    msg[2] = (BYTE)((counter >> 40) & 0xFF);
    msg[3] = (BYTE)((counter >> 32) & 0xFF);
    msg[4] = (BYTE)((counter >> 24) & 0xFF);
    msg[5] = (BYTE)((counter >> 16) & 0xFF);
    msg[6] = (BYTE)((counter >> 8) & 0xFF);
    msg[7] = (BYTE)(counter & 0xFF);

    LPCWSTR alg_id = NULL;
    switch (alg)
    {
        case TOTP_ALG_SHA1:
            alg_id = BCRYPT_SHA1_ALGORITHM;
            break;
        default:
            return FALSE;
    }

    BYTE hmac[64];
    DWORD hmac_len = 0;

    if (!HmacCng(alg_id,
                 secret,
                 secret_len,
                 msg,
                 sizeof(msg),
                 hmac,
                 sizeof(hmac),
                 &hmac_len))
        return FALSE;

    if (hmac_len < 20) // SHA1=20
    {
        SecureZeroMemory(hmac, sizeof(hmac));
        return FALSE;
    }

    // Dynamic truncation
    BYTE offset = (BYTE)(hmac[hmac_len - 1] & 0x0F);
    if ((DWORD)offset + 4 > hmac_len)
    {
        SecureZeroMemory(hmac, sizeof(hmac));
        return FALSE;
    }

    uint32_t bin_code =
        ((uint32_t)(hmac[offset] & 0x7F) << 24) | ((uint32_t)(hmac[offset + 1] & 0xFF) << 16)
        | ((uint32_t)(hmac[offset + 2] & 0xFF) << 8) | ((uint32_t)(hmac[offset + 3] & 0xFF));

    uint32_t mod = 0;
    if (!Pow10_u32(digits, &mod) || mod == 0)
    {
        SecureZeroMemory(hmac, sizeof(hmac));
        return FALSE;
    }

    uint32_t otp = bin_code % mod;

    HRESULT hr = StringCchPrintfW(out, out_cch, L"%0*u", (int)digits, (unsigned)otp);

    SecureZeroMemory(hmac, sizeof(hmac));
    return SUCCEEDED(hr);
}

static int
Base32ValW(WCHAR c)
{
    if (c >= L'a' && c <= L'z')
        c = c - (L'a' - L'A');

    if (c >= L'A' && c <= L'Z')
        return (int)(c - L'A'); // 0..25
    if (c >= L'2' && c <= L'7')
        return (int)(26 + (c - L'2')); // 26..31
    return -1;
}

BOOL
Base32DecodeW(const WCHAR *in, BYTE *out, size_t out_cap, DWORD *out_len)
{
    if (!in || !out || !out_len)
        return FALSE;
    *out_len = 0;

    uint32_t buffer = 0;
    int bits_left = 0;
    size_t out_pos = 0;

    for (const WCHAR *p = in; *p; ++p)
    {
        WCHAR c = *p;

        if (c == L' ' || c == L'\t' || c == L'\r' || c == L'\n' || c == L'-')
            continue;

        if (c == L'=')
            break;

        int v = Base32ValW(c);
        if (v < 0)
            return FALSE;

        buffer = (buffer << 5) | (uint32_t)v;
        bits_left += 5;

        if (bits_left >= 8)
        {
            bits_left -= 8;
            BYTE b = (BYTE)((buffer >> bits_left) & 0xFF);

            if (out_pos >= out_cap)
                return FALSE;

            out[out_pos++] = b;
        }
    }

    *out_len = (DWORD)out_pos;
    return TRUE;
}

BOOL
GenerateTotpOtpFromBase32SecretW(_In_ const WCHAR *secret_b32,
                                 _Out_writes_(otp_cch) WCHAR *otp_out,
                                 size_t otp_cch)
{
    if (!secret_b32 || !*secret_b32 || !otp_out || otp_cch == 0)
        return FALSE;

    BYTE key[128] = { 0 };
    DWORD key_len = 0;

    BOOL ok = FALSE;

    if (!Base32DecodeW(secret_b32, key, sizeof(key), &key_len) || key_len == 0)
        goto Cleanup;

    uint64_t now = GetUnixTimeNow();

    ok = TotpGenerate(key,
                      key_len,
                      now,
                      30,
                      // step
                      6,
                      // digits
                      TOTP_ALG_SHA1,
                      otp_out,
                      otp_cch);

Cleanup:
    SecureZeroMemory(key, sizeof(key));
    return ok;
}