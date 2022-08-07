/* loafanim.c */
int main(int argc, char **argv);
void singleentry(char *PDATname);
void anim2loaf(void);
void emptyentry(void);
void writeLOAF(char *filename);
int getnextname(char *toptr);
int32 loadcel(ubyte *spname);
int32 loadcel(ubyte *spname);
int stepanimcel(struct memfile *mf);
uint32 getPDAT(void);
uint32 getPLUT(void);
int32 memcmp(void *this, void *that, int32 siz);
void usage(void);
void terminate(int i);
void *loadfile(char *filename, int32 *sizptr);
