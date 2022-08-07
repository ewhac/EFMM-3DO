/*  :ts=8 bk=0
 *
 * cinepak.c:	CinePak support code.
 *
 * Leo L. Schwab					9309.28
 */
#define	DEBUG	1
#include <types.h>
#include <graphics.h>

#include "DataStreamDebug.h"
#include "DataStreamLib.h"
#include "DataAcq.h"
#include "CPakSubscriber.h"
#include "SAudioSubscriber.h"
#include "ControlSubscriber.h"

#include "castle.h"

#include "app_proto.h"


/***************************************************************************
 * #defines
 */
#define	NSTREAMS			1
#define	STREAMER_DELTA_PRI		6
#define	SUBSCRIBER_DELTA_PRI		7
#define	AUDIO_SUBSCRIBER_DELTA_PRI	10
#define	CTRL_SUBSCRIBER_DELTA_PRI	(AUDIO_SUBSCRIBER_DELTA_PRI + 1)
#define	DATA_ACQ_DELTA_PRI		8

#define	DATA_BUFFER_COUNT		5
#define	DATA_BUFFER_SIZE		(128 * 1024)

#define	ROUND_TO_LONG(x)		((x + 3) & ~3)


/***************************************************************************
 * Local prototypes.
 */
static DSDataBuf *createbufferlist (long, long);


/***************************************************************************
 * Globaloids.
 */
extern RastPort		*rpvis, *rprend;
extern JoyData		jd;
extern Item		vblIO;


static CtrlContext	*sc_ctl;
static SAudioContext	*sc_saudio;
static CPakContext	*sc_cpak;
static CPakRec		*cpakrec;
static DSStreamCB	*streamcb;
static DSDataBuf	*streambuf;
static AcqContext	*acqctxt;

static Item		streamport;
static Item		streammsg;



/***************************************************************************
 * Code.  (As if you needed this comment to tell you that.  Duh...)
 */
void
initstreaming ()
{
	/*
	 * Create communication items.
	 */
	if ((streamport = NewMsgPort (NULL)) < 0)
		die ("Can't create streamport.\n");

	if ((streammsg = CreateMsgItem (streamport)) < 0)
		die ("Can't create streammsg.\n");

	/*
	 * Initialize required streaming modules.
	 */
	if (InitDataAcq (1) < 0  ||
	    InitDataStreaming (NSTREAMS) < 0)
		die ("Stream system initialization failed.\n");

	if (InitCPakSubscriber () < 0  ||
	    InitSAudioSubscriber () < 0  ||
	    InitCtrlSubscriber () < 0)
		die ("Subscriber module initialization failed.\n");

	/*
	 * Create buffer list.
	 */
	if (NewDataStream (&streamcb, NULL, DATA_BUFFER_SIZE,
			   STREAMER_DELTA_PRI) < 0)
		die ("NewDataStream() failed.\n");

	/*
	 * Initialize Control subscriber.
	 */
	if (NewCtrlSubscriber (&sc_ctl,
			       streamcb,
			       CTRL_SUBSCRIBER_DELTA_PRI) < 0)
		die ("NewCtrlSubscriber() failed.\n");

	if (DSSubscribe (streammsg, NULL, streamcb,
			 CTRL_CHUNK_TYPE,
			 sc_ctl->requestPort) < 0)
		die ("Control subscription failed.\n");

	/*
	 * Initialize audio subscriber.
	 */
	if (NewSAudioSubscriber (&sc_saudio,
				 streamcb,
				 AUDIO_SUBSCRIBER_DELTA_PRI) < 0)
		die ("NewSAudioSubscriber() failed.\n");

	if (DSSubscribe (streammsg, NULL, streamcb,
			 SNDS_CHUNK_TYPE,
			 sc_saudio->requestPort) < 0)
		die ("Audio subscription failed.\n");

	/*
	 * Initialize CinePak subscriber.
	 */
	if (NewCPakSubscriber (&sc_cpak, NSTREAMS, SUBSCRIBER_DELTA_PRI) < 0)
		die ("NewCPakSubscriber() failed.\n");

	if (DSSubscribe (streammsg, NULL, streamcb,
			 FILM_CHUNK_TYPE,
			 sc_cpak->requestPort) < 0)
		die ("CinePak subscription failed.\n");

	if (InitCPakCel (streamcb,
			 sc_cpak,
			 &cpakrec,
			 0,
			 TRUE) < 0)
		die ("InitCPakCel() failed.\n");
}


void
shutdownstreaming ()
{
	DSSubscribe (streammsg, NULL, streamcb, FILM_CHUNK_TYPE, 0);
	DSSubscribe (streammsg, NULL, streamcb, SNDS_CHUNK_TYPE, 0);
	DSSubscribe (streammsg, NULL, streamcb, CTRL_CHUNK_TYPE, 0);

	DestroyCPakCel (sc_cpak, cpakrec, 0);

	DisposeCtrlSubscriber (sc_ctl);
	DisposeSAudioSubscriber (sc_saudio);
	DisposeCPakSubscriber (sc_cpak);

	DisposeDataStream (streammsg, streamcb);  streamcb = NULL;

	RemoveMsgItem (streammsg);  streammsg = 0;
	RemoveMsgPort (streamport);  streamport = 0;
}


/***************************************************************************
 * Data stream buffer management.
 */
