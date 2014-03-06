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
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <ctpublic.h>

static bool eto_connect_freetds(const ETO_PARAMS* params, CS_CONTEXT **ctx, CS_CONNECTION **conn, CS_COMMAND **cmd)
{
	// Init FreeTDP library
	if (cs_ctx_alloc(CS_VERSION_150, ctx) != CS_SUCCEED)
	{
		eto_log_add_entry("ETO-10200 Open database: Context allocation failed");
		return false;
	}

	if (ct_init(*ctx, CS_VERSION_150) != CS_SUCCEED)
	{
		eto_log_add_entry("ETO-10201 Open database: Context init failed");
		return false;
	}

	if (ct_con_alloc(*ctx, conn) != CS_SUCCEED)
	{
		eto_log_add_entry("ETO-10202 Open database: Connection allocation failed");
		return false;
	}

	// Setup diagnose
	if (ct_diag(*conn, CS_INIT, CS_ALLMSG_TYPE, 0, NULL) != CS_SUCCEED)
	{
		eto_log_add_entry("ETO-10203 Open database: Setting up diagnose failed");
		return false;
	}

	// Login to database
	if (ct_con_props(*conn, CS_SET, CS_USERNAME, (CS_VOID*)params->usr, CS_NULLTERM, (CS_INT*)NULL) != CS_SUCCEED)
	{
		eto_log_add_entry("ETO-10204 Open database: Setting username failed");
		return false;
	}

	if (ct_con_props(*conn, CS_SET, CS_PASSWORD, (CS_VOID*)params->pwd, CS_NULLTERM, (CS_INT*)NULL) != CS_SUCCEED)
	{
		eto_log_add_entry("ETO-10205 Open database: Setting password failed");
		return false;
	}

	if (ct_connect(*conn, (CS_CHAR*)params->dbs, CS_NULLTERM) != CS_SUCCEED)
	{
		eto_log_add_entry("ETO-10206 Open database: Create connection failed");
		return false;
	}

	if (ct_cmd_alloc(*conn, cmd) != CS_SUCCEED)
	{
		eto_log_add_entry("ETO-10207 Open database: Command allocation failed");
		return false;
	}

	return true;
}

static void eto_write_output(QUERYDATA *qdata, CS_COMMAND *cmd)
{
	CS_INT			fieldcount;
	CS_RETCODE		ret;
	CS_INT			count;
	CS_DATAFMT		datafmt;
//	CS_INT			datalength = 0;
	CS_SMALLINT		*ind = 0;
	CS_CHAR			**fields = 0;
	CS_INT			f, i;
	int 			rows_processed = 0;
	CS_CHAR* 		cache = (CS_CHAR*)malloc(1024 * sizeof(CS_CHAR));
	unsigned int 	cache_size = 1024;
	cache[0] = 0;

	// Determine number of fields
	if (ct_res_info(cmd, CS_NUMDATA, &fieldcount, CS_UNUSED, NULL) != CS_SUCCEED)
	{
		eto_log_add_entry("ETO-10240 Queryresults: Error determining fieldcount");
		return;
	}
	
	ind			= (CS_SMALLINT*)malloc(sizeof(CS_SMALLINT) * (fieldcount + 1));
	fields		= (CS_CHAR**)malloc(sizeof(CS_CHAR*) * (fieldcount + 1));
	
	// Get fieldlist and bind output variables
	for(f = 1; f <= fieldcount; f++)
	{
		fields[f] = (CS_CHAR*)malloc(sizeof(CS_CHAR) * 4096); // 4097 ?
		
		if (fields[f]==NULL)
			exit(1);

		if (ct_describe(cmd, f, &datafmt) != CS_SUCCEED)
			eto_log_add_entry("ETO-10242 Queryresults: Query describe failed");
    
		/* DATETYPE is reported as 8 bytes from server, but becomes 32 when converted. Adjusting for that. */
	//	if (datafmt.maxlength < 32)   datafmt.maxlength = 32;
	//	if (datafmt.maxlength > 4096) datafmt.maxlength = 4096;
		datafmt.maxlength = 4096;

		// Force type to char
		datafmt.format    = CS_FMT_NULLTERM;
		datafmt.datatype  = CS_CHAR_TYPE;

		if (ct_bind(cmd, f, &datafmt, fields[f], NULL /*&datalength*/, &ind[f]) != CS_SUCCEED)
			eto_log_add_entry_i("ETO-10243 Queryresults: Binding failed. Column: ", f);
	
		// Print header row
		if (qdata->hdr)
			printf("%s%s", f > 1 ? qdata->fsep : "", datafmt.name);
	}

	if (qdata->hdr && fieldcount > 0)
		printf("%s", qdata->lsep);

	// Get first row
	ret = ct_fetch(cmd, CS_UNUSED, CS_UNUSED, CS_UNUSED, &count);

	while (ret == CS_SUCCEED)
	{
		// Read row
		for(f = 1; f <= fieldcount; f++)
		{
			if (cache_size < strlen(cache) + strlen( (CS_CHAR*)fields[f]) + 2)
			{
				cache_size += ( (strlen((CS_CHAR*)fields[f]) + 1024) / 1024) * 1024; // add 1 Kb
				cache = (CS_CHAR*)realloc((CS_CHAR*)cache, cache_size);
				if (!cache)
					exit(1);
			}

			if (ind[f] != -1) // not null
			{
				for (i = 0; i < (CS_INT)strlen((CS_CHAR*)fields[f]); i++)
				{
					if (fields[f][i] == 13) fields[f][i] = 32;
					if (fields[f][i] == 10) fields[f][i] = 32;
				}

				strcat(cache, (CS_CHAR*)fields[f]);
			}

			if (f < fieldcount)
				strcat(cache, qdata->fsep);
		}
		strcat(cache, qdata->lsep);

		// print rows to output
		rows_processed++;
		if (rows_processed == qdata->cache_rows)
		{
			rows_processed = 0;
			printf("%s", cache);
			cache[0] = 0;
		}

		// Get next row
		ret = ct_fetch(cmd, CS_UNUSED, CS_UNUSED, CS_UNUSED, &count);
	}

	// print last rows to output
	if (rows_processed > 0)
		printf("%s", cache);

	// Report reason of end of datafetch
	switch ( (int)ret)
	{
		case CS_END_DATA:
			// End of data
		break;
		case CS_ROW_FAIL:
			eto_log_add_entry_i("ETO-10244 Queryresults: Get row failed", (int)ind[f]);
		break;
		case CS_FAIL:
			eto_log_add_entry("ETO-10245 Queryresults: Failed to get result set");
		break;
		default:
			eto_log_add_entry("ETO-10246 Queryresults:Unknown error");
		break;
	}

	// Free memory
	for(f = 1; f <= fieldcount; f++)
		free(fields[f]);

	free(fields);
	free(ind);
	free(cache);
}

