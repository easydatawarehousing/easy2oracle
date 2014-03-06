/* This file is part of Easy to Oracle - Free Open Source Data Integration
*
* Copyright (C) 2011-2014 Ivo Herweijer
*
* Easy to Oracle is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
*
* You can contact me via email: info@easydatawarehousing.com
*/

#include "eto_dbs.h"
#include "eto_log.h"
#include <stdlib.h>
#include <string.h>
#include <sqlite3.h>

char *getErrorDescription(const unsigned int code)
{
	switch (code)
	{
		case SQLITE_OK         : return "Successful result"; break;
		case SQLITE_ERROR      : return "SQL error or missing database"; break;
		case SQLITE_INTERNAL   : return "Internal logic error in SQLite"; break;
		case SQLITE_PERM       : return "Access permission denied"; break;
		case SQLITE_ABORT      : return "Callback routine requested an abort"; break;
		case SQLITE_BUSY       : return "The database file is locked"; break;
		case SQLITE_LOCKED     : return "A table in the database is locked"; break;
		case SQLITE_NOMEM      : return "A malloc() failed"; break;
		case SQLITE_READONLY   : return "Attempt to write a readonly database"; break;
		case SQLITE_INTERRUPT  : return "Operation terminated by sqlite3_interrupt()"; break;
		case SQLITE_IOERR      : return "Some kind of disk I/O error occurred"; break;
		case SQLITE_CORRUPT    : return "The database disk image is malformed"; break;
		case SQLITE_NOTFOUND   : return "NOT USED. Table or record not found"; break;
		case SQLITE_FULL       : return "Insertion failed because database is full"; break;
		case SQLITE_CANTOPEN   : return "Unable to open the database file"; break;
		case SQLITE_PROTOCOL   : return "NOT USED. Database lock protocol error"; break;
		case SQLITE_EMPTY      : return "Database is empty"; break;
		case SQLITE_SCHEMA     : return "The database schema changed"; break;
		case SQLITE_TOOBIG     : return "String or BLOB exceeds size limit"; break;
		case SQLITE_CONSTRAINT : return "Abort due to constraint violation"; break;
		case SQLITE_MISMATCH   : return "Data type mismatch"; break;
		case SQLITE_MISUSE     : return "Library used incorrectly. Check the SQL statement"; break;
		case SQLITE_NOLFS      : return "Uses OS features not supported on host"; break;
		case SQLITE_AUTH       : return "Authorization denied"; break;
		case SQLITE_FORMAT     : return "Auxiliary database format error"; break;
		case SQLITE_RANGE      : return "2nd parameter to sqlite3_bind out of range"; break;
		case SQLITE_NOTADB     : return "File opened that is not a database file"; break;
		case SQLITE_ROW        : return "sqlite3_step() has another row ready"; break;
		case SQLITE_DONE       : return "sqlite3_step() has finished executing"; break;
	}

	return "Unknown errorcode";
};

static void eto_write_output(QUERYDATA *qdata, const sqlite3_stmt *statement)
{
	unsigned int	i, columncount, curr_cache_len, newdatalen;
	unsigned int	 rc = SQLITE_ROW;
	char			*fieldvalue;

	columncount = (unsigned int)sqlite3_data_count(statement);

	// Get the data
	while (rc == SQLITE_ROW)
	{
		for (i = 0; i < columncount; i++)
		{
			// Get field valua as text
			fieldvalue = (char*)sqlite3_column_text(statement, i);

			// Check for NULL
			if (!fieldvalue)
				newdatalen = 1;	// Just a separator
			else
				newdatalen = strlen(fieldvalue);

			// Cut off at 4000 characters
			if (newdatalen > qdata->vml)
				newdatalen = qdata->vml;

			// Check cache size
			curr_cache_len = strlen(qdata->cache);
			if (qdata->cache_size < curr_cache_len + newdatalen + 3)
			{
				qdata->cache_size = ( (curr_cache_len + newdatalen + 1027) / 1024) * 1024; // add block(s) of 1 Kb
				qdata->cache = (char*)realloc( (char*)qdata->cache, qdata->cache_size);
				if (!qdata->cache)
					exit(1);
			}

			if (fieldvalue)
				strncat(qdata->cache, fieldvalue, newdatalen);

			if (i < columncount - 1)
				strcat(qdata->cache, qdata->fsep);
		}
		strcat(qdata->cache, qdata->lsep);

		// print rows to output
		qdata->rows_processed++;
		if (qdata->rows_processed == qdata->cache_rows)
		{
			qdata->rows_processed = 0;
			printf("%s", qdata->cache);
			qdata->cache[0] = 0;
		}

		// Get next row
		rc = sqlite3_step(statement);
	}

	// print last rows to output
	if (qdata->rows_processed > 0)
		printf("%s", qdata->cache);

	// Report reason of end of datafetch
	if (rc != SQLITE_DONE)
		eto_log_add_entry_s("ETO-10246 Queryresults:Unknown error: ", getErrorDescription(rc) );
}

