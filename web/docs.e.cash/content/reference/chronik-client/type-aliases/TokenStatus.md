[**chronik-client**](../README.md)

---

[chronik-client](../README.md) / TokenStatus

# Type Alias: TokenStatus

> **TokenStatus** = `"TOKEN_STATUS_NON_TOKEN"` \| `"TOKEN_STATUS_NORMAL"` \| `"TOKEN_STATUS_NOT_NORMAL"` \| `"TOKEN_STATUS_UNKNOWN"`

Defined in: ChronikClient.ts:2073

TokenStatus
TOKEN_STATUS_NON_TOKEN - Tx involves no tokens whatsover, i.e. neither any burns nor any failed
parsing/coloring or any tokens being created / moved.
TOKEN_STATUS_NORMAL - Tx involves tokens but no unintentional burns or failed parsings/colorings
TOKEN_STATUS_NOT_NORMAL - Tx involves tokens but contains unintentional burns or failed parsings/colorings
TOKEN_STATUS_UNKNOWN - Token tx of unknown status
