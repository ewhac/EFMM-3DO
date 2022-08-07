/* ctst.c */
int main(int ac, char **av);
int gamemain(void);
int levelmain(void);
void moveplayer(struct Vertex *ppos, frac16 *vang);
void fullstop(void);
void recoil(void);
int moveposition(struct Vertex *ppos, struct Vector *trans, struct Object *ignoreob, int checkobs, int isplayer);
int checkcontact(struct PathBox *pb, struct BBox *bb, int block);
void blockpath(struct PathBox *pb, struct BBox *obst);
void genpathbox(struct BBox *pathbox, struct BBox *fred, struct BBox *barney);
void regenpath(struct PathBox *pathbox);
void extractcels(frac16 x, frac16 z, frac16 dir);
void clearvertsused(void);
void applyyaw(struct Matrix *mat, struct Matrix *newmat, frac16 angle);
void newmat(struct Matrix *mat);
void translatemany(struct Vector *transvec, struct Vertex *verts, int32 n);
void copyverts(struct Vertex *src, struct Vertex *dest, int32 n);
void parseargs(int ac, char **av);
void SetRast(struct RastPort *rp, int32 val);
void CopyRast(struct RastPort *src, struct RastPort *dest);
int clearscreen(struct RastPort *rp);
void installclut(struct RastPort *rp);
void fadetolevel(struct RastPort *rp, frac16 level);
void fadetoblank(struct RastPort *rp, int32 frames);
void fadeout(struct RastPort *rp, int32 frames);
void fadeup(struct RastPort *rp, int32 frames);
void *malloctype(int32 size, uint32 memtype);
void freetype(void *ptr);
void loadlevelsequence(void);
void loadwalls(void);
void unloadwalls(void);
void openlevelstuff(void);
void closelevelstuff(void);
void opengamestuff(void);
void closegamestuff(void);
void openstuff(void);
void closestuff(void);
void die(char *str);
/* rend.c */
void rendercels(void);
void buildcellist(struct VisOb *vo, int nvo, struct Vertex *verts, struct Vertex *xfverts, int indirect);
void img2ccb(struct ImageEntry *ie, struct CCB *ccb);
void cyclewalls(int32 nvbls);
void initwallanim(struct ImageEnv *iev);
void closewallanim(void);
void cycleanimloafs(struct AnimLoaf *al, int32 nal, int32 nvbls);
int32 createanimloafs(struct ImageEnv *iev, struct AnimLoaf **alp);
void deleteanimloafs(struct AnimLoaf *al);
void processgrid(void);
void processvisobs(void);
void initrend(void);
void closerend(void);
void loadskull(void);
void freeskull(void);
void simpledeath(void);
int testlinebuf(uint32 *linebuf, int32 lx, int32 rx);
void fadecel(struct CCB *ccb, frac16 value);
void createbackwall(void);
void FasterMapCel(struct CCB *ccb, struct Point *p);
/* shoot.c */
void shoot(void);
void probe(void);
void sequencegun(frac16 dist);
void loadgun(void);
/* objects.c */
void freeobjects(void);
struct Object *createStdObject(struct ObDef *od, int type, int size, int flags, struct Object *dupsrc);
void deleteStdObject(struct Object *ob);
void initobdefs(void);
void destructobdefs(void);
void registerobs(struct VisOb *vo);
void renderobs(struct VisOb *vo);
void moveobjects(int nframes);
void removeobfromme(struct Object *ob, struct MapEntry *me);
void addobtome(struct Object *ob, struct MapEntry *me);
void removeobfromslot(struct Object *ob);
void takedamage(int32 hitpoints);
frac16 approx2dist(frac16 x1, frac16 y1, frac16 x2, frac16 y2);
frac16 approx3dist(frac16 x1, frac16 y1, frac16 z1, frac16 x2, frac16 y2, frac16 z2);
int32 nextanimframe(struct AnimDef *ad, int32 fnum, int32 nvbls);
int setupanimarray(struct CelArray **captr, int *animseq, struct CCB **ccbp);
void toggledoor(struct Object *ob);
/* ob_zombie.c */
void loadzombie(void);
void destructzombie(void);
/* ob_george.c */
void loadgeorge(void);
void destructgeorge(void);
/* ob_head.c */
void loadhead(void);
void destructhead(void);
/* ob_spider.c */
void loadspider(void);
void destructspider(void);
/* ob_powerup.c */
/* ob_exit.c */
/* ob_trigger.c */
/* clip.c */
frac16 zclip(struct Vertex **xv, int zflags, struct Point *crnr);
void zClipCCB(struct CCB *ccb, frac16 factor);
void xClipCCB(struct CCB *ccb, int32 factor);
void initclip(void);
/* levelfile.c */
int loadlevelmap(char *filename);
void dropfaces(void);
void freelevelmap(void);
char *snarfstr(char *s, char *dest, char *terminators);
/* leveldef.c */
/* genmessage.c */
void initfont(void);
void closefont(void);
void initmessages(void);
void closemessages(void);
int rendermessage(int32 dlay, char *str);
void clearmessage(void);
/* statscreen.c */
void openmmfont(void);
void closemmfont(void);
void loadintrlevscreen(void);
void closeintrlevscreen(void);
int dointerlev(void);
int dispintrlev(int dosound);
void killshard(void);
void loadshard(void);
void loadstatscreen(void);
void closestatscreen(void);
int dostatmap(void);
int dispstats(void);
void loadoptionscreen(void);
void closeoptionscreen(void);
int attemptoptions(void);
int dispoptionscreen(int32 startresume);
int loadgame(void);
int savegame(void);
void showcredits(void);
/* thread.c */
void joythreadfunc(void);
void resetjoydata(void);
/* titleseq.c */
int dotitle(void);
/* imgfile.c */
struct CelArray *parse3DO(char *filename);
struct CelArray *alloccelarray(int32 nentries);
void freecelarray(struct CelArray *ca);
int32 cvt2power(int32 val);
/* loadloaf.c */
struct ImageEnv *loadloaf(char *filename);
void freeloaf(struct ImageEnv *iev);
/* file.c */
void *allocloadfile(char *filename, int32 memtype, int32 *err_len);
void filerr(char *filename, int32 err);
void filedie(char *filename, int32 err);
/* timing.c */
void drawnumxy(Item bmi, int32 num, int32 x, int32 y);
void opentimer(void);
void gettime(struct timeval *tv);
int32 subtime(struct timeval *tv1, struct timeval *tv2);
/* sound.c */
void playsound(int id);
void initsound(void);
void closesound(void);
void loadsfx(void);
void freesfx(void);
int32 loadsound(char *filename, int32 id);
void unloadsound(int32 id);
void spoolsound(char *filename, int32 nreps);
void stopspoolsound(int32 nsecs);
void waitspoolstop (void);
int issoundspooling(void);
/* soundinterface.c */
int32 CallSound(union CallSoundRec *soundPtr);
/* cinepak.c */
void initstreaming(void);
void shutdownstreaming(void);
void freebufferlist(struct DSDataBuf *buf);
void openstream(char *filename);
void closestream(void);
int endofstream(void);
struct CCB *getstreamccb(void);
void uncpaktorp(struct RastPort *rp);
void startstream(void);
void stopstream(void);
int playcpak(char *filename);
/* map.c */
int drawmap(void);
void loadmap(void);
void freemap(void);
/* option.c */
void LoadBkgdScreen(char *fileName);
void CloseBkgdScreen(void);
Err DoHighScoreScr(int32 newScore);
Err DoLoadGameScr(struct SaveGameRec *theGame);
Err DoSaveGameScr(struct SaveGameRec *theGame);
Err ClearSaveStuff(int32 clearScores, int32 clearSaves);
/* elkabong.c */
int32 performElKabong(void *buf);
int32 initElKabong(char *filename);
void closeElKabong(void);
int elKabongRequired(void);
