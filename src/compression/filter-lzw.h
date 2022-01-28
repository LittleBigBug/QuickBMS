#define LZW_LITTLE_ENDIAN     0x00 ///< Read bytes in little-endian order
#define LZW_BIG_ENDIAN        0x01 ///< Read bytes in big-endian order
#define LZW_RESET_FULL_DICT   0x02 ///< Should the dict be wiped when it's full?
#define LZW_NO_BITSIZE_RESET  0x04 ///< Leave the codeword bit length unchanged after dictionary reset
#define LZW_EOF_PARAM_VALID   0x08 ///< Is the EOF parameter (to c'tor) valid?
#define LZW_RESET_PARAM_VALID 0x10 ///< Is the reset parameter (to c'tor) valid?
#define LZW_FLUSH_ON_RESET    0x20 ///< Jump to next word boundary on reset

#ifdef __cplusplus
extern "C" {
#endif

int camoto_lzw_decompress(
    int initialBits, int maxBits, int firstCode, int eofCode, int resetCode, int flags,
    uint8_t *in, int insz, uint8_t *out, int outsz
);

#ifdef __cplusplus
}
#endif