static void eto_describe_output(QUERYDATA *qdata, CS_COMMAND *cmd)
{
	CS_INT			fieldcount;
	CS_DATAFMT		datafmt;
	int				i;
	CS_CHAR			fieldtype[15];

	// Determine number of fields
	if (ct_res_info(cmd, CS_NUMDATA, &fieldcount, CS_UNUSED, NULL) != CS_SUCCEED)
	{
		eto_log_add_entry("ETO-10340 Queryresults: Error determining fieldcount");
		return;
	}
	
	// Copy header record to output
	printf("describe_id%sname%sdatatype%sdatatype_descr%smaxlength%sscale%sprecision%snotnull%sstatement%s"
		, qdata->fsep, qdata->fsep, qdata->fsep, qdata->fsep, qdata->fsep, qdata->fsep, qdata->fsep, qdata->fsep
		, qdata->lsep);

	// Get fieldlist
	for(i = 1; i <= fieldcount; i++)
	{
		if (ct_describe(cmd, i, &datafmt) != CS_SUCCEED)
			eto_log_add_entry("ETO-10342 Queryresults: Query describe failed");

		// Describe column
		switch (datafmt.datatype)
		{
			case CS_ILLEGAL_TYPE		: strcpy(fieldtype, "TDP_ILLEGAL");		break;
			case CS_CHAR_TYPE			: strcpy(fieldtype, "TDP_CHAR");		break;
			case CS_BINARY_TYPE			: strcpy(fieldtype, "TDP_BINARY");		break;
			case CS_LONGCHAR_TYPE		: strcpy(fieldtype, "TDP_LONGCHAR");	break;
			case CS_LONGBINARY_TYPE		: strcpy(fieldtype, "TDP_LONGBINARY");	break;
			case CS_TEXT_TYPE			: strcpy(fieldtype, "TDP_TEXT");		break;
			case CS_IMAGE_TYPE			: strcpy(fieldtype, "TDP_IMAGE");		break;
			case CS_TINYINT_TYPE		: strcpy(fieldtype, "TDP_TINYINT");		break;
			case CS_SMALLINT_TYPE		: strcpy(fieldtype, "TDP_SMALLINT");	break;
			case CS_INT_TYPE			: strcpy(fieldtype, "TDP_INT");			break;
			case CS_REAL_TYPE			: strcpy(fieldtype, "TDP_REAL");		break;
			case CS_FLOAT_TYPE			: strcpy(fieldtype, "TDP_FLOAT");		break;
			case CS_BIT_TYPE			: strcpy(fieldtype, "TDP_BIT");			break;
			case CS_DATETIME_TYPE		: strcpy(fieldtype, "TDP_DATETIME");	break;
			case CS_DATETIME4_TYPE		: strcpy(fieldtype, "TDP_DATETIME4");	break;
			case CS_MONEY_TYPE			: strcpy(fieldtype, "TDP_MONEY");		break;
			case CS_MONEY4_TYPE			: strcpy(fieldtype, "TDP_MONEY4");		break;
			case CS_NUMERIC_TYPE		: strcpy(fieldtype, "TDP_NUMERIC");		break;
			case CS_DECIMAL_TYPE		: strcpy(fieldtype, "TDP_DECIMAL");		break;
			case CS_VARCHAR_TYPE		: strcpy(fieldtype, "TDP_VARCHAR");		break;
			case CS_VARBINARY_TYPE		: strcpy(fieldtype, "TDP_VARBINARY");	break;
			case CS_LONG_TYPE			: strcpy(fieldtype, "TDP_LONG");		break;
			case CS_SENSITIVITY_TYPE	: strcpy(fieldtype, "TDP_SENSITIVITY");	break;
			case CS_BOUNDARY_TYPE		: strcpy(fieldtype, "TDP_BOUNDARY");	break;
			case CS_VOID_TYPE			: strcpy(fieldtype, "TDP_VOID");		break;
			case CS_USHORT_TYPE			: strcpy(fieldtype, "TDP_USHORT");		break;
			case CS_UNICHAR_TYPE		: strcpy(fieldtype, "TDP_UNICHAR");		break;
			#ifdef CS_BLOB_TYPE
			case CS_BLOB_TYPE			: strcpy(fieldtype, "TDP_BLOB");		break;
			#endif
			#ifdef CS_DATE_TYPE
			case CS_DATE_TYPE			: strcpy(fieldtype, "TDP_DATE");		break;
			#endif
			#ifdef CS_TIME_TYPE
			case CS_TIME_TYPE			: strcpy(fieldtype, "TDP_TIME");		break;
			#endif
			#ifdef CS_UNITEXT_TYPE
			case CS_UNITEXT_TYPE		: strcpy(fieldtype, "TDP_UNITEXT");		break;
			#endif
			#ifdef CS_BIGINT_TYPE
			case CS_BIGINT_TYPE			: strcpy(fieldtype, "TDP_BIGINT");		break;
			#endif
			#ifdef CS_USMALLINT_TYPE
			case CS_USMALLINT_TYPE		: strcpy(fieldtype, "TDP_USMALLINT");	break;
			#endif
			#ifdef CS_UINT_TYPE
			case CS_UINT_TYPE			: strcpy(fieldtype, "TDP_UINT");		break;
			#endif
			#ifdef CS_UBIGINT_TYPE
			case CS_UBIGINT_TYPE		: strcpy(fieldtype, "TDP_UBIGINT");		break;
			#endif
			#ifdef CS_XML_TYPE
			case CS_XML_TYPE			: strcpy(fieldtype, "TDP_XML");			break;
			#endif

			case CS_UNIQUE_TYPE			: strcpy(fieldtype, "TDP_UNIQUE");		break;
			case CS_USER_TYPE			: strcpy(fieldtype, "TDP_USER");		break;
			default						: strcpy(fieldtype, "TDP_UNKNOWN");		break;
		}

		// Print to output
		// usertype and locale currently not used
		printf("%d%s%s%s%d%s%s%s%d%s%d%s%d%s%s%s%s"
				, qdata->describe_id
				, qdata->fsep
				, datafmt.name
				, qdata->fsep
				, datafmt.datatype + 10000
				, qdata->fsep
				, fieldtype
				, qdata->fsep
				, datafmt.maxlength
				, qdata->fsep
				, datafmt.scale
				, qdata->fsep
				, datafmt.precision
				, qdata->fsep
				, datafmt.status & CS_CANBENULL ? "N" : "Y"
				, qdata->fsep
				, qdata->sql
				);

		if (i < fieldcount)
			printf("%s", qdata->lsep);
	}

	// End datafetch
	if (ct_cancel(NULL, cmd, CS_CANCEL_ALL) != CS_SUCCEED)
	{
		eto_log_add_entry("ETO-10349 Describe: Cancel query failed");
	}
}

