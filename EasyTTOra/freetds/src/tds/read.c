/* FreeTDS - Library of routines accessing Sybase and Microsoft databases
 * Copyright (C) 1998, 1999, 2000, 2001, 2002, 2003, 2004  Brian Bruns
 * Copyright (C) 2005, 2006, 2007, 2010  Frediano Ziglio
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#if HAVE_CONFIG_H
#include <config.h>
#endif

#if HAVE_ERRNO_H
#include <errno.h>
#endif /* HAVE_ERRNO_H */

#if HAVE_STDLIB_H
#include <stdlib.h>
#endif /* HAVE_STDLIB_H */

#if HAVE_STRING_H
#include <string.h>
#endif /* HAVE_STRING_H */

#if HAVE_UNISTD_H
#include <unistd.h>
#endif /* HAVE_UNISTD_H */

#include <assert.h>

#include "tds.h"
#include "tdsiconv.h"
#include "tdsbytes.h"
#ifdef DMALLOC
#include <dmalloc.h>
#endif

TDS_RCSID(var, "$Id: read.c,v 1.112 2010/07/30 07:34:06 freddy77 Exp $");

static int read_and_convert(TDSSOCKET * tds, const TDSICONV * char_conv,
			    size_t * wire_size, char **outbuf, size_t * outbytesleft);

/**
 * \ingroup libtds
 * \defgroup network Network functions
 * Functions for reading or writing from network.
 */

/**
 * \addtogroup network
 * @{ 
 */

/*
** Return a single byte from the input buffer
*/
unsigned char
tds_get_byte(TDSSOCKET * tds)
{
	while (tds->in_pos >= tds->in_len) {
		if (tds_read_packet(tds) < 0)
			return 0;
	}
	return tds->in_buf[tds->in_pos++];
}

/**
 * Unget will always work as long as you don't call it twice in a row.  It
 * it may work if you call it multiple times as long as you don't backup
 * over the beginning of network packet boundary which can occur anywhere in
 * the token stream.
 */
void
tds_unget_byte(TDSSOCKET * tds)
{
	/* this is a one trick pony...don't call it twice */
	tds->in_pos--;
}

unsigned char
tds_peek(TDSSOCKET * tds)
{
	unsigned char result = tds_get_byte(tds);
	if (tds->in_pos > 0)
		--tds->in_pos;
	return result;
}				/* tds_peek()  */


/**
 * Get an int16 from the server.
 */
TDS_SMALLINT
tds_get_smallint(TDSSOCKET * tds)
{
	unsigned char bytes[2];

	tds_get_n(tds, bytes, 2);
#if WORDS_BIGENDIAN
	if (tds->emul_little_endian)
		return (TDS_SMALLINT) TDS_GET_A2LE(bytes);
#endif
	return (TDS_SMALLINT) TDS_GET_A2(bytes);
}


/**
 * Get an int32 from the server.
 */
TDS_INT
tds_get_int(TDSSOCKET * tds)
{
	unsigned char bytes[4];

	tds_get_n(tds, bytes, 4);
#if WORDS_BIGENDIAN
	if (tds->emul_little_endian)
		return (TDS_INT) TDS_GET_A4LE(bytes);
#endif
	return (TDS_INT) TDS_GET_A4(bytes);
}

TDS_INT8
tds_get_int8(TDSSOCKET * tds)
{
	TDS_INT h;
	TDS_UINT l;
	unsigned char bytes[8];

	tds_get_n(tds, bytes, 8);
#if WORDS_BIGENDIAN
	if (tds->emul_little_endian) {
		l = TDS_GET_A4LE(bytes);
		h = (TDS_INT) TDS_GET_A4LE(bytes+4);
	} else {
		h = (TDS_INT) TDS_GET_A4(bytes);
		l = TDS_GET_A4(bytes+4);
	}
#else
	l = TDS_GET_A4(bytes);
	h = (TDS_INT) TDS_GET_A4(bytes+4);
#endif
	return (((TDS_INT8) h) << 32) | l;
}

#if ENABLE_EXTRA_CHECKS
# define TEMP_INIT(s) char* temp = (char*)malloc(32); const size_t temp_size = 32
# define TEMP_FREE free(temp);
# define TEMP_SIZE temp_size
#else
# define TEMP_INIT(s) char temp[s]
# define TEMP_FREE ;
# define TEMP_SIZE sizeof(temp)
#endif

