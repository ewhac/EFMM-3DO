/*  :ts=8 bk=0
 *
 * file.c:	File handling routines (Opera filesystem).
 *
 * Leo L. Schwab					9301.18
 ***************************************************************************
 *				--== RCS Log ==--
 * $Log$
 */
#include <types.h>
#include <mem.h>
#include <io.h>
#include <filestream.h>
#include <filestreamfunctions.h>
#include <debug.h>
#include <stdlib.h>


/***************************************************************************
 * Prototypes.
 */
void *allocloadfile(char *filename, int32 memtype, int32 *err_len);
void filerr(char *filename, int32 err);
void filedie(char *filename, int32 err);

extern void	closestuff (void);


/***************************************************************************
 * This handy little routine allocates a buffer that's large enough for
 * the named file, then loads that file into the buffer, and.....
 * returns a pointer to the client.  The length of the file is written to
 * the int32 pointed to by err_len.
 * If any of the operations fails, nothing is allocated, NULL is returned,
 * and err_len contains a pointer to a diagnostic string which can then be
 * passed to filerr() or filedie().
 */
void *
allocloadfile (filename, memtype, err_len)
char	*filename;
int32	memtype;
int32	*err_len;
{
	Stream	*stream;
	int32	len;
	char	*errstr;
	void	*buf;

	buf = NULL;
	errstr = NULL;

	/*
	 * Try and load it.
	 */
	if (stream = OpenDiskStream (filename, 0)) {
		if ((len = stream->st_FileLength) > 0) {
			if (buf = ALLOCMEM (len, memtype)) {
				if (ReadDiskStream (stream, buf, len) < 0)
					errstr = "Error reading file.";
			} else
				errstr = "No memory to load file.";
		} else
			errstr = "File is empty.";
		CloseDiskStream (stream);
	} else
		errstr = "File not found.";

	/*
	 * Clean up in case of failure.
	 */
	if (errstr) {
		if (buf) {
			FREEMEM (buf, len);
			buf = NULL;
		}
		len = (int32) errstr;
	}
	if (err_len)
		*err_len = len;
	return (buf);
}


void
filerr (filename, err)
char	*filename;
int32	err;
{
	kprintf ("%s: %s\n", filename, (char *) err);
}

void
filedie (filename, err)
char	*filename;
int32	err;
{
	filerr (filename, err);
	closestuff ();
	exit (20);
}