static bool eto_execute_query(QUERYDATA *qdata, CS_CONTEXT *ctx, CS_CONNECTION *conn, CS_COMMAND *cmd)
{
	CS_RETCODE	results_ret;
	CS_INT		result_type;
	CS_INT		rowcount;
	CS_INT		i, msg_count;
	CS_SERVERMSG sm;
	CS_CLIENTMSG cm;
	CS_CHAR		message[1024];
	CS_INT		buflen;
    
	assert(qdata);
	assert(ctx);
	assert(conn);
	assert(cmd);

	// todo print header row

	// Execute query
	if (ct_command(cmd, CS_LANG_CMD, (CS_VOID*)qdata->sql, CS_NULLTERM, CS_UNUSED) != CS_SUCCEED)
	{
		eto_log_add_entry("ETO-10221 Execute query: prepare query failed");
		return false;
	}

	if (ct_send(cmd) != CS_SUCCEED)
	{
		eto_log_add_entry("ETO-10222 Execute query: send query failed");
		return false;
	}

	// Get results
	while ((results_ret = ct_results(cmd, &result_type)) == CS_SUCCEED)
	{
		switch ((int) result_type)
		{
			case CS_CMD_SUCCEED:
				if (ct_res_info(cmd, CS_ROW_COUNT, &rowcount, CS_UNUSED, NULL) != CS_SUCCEED)
					eto_log_add_entry("ETO-10223 Execute query: Get row count failed");
			break;
			case CS_CMD_DONE:
				if (ct_res_info(cmd, CS_ROW_COUNT, &rowcount, CS_UNUSED, NULL) != CS_SUCCEED)
					eto_log_add_entry("ETO-10224 Execute query: Get row count failed");
			break;
			case CS_CMD_FAIL:
				// Get error message from database
				if (ct_diag(conn, CS_STATUS, CS_SERVERMSG_TYPE, 0, &msg_count) == CS_SUCCEED)
				{
					for (i = 1; i <= msg_count; i++)
					{
						if (ct_diag(conn, CS_GET, CS_SERVERMSG_TYPE, i, &sm) == CS_SUCCEED)
							eto_log_add_entry_s("SERVERMSG: ", sm.text);
					}
				}

				if (ct_diag(conn, CS_STATUS, CS_CLIENTMSG_TYPE, 0, &msg_count) == CS_SUCCEED)
				{
					for (i = 1; i <= msg_count; i++)
					{
						if (ct_diag(conn, CS_GET, CS_CLIENTMSG_TYPE, i, &cm) == CS_SUCCEED)
							eto_log_add_entry_s("CLIENTMSG: ", cm.msgstring);
					}
				}

				if (ct_config(ctx, CS_GET, CS_VER_STRING, &message, 1024, &buflen) == CS_SUCCEED)
				{
					eto_log_add_entry_s("LIBRARYMSG:", message);
				}

				// Clear message buffer
				ct_diag(conn, CS_CLEAR, CS_ALLMSG_TYPE, 0, NULL);

				// Add generic message
				eto_log_add_entry("ETO-10225 Execute query: Query failed.");
			break;
			case CS_ROW_RESULT:
				// Execute succesful
				if (!qdata->describe_id)
					// Get results
					eto_write_output(qdata, cmd);
				else
					// Describe output and get results
					eto_describe_output(qdata, cmd);
			break;
			case CS_COMPUTE_RESULT:
				eto_log_add_entry("ETO-10226 Execute query: COMPUTE_RESULT error");
			break;
			default:
				eto_log_add_entry("ETO-10227 Execute query: Unknown error");
			break;
		}
	}

	switch ((int) results_ret)
	{
		case CS_END_RESULTS:
		break;
		case CS_FAIL:
			eto_log_add_entry("ETO-10228 Execute query: Query execution failed");
			return false;
		default:
			eto_log_add_entry("ETO-10229 Execute query: Unknown error during execution");
			return false;
    }

	return true;
};

