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
#include <my_global.h>
#include <mysql.h>

static void eto_write_output(QUERYDATA *qdata, MYSQL_RES *result)
{
	MYSQL_ROW		 row;
	unsigned int	 i, columncount, newdatalen, lseplen, curr_cache_len = 0;

	lseplen = strlen(qdata->lsep);

	// Get fieldcount
	columncount = mysql_num_fields(result);

	if (!columncount)
		return;
	
	while ( (row = mysql_fetch_row(result) ) )
	{
		for(i = 0; i < columncount; i++)
		{
			// Check for NULL
			if (!row[i])
				newdatalen = 1;	// Just a separator
			else
				newdatalen = strlen(row[i]);

			// Cut off at 4000 characters
			if (newdatalen > qdata->vml)
				newdatalen = qdata->vml;

			// Check cache size
			if (qdata->cache_size < curr_cache_len + newdatalen + 3)
			{
				qdata->cache_size = ( (curr_cache_len + newdatalen + 1027) / 1024) * 1024; // add block(s) of 1 Kb
				qdata->cache = (char*)realloc( (char*)qdata->cache, qdata->cache_size);
				if (!qdata->cache)
					exit(1);
			}

			if (row[i])
				strncat(qdata->cache, row[i], newdatalen);

			curr_cache_len += newdatalen;

			if (i < columncount - 1)
			{
				strcat(qdata->cache, qdata->fsep);
				curr_cache_len++;
			}
		}

		strcat(qdata->cache, qdata->lsep);
		curr_cache_len += lseplen;

		// print rows to output
		qdata->rows_processed++;
		if (qdata->rows_processed == qdata->cache_rows)
		{
			qdata->rows_processed = 0;
			printf("%s", qdata->cache);
			qdata->cache[0] = 0;
			curr_cache_len = 0;
		}
	}

	// print last rows to output
	if (qdata->rows_processed > 0)
	{
		printf("%s", qdata->cache);
	//	fflush(stdout);
	}
}

static void eto_describe_output(QUERYDATA *qdata, MYSQL_RES *result)
{
	MYSQL_FIELD		*field;
	unsigned int	 i, columncount;
	char			 fieldtype[18];

	columncount = mysql_num_fields(result);

	if (!columncount)
	{
		eto_log_add_entry("ETO-10240 Queryresults: Error determining fieldcount");
		return;
	}

	// Output header
	if (qdata->hdr && !qdata->hdr_printed && columncount > 0)
	{
		if (!qdata->is_describe)
		{
			for (i = 0; i < columncount; i++)
			{
				field = mysql_fetch_field(result);
				printf("%s%s", i > 0 ? qdata->fsep : "", field->name);
			}

			printf("%s", qdata->lsep);
		}
		else
		{
			// Copy header record to output
			printf("%s", "describe_id;name;datatype;datatype_descr;maxlength;scale;precision;notnull;statement\n");

			while (field = mysql_fetch_field(result) )
			{
				// Describe column
				switch (field->type)
				{
					case MYSQL_TYPE_DECIMAL		: strcpy(fieldtype, "MYSQL_DECIMAL");		break;
					case MYSQL_TYPE_TINY		: strcpy(fieldtype, "MYSQL_TINY");			break;
					case MYSQL_TYPE_SHORT		: strcpy(fieldtype, "MYSQL_SHORT");			break;
					case MYSQL_TYPE_LONG		: strcpy(fieldtype, "MYSQL_LONG");			break;
					case MYSQL_TYPE_FLOAT		: strcpy(fieldtype, "MYSQL_FLOAT");			break;
					case MYSQL_TYPE_DOUBLE		: strcpy(fieldtype, "MYSQL_DOUBLE");		break;
					case MYSQL_TYPE_NULL		: strcpy(fieldtype, "MYSQL_NULL");			break;
					case MYSQL_TYPE_TIMESTAMP	: strcpy(fieldtype, "MYSQL_TIMESTAMP");		break;
					case MYSQL_TYPE_LONGLONG	: strcpy(fieldtype, "MYSQL_LONGLONG");		break;
					case MYSQL_TYPE_INT24		: strcpy(fieldtype, "MYSQL_INT24");			break;
					case MYSQL_TYPE_DATE		: strcpy(fieldtype, "MYSQL_DATE");			break;
					case MYSQL_TYPE_TIME		: strcpy(fieldtype, "MYSQL_TIME");			break;
					case MYSQL_TYPE_DATETIME	: strcpy(fieldtype, "MYSQL_DATETIME");		break;
					case MYSQL_TYPE_YEAR		: strcpy(fieldtype, "MYSQL_YEAR");			break;
					case MYSQL_TYPE_NEWDATE		: strcpy(fieldtype, "MYSQL_NEWDATE");		break;
					case MYSQL_TYPE_VARCHAR		: strcpy(fieldtype, "MYSQL_VARCHAR");		break;
					case MYSQL_TYPE_BIT			: strcpy(fieldtype, "MYSQL_BIT");			break;
					case MYSQL_TYPE_NEWDECIMAL	: strcpy(fieldtype, "MYSQL_NEWDECIMAL");	break;
					case MYSQL_TYPE_ENUM		: strcpy(fieldtype, "MYSQL_ENUM");			break;
					case MYSQL_TYPE_SET			: strcpy(fieldtype, "MYSQL_SET");			break;
					case MYSQL_TYPE_TINY_BLOB	: strcpy(fieldtype, "MYSQL_TINY_BLOB");		break;
					case MYSQL_TYPE_MEDIUM_BLOB	: strcpy(fieldtype, "MYSQL_MEDIUM_BLOB");	break;
					case MYSQL_TYPE_LONG_BLOB	: strcpy(fieldtype, "MYSQL_LONG_BLOB");		break;
					case MYSQL_TYPE_BLOB		: strcpy(fieldtype, "MYSQL_BLOB");			break;
					case MYSQL_TYPE_VAR_STRING	: strcpy(fieldtype, "MYSQL_VAR_STRING");	break;
					case MYSQL_TYPE_STRING		: strcpy(fieldtype, "MYSQL_STRING");		break;
					case MYSQL_TYPE_GEOMETRY	: strcpy(fieldtype, "MYSQL_GEOMETRY");		break;
					default						: strcpy(fieldtype, "MYSQL_UNKNOWN");		break;
				}

				// Copy results to output
				// usertype and locale currently not used
				printf("%d%s%s%s%d%s%s%s%d%s%s%d%s%s%s%s%s"
						, qdata->describe_id
						, qdata->fsep
						, field->name
						, qdata->fsep
						, field->type + 20000
						, qdata->fsep
						, fieldtype
						, qdata->fsep
						, (int)field->length
						, qdata->fsep
						, qdata->fsep
						, (int)field->decimals
						, qdata->fsep
						, field->flags & NOT_NULL_FLAG ? "Y" : "N"
						, qdata->fsep
						, qdata->sql
						, qdata->lsep
						);
			}
		}

		qdata->hdr_printed = true;
	}
}