/**
 * Fetch a string from the wire.
 * Output string is NOT null terminated.
 * If TDS version is 7 or 8 read unicode string and convert it.
 * This function should be use to read server default encoding strings like 
 * columns name, table names, etc, not for data (use tds_get_char_data instead)
 * @return bytes written to \a dest
 * @param tds  connection information
 * @param string_len length of string to read from wire 
 *        (in server characters, bytes for tds4-tds5, ucs2 for tds7+)
 * @param dest destination buffer, if NULL string is read and discarded
 * @param dest_size destination buffer size, in bytes
 */
int
tds_get_string(TDSSOCKET * tds, int string_len, char *dest, size_t dest_size)
{
	size_t wire_bytes;

	/*
	 * FIX: 02-Jun-2000 by Scott C. Gray (SCG)
	 * Bug to malloc(0) on some platforms.
	 */
	if (string_len == 0) {
		return 0;
	}

	assert(string_len >= 0 && dest_size >= 0);

	wire_bytes = IS_TDS7_PLUS(tds) ? string_len * 2 : string_len;

	if (IS_TDS7_PLUS(tds)) {
		if (dest == NULL) {
			tds_get_n(tds, NULL, (int)wire_bytes);
			return string_len;
		}

		return read_and_convert(tds, tds->char_convs[client2ucs2], &wire_bytes, &dest, &dest_size);
	} else {
		/* FIXME convert to client charset */
		assert(dest_size >= (size_t) string_len);
		tds_get_n(tds, dest, string_len);
		return string_len;
	}
}

/**
 * Fetch character data the wire.
 * Output is NOT null terminated.
 * If \a char_conv is not NULL, convert data accordingly.
 * \param tds         state information for the socket and the TDS protocol
 * \param row_buffer  destination buffer in current_row. Can't be NULL
 * \param wire_size   size to read from wire (in bytes)
 * \param curcol      column information
 * \return TDS_SUCCEED or TDS_FAIL (probably memory error on text data)
 * \todo put a TDSICONV structure in every TDSCOLUMN
 */
int
tds_get_char_data(TDSSOCKET * tds, char *row_buffer, size_t wire_size, TDSCOLUMN * curcol)
{
	size_t in_left;
	TDSBLOB *blob = NULL;
	char *dest = row_buffer;

	if (is_blob_col(curcol)) {
		blob = (TDSBLOB *) row_buffer;
		dest = blob->textvalue;
	}

	/* 
	 * dest is usually a column buffer, allocated when the column's metadata are processed 
	 * and reused for each row.  
	 * For blobs, dest is blob->textvalue, and can be reallocated or freed
	 * TODO: reallocate if blob and no space 
	 */
	 
	/* silly case, empty string */
	if (wire_size == 0) {
		curcol->column_cur_size = 0;
		if (blob)
			TDS_ZERO_FREE(blob->textvalue);
		return TDS_SUCCEED;
	}

	if (curcol->char_conv) {
		/*
		 * TODO The conversion should be selected from curcol and tds version
		 * TDS8/single -> use curcol collation
		 * TDS7/single -> use server single byte
		 * TDS7+/unicode -> use server (always unicode)
		 * TDS5/4.2 -> use server 
		 * TDS5/UTF-8 -> use server
		 * TDS5/UTF-16 -> use UTF-16
		 */
		in_left = blob ? curcol->column_cur_size : curcol->column_size;
		curcol->column_cur_size = read_and_convert(tds, curcol->char_conv, &wire_size, &dest, &in_left);
		if (wire_size > 0) {
			tdsdump_log(TDS_DBG_NETWORK, "error: tds_get_char_data: discarded %u on wire while reading %d into client. \n", 
							 (unsigned int) wire_size, curcol->column_cur_size);
			return TDS_FAIL;
		}
	} else {
		curcol->column_cur_size = (TDS_INT)wire_size;
		if (tds_get_n(tds, dest, (int)wire_size) == NULL) {
			tdsdump_log(TDS_DBG_NETWORK, "error: tds_get_char_data: failed to read %u from wire. \n",
				    (unsigned int) wire_size);
			return TDS_FAIL;
		}
	}
	return TDS_SUCCEED;
}

