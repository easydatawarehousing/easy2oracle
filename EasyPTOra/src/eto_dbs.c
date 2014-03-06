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
#include "prestoclient.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

static void eto_describe_callback_function(void *in_querydata, void *in_result)
{
	unsigned int		 i, columncount;
	QUERYDATA			*qdata  = (QUERYDATA*)in_querydata;
	PRESTOCLIENT_RESULT	*result = (PRESTOCLIENT_RESULT*)in_result;

	columncount = prestoclient_getcolumncount(result);

	// Output header
	if (qdata->hdr && !qdata->hdr_printed && columncount > 0)
	{
		if (!qdata->is_describe)
		{
		//	printf("%c%c%c", 239, 187, 191);

			for (i = 0; i < columncount; i++)
				printf("%s%s", i > 0 ? qdata->fsep : "", prestoclient_getcolumnname(result, i) );

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
				printf("%d%s%s%s%d%s%s%s%d%s%s%sN%s%s"
						, qdata->describe_id
						, qdata->fsep
						, prestoclient_getcolumnname(result, i)
						, qdata->fsep
						, prestoclient_getcolumntype(result, i) + 60000
						, qdata->fsep
						, prestoclient_getcolumntypedescription(result, i)
						, qdata->fsep
						, prestoclient_getcolumntype(result, i) == PRESTOCLIENT_TYPE_VARCHAR ? qdata->vml : 0
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

static void eto_write_callback_function(void *in_querydata, void *in_result)
{
	unsigned int		 i, columncount, curr_cache_len, newdatalen;
	QUERYDATA			*qdata  = (QUERYDATA*)in_querydata;
	PRESTOCLIENT_RESULT	*result = (PRESTOCLIENT_RESULT*)in_result;

	assert(in_querydata);
	assert(in_result);

	columncount = prestoclient_getcolumncount(result);

	// Output data row
	for (i = 0; i < columncount; i++)
	{
		// Check for NULL
		if (prestoclient_getnullcolumnvalue(result, i) )
			newdatalen = 1;	// Just a separator
		else
			newdatalen = strlen(prestoclient_getcolumndata(result, i) );

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

		strncat(qdata->cache, prestoclient_getcolumndata(result, i), newdatalen);

		if (i < columncount - 1)
			strcat(qdata->cache, qdata->fsep);
	}

	strcat(qdata->cache, qdata->lsep);
	qdata->rows_processed++;

	// print rows to output
	if (qdata->rows_processed == qdata->cache_rows)
	{
		qdata->rows_processed = 0;
		printf("%s", qdata->cache);
		qdata->cache[0] = 0;
	}
}

bool eto_open_database(ETO_PARAMS* params)
{
	QUERYDATA			*qdata;
	PRESTOCLIENT		*pc;
	PRESTOCLIENT_RESULT	*result;
	bool				 status = true;

	// Check parameters
	if (!params || params->dbs == NULL)
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
		params->prt = 8080;

	// Set up data
	qdata = eto_new_querydata(params->sql, params->tbl);

	if (!qdata)
	{
		// Log messages added by eto_new_querydata
		return false;
	}

	// Set up data for callback function
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
	if (!eto_create_query(&qdata->sql, params->sql, params->tbl, qdata->is_describe, true) )
	{
		// Log messages added by eto_create_query
		return false;
	}

	// Initialize prestoclient
	pc = prestoclient_init(params->dbs, params->prt > 0 ? (unsigned int*)&params->prt : NULL, params->cat, params->usr, params->pwd);

	if (!pc)
	{
		eto_log_add_entry("ETO-10202 Open database: could not attach to presto server");
		return false;
	}

	result = prestoclient_query(pc, qdata->sql, params->scm, &eto_write_callback_function, &eto_describe_callback_function, (void*)qdata );

	if (!result)
	{
		eto_log_add_entry("ETO-10202 Open database: could not start query");
		return false;
	}

	// Print any remaining output
	if (qdata->cache && strlen(qdata->cache) > 0)
		printf("%s", qdata->cache);

	// Add any errormessages to the logfile
	if (result)
	{
		if (prestoclient_getlastservererror(result) )
		{
			eto_log_add_entry_s("ETO-10202 Open database:", prestoclient_getlastservererror(result) );
		//	eto_log_add_entry_s("ETO-10202 Open database: Serverstate =", prestoclient_getlastserverstate(result) );
			status = false;
		}

		if (prestoclient_getlastclienterror(result) )
		{
			eto_log_add_entry_s("ETO-10202 Open database:", prestoclient_getlastclienterror(result) );
			status = false;
		}

		if (prestoclient_getlastcurlerror(result) )
		{
			eto_log_add_entry_s("ETO-10202 Open database:", prestoclient_getlastcurlerror(result) );
		}

	}

	// Cleanup
	prestoclient_close(pc);

	eto_delete_querydata(qdata);

	return status;
}