static void eto_close_database(CS_CONTEXT *ctx, CS_CONNECTION *conn, CS_COMMAND *cmd)
{
    if (ct_cancel(conn, NULL, CS_CANCEL_ALL) != CS_SUCCEED)
	{
        eto_log_add_entry("ETO-10250 Close database: Cancel connection failed");
    }
	else
	{
	    ct_cmd_drop(cmd);
		ct_close(conn, CS_UNUSED);
	    ct_con_drop(conn);
	}

    ct_exit(ctx, CS_UNUSED);
    cs_ctx_drop(ctx);
}

bool eto_open_database(ETO_PARAMS* params)
{
	QUERYDATA		*qdata;
	CS_CONTEXT		*ctx = NULL;
	CS_CONNECTION	*conn = NULL;
	CS_COMMAND		*cmd = NULL;

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
		params->prt = 1433;

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
	if (!eto_create_query(&qdata->sql, params->sql, params->tbl, false /* is_describe: always false for freetdp */, false) )	// todo: is_describe ?
	{
		// Log messages added by eto_create_query
		return false;
	}

	// Open database
	if (!eto_connect_freetds(params, &ctx, &conn, &cmd) )
	{
		// Log messages added by eto_connect_freetds
		return false;
	}

	// Execute query
	eto_execute_query(qdata, ctx, conn, cmd);

	// Cleanup
	eto_close_database(ctx, conn, cmd);

	eto_delete_querydata(qdata);

	return true;
}
