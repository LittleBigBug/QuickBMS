#ifndef HUFFMAN_H
#define HUFFMAN_H

class DecompressingCopier
{
private:
    static int tree[512];

    // Byte variable is stored here
    int value;
    // Mask for current bit
    int mask;
    // Current bit position 0..7
    int bit_num;
    // Current position in the tree
    int treepos;

public:
    DecompressingCopier();

    bool Decode (char * dest, const char * src, int & dest_size, int & src_size);
};

#endif
