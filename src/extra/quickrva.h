typedef struct {
    unsigned char       *filemem;
    int                 filememsz;  // useless
    void                *section;
    int                 sections;
    unsigned int        sec_align;
    unsigned long long  imagebase;
    unsigned long long  entrypoint;
    int                 asmsz;      // useless
} quickrva_t;



unsigned long long rva2file(quickrva_t *q, unsigned long long va);
unsigned long long file2rva(quickrva_t *q, unsigned long long file);
int quickrva(quickrva_t *q, unsigned char *data, int size);