/**
 * Get N bytes from the buffer and return them in the already allocated space  
 * given to us.  We ASSUME that the person calling this function has done the  
 * bounds checking for us since they know how many bytes they want here.
 * dest of NULL means we just want to eat the bytes.   (tetherow@nol.org)
 */
void *
tds_get_n(TDSSOCKET * tds, void *dest, int need)
{
	assert(need >= 0);

	for (;;) {
		int have = tds->in_len - tds->in_pos;

		if (need <= have)
			break;
		/* We need more than is in the buffer, copy what is there */
		if (dest != NULL) {
			memcpy((char *) dest, tds->in_buf + tds->in_pos, have);
			dest = (char *) dest + have;
		}
		need -= have;
		if (tds_read_packet(tds) < 0)
			return NULL;
	}
	if (need > 0) {
		/* get the remainder if there is any */
		if (dest != NULL) {
			memcpy((char *) dest, tds->in_buf + tds->in_pos, need);
		}
		tds->in_pos += need;
	}
	return dest;
}

/*
 * For UTF-8 and similar, tds_iconv() may encounter a partial sequence when the chunk boundary
 * is not aligned with the character boundary.  In that event, it will return an error, and
 * some number of bytes (less than a character) will remain in the tail end of temp[].  They are  
 * moved to the beginning, ptemp is adjusted to point just behind them, and the next chunk is read.
 */
static int
read_and_convert(TDSSOCKET * tds, const TDSICONV * char_conv, size_t * wire_size, char **outbuf,
		 size_t * outbytesleft)
{
	TEMP_INIT(256);
	/*
	 * temp (above) is the "preconversion" buffer, the place where the UCS-2 data 
	 * are parked before converting them to ASCII.  It has to have a size, 
	 * and there's no advantage to allocating dynamically.  
	 * This also avoids any memory allocation error.  
	 */
	const char *bufp;
	size_t bufleft = 0;
	const size_t max_output = *outbytesleft;

	/* cast away const for message suppression sub-structure */
	TDS_ERRNO_MESSAGE_FLAGS *suppress = (TDS_ERRNO_MESSAGE_FLAGS*) &char_conv->suppress;

	memset(suppress, 0, sizeof(char_conv->suppress));
	
	for (bufp = temp; *wire_size > 0 && *outbytesleft > 0; bufp = temp + bufleft) {
		assert(bufp >= temp);
		/* read a chunk of data */
		bufleft = TEMP_SIZE - bufleft;
		if (bufleft > *wire_size)
			bufleft = *wire_size;
		tds_get_n(tds, (char *) bufp, (int)bufleft);
		*wire_size -= bufleft;
		bufleft += bufp - temp;

		/* Convert chunk and write to dest. */
		bufp = temp; /* always convert from start of buffer */
		suppress->einval = *wire_size > 0; /* EINVAL matters only on the last chunk. */
		if ((size_t)-1 == tds_iconv(tds, char_conv, to_client, &bufp, &bufleft, outbuf, outbytesleft)) {
			tdsdump_log(TDS_DBG_NETWORK, "Error: read_and_convert: tds_iconv returned errno %d\n", errno);
			if (errno != EILSEQ) {
				tdsdump_log(TDS_DBG_NETWORK, "Error: read_and_convert: "
							     "Gave up converting %u bytes due to error %d.\n",
							     (unsigned int) bufleft, errno);
				tdsdump_dump_buf(TDS_DBG_NETWORK, "Troublesome bytes:", bufp, bufleft);
			}

			if (bufp == temp) {	/* tds_iconv did not convert anything, avoid infinite loop */
				tdsdump_log(TDS_DBG_NETWORK, "No conversion possible: draining remaining %u bytes.\n",
							     (unsigned int) *wire_size);
				tds_get_n(tds, NULL, (int)(*wire_size)); /* perhaps we should read unconverted data into outbuf? */
				*wire_size = 0;
				break;
			}
				
			if (bufleft) {
				memmove(temp, bufp, bufleft);
			}
		}
	}

	assert(*wire_size == 0 || *outbytesleft == 0);

	TEMP_FREE;
	return (int)(max_output - *outbytesleft);
}

/** @} */