static void eto_describe_output(QUERYDATA *qdata, const sqlite3_stmt *statement)
{
	unsigned int	i, columncount, columnlength;
	int				columntype, realcolumntype;
	char			fieldtype[18];

	columncount = sqlite3_data_count(statement);

	// Output header
	if (qdata->hdr && !qdata->hdr_printed && columncount > 0)
	{
		if (!qdata->is_describe)
		{
			for (i = 0; i < columncount; i++)
				printf("%s%s", i > 0 ? qdata->fsep : "", sqlite3_column_name(statement, i) );

			printf("%s", qdata->lsep);
		}
		else
		{
			// Copy header record to output
			printf("describe_id%sname%sdatatype%sdatatype_descr%smaxlength%sscale%sprecision%snotnull%sstatement%s"
				, qdata->fsep, qdata->fsep, qdata->fsep, qdata->fsep, qdata->fsep, qdata->fsep, qdata->fsep, qdata->fsep
				, qdata->lsep);

			for (i = 0; i < columncount; i++)
			{
				// Describe column
				realcolumntype = columntype = sqlite3_column_type(statement, i);
				switch (columntype)
				{
					case SQLITE_INTEGER		: strcpy(fieldtype, "SQLITE_INTEGER");	columnlength = 0;			break;
					case SQLITE_FLOAT		: strcpy(fieldtype, "SQLITE_FLOAT");	columnlength = 0;			break;
					case SQLITE_TEXT		: strcpy(fieldtype, "SQLITE_TEXT");		columnlength = qdata->vml;	break;
					case SQLITE_BLOB		: strcpy(fieldtype, "SQLITE_BLOB");		columnlength = 0;			break;
					case SQLITE_NULL		: strcpy(fieldtype, "SQLITE_TEXT");		columnlength = qdata->vml;	realcolumntype = SQLITE_TEXT;	break;
					default					: strcpy(fieldtype, "SQLITE_TEXT");		columnlength = qdata->vml;	realcolumntype = SQLITE_TEXT;	break;
				}

				// Print to output
				printf("%d%s%s%s%d%s%s%s%d%s%s%sN%s%s"
						, qdata->describe_id
						, qdata->fsep
						, sqlite3_column_name(statement, i)
						, qdata->fsep
						, realcolumntype + 40000
						, qdata->fsep
						, fieldtype
						, qdata->fsep
						, columnlength
						, qdata->fsep
						, qdata->fsep
						, qdata->fsep
						, qdata->fsep
						, qdata->sql
						);

				if (i < columncount - 1)
					printf("%s", qdata->lsep);
			}
		}

		qdata->hdr_printed = true;
	}
}

static void eto_execute_query(QUERYDATA *qdata, const sqlite3 *conn)
{
	sqlite3_stmt	*statement;
	unsigned int	 rc;

	// Prepare query
	rc =  sqlite3_prepare(conn, qdata->sql, -1, &statement, 0);

	if (rc)
	{
		eto_log_add_entry_s("ETO-10327 Execute query error: ", getErrorDescription(rc));
		return;
	}

	// Start fetch
	rc = sqlite3_step(statement);

	if (rc != SQLITE_ROW && rc != SQLITE_DONE)
	{
		eto_log_add_entry_s("ETO-10342 Execute query error getting results: ", getErrorDescription(rc));
	}
	else
	{
		// Headers first
		eto_describe_output(qdata, statement);

		// Data next
		if (!qdata->is_describe)
			eto_write_output(qdata, statement);
	}

	// Finish and cleanup
	rc = sqlite3_finalize(statement);

	if (rc)
		eto_log_add_entry_s("ETO-10349 Describe: Cancel query failed: ", getErrorDescription(rc) );
}

bool eto_open_database(ETO_PARAMS* params)
{
	QUERYDATA	*qdata;
	sqlite3		*conn;
	unsigned int rc;

	// Check parameters
	if (!params || !params->dbs)
	{
		eto_log_add_entry("ETO-10110 No database servername or address given");
		return false;
	}

	if (!params->tbl && !params->sql)
	{
		eto_log_add_entry("ETO-10113 No tablename or query given");
		return false;
	}

	// No port needed
	
	// Set up data
	qdata = eto_new_querydata(params->sql, params->tbl);

	if (!qdata)
	{
		// Log messages added by eto_new_querydata
		return false;
	}

	qdata->is_describe		= params->did > 0;
	qdata->describe_id		= params->did;
	qdata->hdr				= (bool)params->hdr || qdata->is_describe;
	qdata->cache_rows		= params->rws > 0 ? params->rws : 1;
	qdata->vml				= params->vml;

	// Convert separator strings
	if (!eto_convert_separators(qdata->fsep, qdata->lsep, params->fsep, params->lsep) )
	{
		// Log messages added by eto_convert_separators
		return false;
	}

	// Check/copy/create sql
	if (!eto_create_query(&qdata->sql, params->sql, params->tbl, false /* is_describe: always false for sqlite */, false) )
	{
		// Log messages added by eto_create_query
		return false;
	}

	// SQLite wants a ;
	strcat(qdata->sql, ";");

	// Open SQLite database
	rc = sqlite3_open(params->dbs, &conn);

	if (rc)
	{
		sqlite3_close(conn);

		eto_log_add_entry_s("ETO-10202 Open database failed: ", sqlite3_errmsg(conn) );
		return false;
	}

	// Execute query
	eto_execute_query(qdata, conn);

	// Cleanup
	if (sqlite3_close(conn) )
	{
		eto_log_add_entry_s("ETO-10250 Close database failed: ", sqlite3_errmsg(conn) );
	}

	eto_delete_querydata(qdata);

	return true;
}
