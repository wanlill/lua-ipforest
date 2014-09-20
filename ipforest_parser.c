#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "ipforest_types.h"
#include <string.h>

inline static const char *
ipforest_index(const char *str, size_t len, char c)
{
    int i;

    for (i = 0; i < len; i++) {
        if (str[i] == c) {
            return &str[i];
        }
    }

    return NULL;
}

inline static int
ipforest_strtol(const char *str, size_t len, ssize_t *plen)
{
    int i, ret;

    *plen = 0;
    ret = 0;

    for (i = 0; i < len; i++) {
        if (str[i] <= '9' && str[i] >= '0') {
            ret = ret * 10 + (str[i] - '0');
            *plen += 1;
        } else {
            *plen = -1;
            return 0;
        }
    }

    return ret;
}

inline static IPFOREST_BOOLEAN
ipforest_atouint8(const char *ip, size_t len, uint8_t *pret)
{
    ssize_t parsed_len;
    int num;

    num = ipforest_strtol(ip, len, &parsed_len);

    if (parsed_len < 0 || parsed_len != len) {
        /* not fully parsed, error */
        return IPFOREST_FALSE;
    }

    if (num >= 0 &&  num < 256) {
        *pret = (uint8_t)num;
        return IPFOREST_TRUE;
    }

    return IPFOREST_FALSE;
}


IPFOREST_BOOLEAN
ipforest_atohl(const char *ip, size_t len, uint32_t *addr)
{
    int i, m;
    uint8_t byte;
    const char *cur, *p;

    *addr = 0;
    p = NULL;
    cur = ip;
    i = 3;
    m = 0;
    

    /* deal with "192.168.1.10/24" issue */
    if (!ipforest_index(cur, len, '.')) {
        if (ipforest_atouint8(cur, len, &byte)) {
            if (byte <= 32) {
                m = 31;
                while (byte > 0) {
                    *addr += 1 << m;
                    m--;
                    byte--;
                }
                return IPFOREST_TRUE;
            }
        }
        return IPFOREST_FALSE;
    }

    do {
        p = ipforest_index(cur, len - (ip - cur), '.');
        if (!p) {
            return IPFOREST_FALSE;
        }

        if (!ipforest_atouint8(cur, p - cur, &byte)) {
            return IPFOREST_FALSE;
        }

        *addr += byte << (i * 8);

        cur = p + 1;
        i--;
    } while (i > 0);

    if (!ipforest_atouint8(cur, len - (cur - ip), &byte)) {
        return IPFOREST_FALSE;
    }

    *addr += byte;

    return IPFOREST_TRUE;
}

