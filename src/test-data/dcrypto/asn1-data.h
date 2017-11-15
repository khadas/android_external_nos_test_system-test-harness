#ifndef ASN1_DATA_H
#define ASN1_DATA_H

/*
 * ASN.1 test data.
 */

typedef struct {
  uint8_t tag[8];
  size_t tag_len;
  uint8_t tag_class;
  bool constructed;
  uint32_t tag_number;
  size_t remaining;
} asn1_tag_data;


asn1_tag_data ASN1_VALID_TAGS[] = {
  {
    // Explicit low-tag:
    //   [1] {
    //   }
    .tag = {0xa1, 0x00},
    .tag_len = 2,
    .tag_class = 0x80,
    .constructed = true,
    .tag_number = 1,
    .remaining = 1,
  },
  {
    // Explicit low-tag:
    //   [30] {
    //   }
    .tag = {0xbe, 0x00},
    .tag_len = 2,
    .tag_class = 0x80,
    .constructed = true,
    .tag_number = 30,
    .remaining = 1,
  },
  {
    // Explicit high-tag:
    //   [31] {
    //   }
    .tag = {0xbf, 0x1f, 0x00},
    .tag_len = 3,
    .tag_class = 0x80,
    .constructed = true,
    .tag_number = 31,
    .remaining = 1,
  },
  {
    // Explicit high-tag:
    //   [1 << 29] {
    //   }
    .tag = {0xbf, 0x82, 0x80, 0x80, 0x80, 0x00, 0x00},
    .tag_len = 7,
    .tag_class = 0x80,
    .constructed = true,
    .tag_number = (1 << 29),
    .remaining = 1,
  },
  {
    // Explicit high-tag:
    //   [(UINT32_MAX] {
    //   }
    .tag = {0xbf, 0x8f, 0xff, 0xff, 0xff, 0x7f, 0x00},
    .tag_len = 7,
    .tag_class = 0x80,
    .constructed = true,
    .tag_number = (0xFFFFFFFF),
    .remaining = 1,
  },
  {
    // Empty sequence:
    //   SEQUENCE {
    //   }
    .tag = {0x30, 0x00},
    .tag_len = 2,
    .tag_class = 0,
    .constructed = true,
    .tag_number = 0x10,
    .remaining = 1,
  },
  {
    // Empty integer:
    //   INTEGER {
    //   }
    .tag = {0x02, 0x00},
    .tag_len = 2,
    .tag_class = 0,
    .constructed = false,
    .tag_number = 0x02,
    .remaining = 1,
  },
  {
    // Empty integer, minus length byte:
    //   INTEGER {
    //   }
    .tag = {0x02},
    .tag_len = 1,
    .tag_class = 0,
    .constructed = false,
    .tag_number = 0x02,
    .remaining = 0,  // All bytes consumed.
  },
};

asn1_tag_data ASN1_INVALID_TAGS[] = {
  {
    // NULL tag!
    .tag = {},
    .tag_len = 0,
    .tag_class = 0,
    .constructed = 0,
    .tag_number = 0,
    .remaining = 0,
  },
  {
    // Zero tag!
    .tag = {0x00},
    .tag_len = 1,
    .tag_class = 0,
    .constructed = 0,
    .tag_number = 0,
    .remaining = 0,
  },
  {
    // Explicit high-tag, missing bytes[1:]:
    //   [31] {
    //   }
    .tag = {0xbf},
    .tag_len = 1,
    .tag_class = 0,
    .constructed = 0,
    .tag_number = 0,
    .remaining = 0,
  },
  {
    // Explicit high-tag, non-minimally encoded:
    //   [31] {
    //   }
    .tag = {0xbf, 0x80, 0x1f},
    .tag_len = 3,
    .tag_class = 0,
    .constructed = 0,
    .tag_number = 0,
    .remaining = 0,
  },
  {
    // Explicit high-tag, non-minimally encoded:
    //   [31] {
    //   }
    .tag = {0xbf, 0x80, 0x1f},
    .tag_len = 3,
    .tag_class = 0,
    .constructed = 0,
    .tag_number = 0,
    .remaining = 0,
  },
  {
    // Explicit high-tag, over-flows:
    //   [UINT32_MAX + 1] {
    //   }
    .tag = {0xbf, 0x8f, 0xff, 0xff, 0xff, 0x80, 0x00, 0x00},
    .tag_len = 8,
    .tag_class = 0,
    .constructed = 0,
    .tag_number = 0,
    .remaining = 0,
  },
  {
    // Explicit high-tag, truncated input:
    //   [UINT32_MAX - 0x7f] {
    //   }
    .tag = {0xbf, 0x8f, 0xff, 0xff, 0xff},
    .tag_len = 5,
    .tag_class = 0,
    .constructed = 0,
    .tag_number = 0,
    .remaining = 0,
  },
  {
    // Explicit high-tag, non-minimally encoded:
    //   [30] {
    //   }
    .tag = {0xbf, 0x1e, 0x00},
    .tag_len = 3,
    .tag_class = 0,
    .constructed = 0,
    .tag_number = 0,
    .remaining = 0,
  }
};


#endif  // ASN1_DATA_H