static DSDataBuf *
createbufferlist (
long	numBuffers,
long	bufferSize
)
{
	DSDataBuf	*bp;
	DSDataBuf	*lastBp;
	long		totalBufferSpace;
	long		actualEntrySize;

	actualEntrySize = sizeof (DSDataBuf) + ROUND_TO_LONG (bufferSize);
	totalBufferSpace = numBuffers * actualEntrySize;

	if (!(bp = malloc (totalBufferSpace)))
		die ("malloc() of buffer list space failed!\n");

	/*  Make a linked list of entries  */
	lastBp = NULL;
	while (--numBuffers >= 0)
	{
		bp->permanentNext	= lastBp;
		bp->next		= lastBp;
		lastBp			= bp;

		bp = (DSDataBuf *) (((char *) bp) + actualEntrySize);
	}

	/*
	 * Return a pointer to the first buffer in the list.
	 */
	return (lastBp);
}

void
freebufferlist (buf)
struct DSDataBuf	*buf;
{
	if (buf) {
		while (buf->permanentNext)
			buf = buf->permanentNext;

		free (buf);
	}
}


/***************************************************************************
 * Individual stream management.
 */
static int32	templateTags[] = {
	SA_22K_16B_S_SDX2,
	SA_22K_16B_M_SDX2,
	0
};

void
openstream (filename)
char	*filename;
{
	SAudioCtlBlock	audiocmd;

	/*
	 * Actually create buffer and install into DSStreamCB.
	 * NOTE: This am a hack; may not be supported.  (Works, though.)
	 */
	streambuf = createbufferlist (DATA_BUFFER_COUNT, DATA_BUFFER_SIZE);
	streamcb->freeBufHead = streambuf;

	/*
	 * Initialize data acquisition
	 */
	if (NewDataAcq (&acqctxt, filename, DATA_ACQ_DELTA_PRI) < 0)
		die ("Failed to acquire stream file.\n");

	if (DSConnect (streammsg, NULL, streamcb,
		       acqctxt->requestPort) < 0)
		die ("Failed to connect stream to acquisitor.\n");

	audiocmd.loadTemplates.tagListPtr = templateTags;
	if (DSControl (streammsg, NULL, streamcb,
		       SNDS_CHUNK_TYPE,
		       kSAudioCtlOpLoadTemplates,
		       &audiocmd) < 0)
		die ("Couldn't load audio templates.\n");

	if (DSSetChannel (streammsg, NULL, streamcb,
			  SNDS_CHUNK_TYPE,
			  1,
			  CHAN_ENABLED) < 0)
		die ("Couldn't set stream channel.\n");

	if (DSGoMarker (streammsg, NULL, streamcb,
			0,
			GOMARKER_ABSOLUTE) < 0)
		die ("Couldn't seek to marker.\n");

	if (DSPreRollStream (streammsg, FALSE, streamcb) < 0)
		die ("Preroll failed.\n");
}

void
closestream ()
{
	SAudioCtlBlock	audiocmd;

	audiocmd.closeChannel.channelNumber = 1;
	if (DSControl (streammsg, NULL, streamcb,
		       SNDS_CHUNK_TYPE,
		       kSAudioCtlOpCloseChannel,
		       &audiocmd) < 0)
		die ("Couldn't close audio channel.\n");

	if (DSConnect (streammsg, NULL, streamcb, 0) < 0)
		die ("Disconnect hack failed.\n");

	DisposeDataAcq (acqctxt);  acqctxt = NULL;

	streamcb->freeBufHead = NULL;
	freebufferlist (streambuf);  streambuf = NULL;
}


/***************************************************************************
 * Convenient goodies.
 */
int
endofstream ()
{
	register struct DSDataBuf	*buf;
	register int			usecnt;

	if (!(streamcb->streamFlags & STRM_EOF))
		return (FALSE);

	buf = streambuf;
	usecnt = 0;
	while (buf) {
		usecnt += buf->useCount;
		buf = buf->permanentNext;
	}
//kprintf ("usecnt was %d\n", usecnt);

	return (usecnt < 2);
}


struct CCB *
getstreamccb ()
{
	register CCB	*ccb;

	ccb = GetCPakCel (sc_cpak, cpakrec);
	SendFreeCPakSignal (sc_cpak);

	return (ccb);
}


void
uncpaktorp (rp)
struct RastPort	*rp;
{
	DrawCPakToBuffer (sc_cpak, cpakrec, rp->rp_Bitmap);
	SendFreeCPakSignal (sc_cpak);
}


void
startstream ()
{
	if (DSStartStream (streammsg, NULL, streamcb, 0) < 0)
		die ("Failed to start stream.\n");
}

void
stopstream ()
{
	if (DSStopStream (streammsg, NULL, streamcb, SOPT_FLUSH) < 0)
		die ("Failed to stop stream.\n");

	FlushCPakChannel (sc_cpak, cpakrec, 0);
}


/***************************************************************************
 * Even more convenient goodie.
 */
int
playcpak (filename)
char	*filename;
{
	register RastPort	*tmp;
	int			retval, foo;

	retval = FALSE;
	SetRast (rpvis, 0);
	SetRast (rprend, 0);
	DisplayScreen (rpvis->rp_ScreenItem, 0);

	openstream (filename);

	startstream ();
	resetjoydata ();
	while (!endofstream ()) {
		uncpaktorp (rprend);

		tmp = rpvis;  rpvis = rprend;  rprend = tmp;
		DisplayScreen (rpvis->rp_ScreenItem, 0);

		CopyRast (rpvis, rprend);
//		WaitVBL (vblIO, 1);

		if (jd.jd_ADown  ||  jd.jd_BDown  ||  jd.jd_CDown  ||
		    jd.jd_DX  ||  jd.jd_XDown  ||  jd.jd_StartDown)
		{
			retval = TRUE;
			break;
		}
	}
	stopstream ();
	closestream ();
	resetjoydata ();
	if (foo = performElKabong (NULL)) {
		kprintf ("El Kabong failed.\n");
		PrintfSysErr (foo);
	}

	fadetoblank (rpvis, 30);

	return (retval);
}