static bool eto_execute_query(QUERYDATA *qdata, MYSQL *conn)
{
	MYSQL_RES *result;

	// Execute query
	if (mysql_query(conn, qdata->sql))
	{
		eto_log_add_entry_s("ETO-10221 Execute query: prepare query failed: ", mysql_error(conn) );
		return false;
	}

	// Start fetch
	result = mysql_use_result(conn);
	if (result == NULL)
	{
		eto_log_add_entry_s("ETO-10327 Execute query: Unknown error", mysql_error(conn) );
		mysql_free_result(result);
		return false;
	}

	// Print headers/describe
	eto_describe_output(qdata, result);

	// Print rows
	if (!qdata->describe_id)
		eto_write_output(qdata, result);

	// Report reason of end of datafetch
	if (mysql_errno(conn) )
		eto_log_add_entry_s("ETO-10246 Queryresults:Unknown error: ", mysql_error(conn) );

	// Free results
	mysql_free_result(result);

	return true;
}

bool eto_open_database(ETO_PARAMS* params)
{
	QUERYDATA		*qdata;
	MYSQL			*conn;

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

	// Set default port
	if (params->prt < 1 || params->prt > 65535)
		params->prt = 3306;

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
	if (!eto_create_query(&qdata->sql, params->sql, params->tbl, false /* is_describe: always false for MySQL */, true) )
	{
		// Log messages added by eto_create_query
		return false;
	}

	// Init MySQL client library
	conn = mysql_init(NULL);

	if (!conn)
	{
		eto_log_add_entry_s("ETO-10202 Open database: Connection allocation failed. MySQL client version:", mysql_get_client_info() );
		return false;
	}

	// Login to database
	if (mysql_real_connect(conn, params->dbs, params->usr, params->pwd, NULL, params->prt, NULL, 0) == NULL)
	{
		eto_log_add_entry_s("ETO-10206 Open database: Create connection failed:", mysql_error(conn));
		return false;
	}

	if (mysql_set_character_set(conn, "utf8"))
	{
		eto_log_add_entry_s("ETO-10207 Open database: Could not set UTF-8 character set. Default is:", mysql_character_set_name(conn) );
	}

	// Execute query
	eto_execute_query(qdata, conn);

	// Cleanup
	mysql_close(conn);

	eto_delete_querydata(qdata);

	return true;
}