inline static void
_split_ip_range(uint32_t low, uint32_t high, int len, int *count, ipforest_ipaddr_t *addrs)
{
    int i, j, m;
    int low_left_one;
    int high_left_one;


    low_left_one = -1;
    high_left_one = -1;

    /* printf("low: 0x%0x, high: 0x%0x, len: %d\n", low, high, len); */

    /* reverse order not permitted */
    if (low > high) {
        *count = -1;
        return;
    }

    if (len == 0) {
        /* all same, return it */
        if (addrs) {
            addrs[*count].addr = low;
            addrs[*count].mask = 0xffffffff;
            /* printf("ip: 0x%08x, mask:0x%08x\n", addrs[*count].addr, addrs[*count].mask); */
        }
        *count += 1;
        return;
    }

    /* deal with first time, may equal with len be initial value */
    if (low == high) {
        if (addrs) {
            addrs[*count].addr = low;
            addrs[*count].mask = 0xffffffff;
            /* printf("ip: 0x%08x, mask:0x%08x\n", addrs[*count].addr, addrs[*count].mask); */
        }
        *count += 1;
        return;
    }

    m = 1 << (len - 1);
    while (len > 0) {
        if ((low & m) != (high & m))
        {
            break;
        }
        len--;
        m = m >> 1;
    }

    /* m = 1; */
    /* for (i = 0; i < len; i++) { */
    /*     if (m & low) { */
    /*         low_left_one = i; */
    /*     } */

    /*     if (m & high) { */
    /*         high_left_one = i; */
    /*     } */

    /*     m = m << 1; */
    /* } */

    /* first diff tells high_left_one */
    high_left_one = len - 1;
    for (i = high_left_one - 1; i >=0; i--) {
        if ((1 << i) & low) {
            low_left_one = i;
            break;
        }
    }

    if (high_left_one - low_left_one > 1) {
        for (j = low_left_one + 1; j < high_left_one; j++) {
            if (addrs) {
                addrs[*count].addr = (IPFOREST_MASK_UP(len) & low) | (1 << j);
                addrs[*count].mask = IPFOREST_MASK_UP(j);
                /* printf("range ip: 0x%08x, mask:0x%08x, addr:0x%08x\n", addrs[*count].addr, addrs[*count].mask, IPFOREST_MASK_UP(len) & low); */
            }
            *count += 1;
        }
    }

    
    if (low_left_one >= 0) {
        _split_ip_range(low,
                        (low & ~IPFOREST_MASK_DOWN(low_left_one)) | IPFOREST_MASK_DOWN(low_left_one),
                        low_left_one, /* len dec by one */
                        count,
                        addrs);
    } else {
        if (addrs) {
            addrs[*count].addr = low;
            addrs[*count].mask = 0xffffffff;
            /* printf("ip: 0x%08x, mask:0x%08x\n", addrs[*count].addr, addrs[*count].mask); */
        }
        *count += 1;
    }

    _split_ip_range(high & IPFOREST_MASK_UP(high_left_one),
                    high,
                    high_left_one, /* len dec by one */
                    count,
                    addrs);
}

inline static int
_parse_ip_range(const char *line, ipforest_ipaddr_t *addrs)
{
    const char *p;
    uint32_t high;
    uint32_t low;
    int ret;
    size_t len;

    ret = 0;
    p = NULL;

    p = ipforest_index(line, strlen(line), '-');
    assert(p);

    if (ipforest_atohl(line, p - line, &low)) {
        len = strlen(line) - (p - line + 1);
        if (ipforest_index(p + 1, len, '.')) {
            if (ipforest_atohl(p + 1, len, &high)) {
                _split_ip_range(low, high, 32, &ret, addrs);
                return ret;
            }
        } else {
            high = low;
            if (ipforest_atouint8(p + 1, len, (uint8_t *)&high)) {
                _split_ip_range(low, high, 32, &ret, addrs);
                return ret;
            }
        }
    }

    return -1;
}

inline static int
_parse_ip_cidr(const char *line, ipforest_ipaddr_t *addrs)
{
    const char *p;
    uint32_t addr;
    uint32_t mask;

    p = NULL;

    p = ipforest_index(line, strlen(line), '/');
    assert(p);

    if (ipforest_atohl(line, p - line, &addr)) {
        if (ipforest_atohl(p + 1,
                           strlen(line) - (p - line + 1), &mask)) {
            if (addrs) {
                addrs->addr = addr;
                addrs->mask = mask;
            }
            return 1;
        }
    }

    return -1;
}

inline static int
_parse_ip_addr(const char *line, ipforest_ipaddr_t *addrs)
{
    char buf[IPFOREST_IPSTR_BUF_LEN];
    if (strlen(line) > IPFOREST_IPSTR_MAX_LEN) {
        return -1;
    }

    strncpy(buf, line, IPFOREST_IPSTR_BUF_LEN);
    strncat(buf, "/32", 3);

    return _parse_ip_cidr(buf, addrs);
}

int
ipforest_parse_ip_line(const char *line, ipforest_ipaddr_t *addrs)
{
    const char *p;

    p = NULL;

    /* deal with ip range */
    p = ipforest_index(line, strlen(line), '-');
    if (p) {
        return _parse_ip_range(line, addrs);
    }

    /* deal with cidr */
    p = ipforest_index(line, strlen(line), '/');
    if (p) {
        return _parse_ip_cidr(line, addrs);
    }
    
    return _parse_ip_addr(line, addrs);
}
