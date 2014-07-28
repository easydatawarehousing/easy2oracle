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
#include <oci.h>

#define MAX_FIELDTYPE_LENGTH  30
#define DATETIMEFIELD_LENGTH  40
#define UTF8_LANGUAGECODE    873

// Describe info
typedef struct ST_ETO_ORA_COLUMNINFO
{
	char				*fieldname;
	ub2					 oracletype;
	char				*fieldtype;
	ub2					 size;
	ub1					 precision;
	sb1					 scale;
	ub1					 notnull;
	ub4					 value_buffer_size;
} ETO_ORA_COLUMNINFO;

static ETO_ORA_COLUMNINFO* eto_new_columninfo()
{
	ETO_ORA_COLUMNINFO* column = (ETO_ORA_COLUMNINFO*)malloc( sizeof(ETO_ORA_COLUMNINFO) );
	if (!column)
		exit(1);

	column->fieldname			= NULL;
	column->oracletype			= 0;
	column->fieldtype			= NULL;
	column->size				= 0;
	column->precision			= 0;
	column->scale				= 0;
	column->notnull				= 0;
	column->value_buffer_size	= 0;

	return column;
}

static void eto_delete_columninfo(ETO_ORA_COLUMNINFO* column)
{
	if (!column)
		return;

	if (column->fieldname)
		free(column->fieldname);

	if (column->fieldtype)
		free(column->fieldtype);

	free(column);
}

static bool eto_continue(OCIError *error, sword status, char* errormessage)
{
	text message[512];
	sb4 errcode = 0;

	// Only check when we actually want to report something
	if (!errormessage)
		return true;

	// See page: OCI Programming Basics 2-21
	switch (status)
	{
		case OCI_SUCCESS_WITH_INFO:
			OCIErrorGet( (dvoid *)error, (ub4)1, (text *)NULL, &errcode, message, (ub4)sizeof(message), OCI_HTYPE_ERROR);
			eto_log_add_entry_s(errormessage, message);
			return true;

		case OCI_NEED_DATA:
			eto_log_add_entry_s(errormessage, "no runtime data available");
			return false;

		case OCI_NO_DATA:
			eto_log_add_entry_s(errormessage, "no (further) data available");
			return false;

		case OCI_ERROR:
			OCIErrorGet( (dvoid *)error, (ub4)1, (text *)NULL, &errcode, message, (ub4)sizeof(message), OCI_HTYPE_ERROR);
			eto_log_add_entry_s(errormessage, message);
			return false;

		case OCI_INVALID_HANDLE:
			eto_log_add_entry_s(errormessage, "Invalid handle used");
			return false;

		case OCI_STILL_EXECUTING:
			eto_log_add_entry_s(errormessage, "nonblocking mode, current operation could not be completed immediately");
			return true;

		case OCI_CONTINUE:
			eto_log_add_entry_s(errormessage, "Callback completed");
			return true;

		default:
			return true;
	}

	return false;
}

typedef struct ST_ETO_ORA_CONNECTION
{
	// Oracle handles
	OCIEnv					*environment;
	OCIError				*error;
	OCISvcCtx				*servercontext;
	OCIStmt					*stmt;

	// Statement results
	ub4						 numcolumns;
	ETO_ORA_COLUMNINFO	   **columninfo;
	char				   **value;
	OCIDefine			   **defs;
	sb2					   **indicator;
	ub2					   **valuelength;
} ETO_ORA_CONNECTION;

static ETO_ORA_CONNECTION* eto_new_connection()
{
	ETO_ORA_CONNECTION* conn = (ETO_ORA_CONNECTION*)malloc( sizeof(ETO_ORA_CONNECTION) );
	if (!conn)
		exit(1);

	conn->environment	= NULL;
	conn->error			= NULL;
	conn->servercontext	= NULL;
	conn->stmt			= NULL;
	conn->numcolumns	= 0;
	conn->columninfo	= NULL;
	conn->value			= NULL;
	conn->defs			= NULL;
	conn->indicator		= NULL;
	conn->valuelength	= NULL;

	return conn;
}

static void eto_delete_connection(ETO_ORA_CONNECTION* conn)
{
	ub4 i;

	if (!conn)
		return;

	// Delete arrays
	if (conn->numcolumns && conn->columninfo)
	{
		for (i = 0; i < conn->numcolumns; i++)
		{
			eto_delete_columninfo(conn->columninfo[i]);
			
			if (conn->value && conn->value[i])
				free(conn->value[i]);
	
			//free(conn->defs[i]);

			if (conn->indicator && conn->indicator[i])
				free(conn->indicator[i]);

			if (conn->valuelength && conn->valuelength[i])
				free(conn->valuelength[i]);
		}

		free(conn->columninfo);

		if (conn->value)
			free(conn->value);

		//free(conn->defs);

		if(conn->indicator)
			free(conn->indicator);

		if(conn->valuelength)
			free(conn->valuelength);
	}

	// Logout
	if (conn->servercontext)
	{
		eto_continue(conn->error, OCILogoff(conn->servercontext, conn->error), NULL);
	}

	// Free all handles
	if (conn->environment)
	{
		OCIHandleFree( (dvoid *) conn->stmt,			OCI_HTYPE_STMT);
		OCIHandleFree( (dvoid *) conn->servercontext,	OCI_HTYPE_SVCCTX);
		OCIHandleFree( (dvoid *) conn->error,			OCI_HTYPE_ERROR);
		OCIHandleFree( (dvoid *) conn->environment,		OCI_HTYPE_ENV);
	}

	free(conn);
}

static OraText* eto_oracle_connectstring(char* dbname, int port)
{
	int i, colonpos, slashpos, servernamelength;
	bool copy_port = true;
	OraText	*connectstring;

	// Create connect string (like: host:1521/XE) from databasename and/or port
	// Note that a servicename is optional
	i = strlen(dbname) + 6 /* port=5 + terminator=1 */;
	connectstring = (OraText*)calloc(i, sizeof(OraText) );
	if (!connectstring)
		exit(1);
	memset(connectstring, 0, i);

	// Port given ?
	if (port < 1 || port > 65535)
	{
		// No port specified => use databasename as is (may contain a port/servicename)
		strcpy(connectstring, dbname);
		return connectstring;
	}

	// Modify connectstring to add port

	// Check if tcp port and/or servicename is included in the database string
	colonpos = -1;
	slashpos = -1;
	
	for (i = 0; i < (int)strlen(dbname); i++)
	{
		if (strncmp(dbname + i, ":", 1) == 0)
		{
			colonpos = i;
			break;
		}
	}

	for (i = colonpos > 0 ? colonpos + 1 : 0; i < (int)strlen(dbname); i++)
	{
		if (strncmp(dbname + i, "/", 1) == 0)
		{
			slashpos = i;
			break;
		}
	}

	// Copy servername
	if (colonpos > 0)
		servernamelength = colonpos;
	else if (slashpos > 0)
		servernamelength = slashpos;
	else
		servernamelength = strlen(dbname);

	// Create connectstring
	sprintf(connectstring
		, "%.*s:%d%s"
		, servernamelength, dbname
		, port
		, slashpos > 0 ? dbname + slashpos : ""
		);

	return connectstring;
}

static bool eto_oracle_connect(ETO_ORA_CONNECTION* conn, ETO_PARAMS* params)
{
	ub2		 languagecode;
	OraText	 languagename[65];
	OraText	*connectstring = NULL;
	OraText	*setdateformatsql = "ALTER SESSION SET NLS_DATE_FORMAT='yyyy-mm-dd hh24:mi:ss' NLS_TIMESTAMP_FORMAT='yyyy-mm-dd hh24:mi:ss.ff'";

	// Create environment
	sword errorcode = OCIEnvNlsCreate(
		(OCIEnv **)&conn->environment,
		(ub4)OCI_DEFAULT,
		(dvoid *)0,
		(dvoid * (*)(dvoid *,size_t) ) 0,
		(dvoid * (*)(dvoid *, dvoid *, size_t) ) 0,
		(void (*)(dvoid *, dvoid *) ) 0,
		(size_t)0,
		(dvoid **) 0,
		UTF8_LANGUAGECODE,
		UTF8_LANGUAGECODE
	);

	if (errorcode != 0)
	{
		eto_log_add_entry_i("ETO-10202 Open database: Create Oracle environment failed with errorcode =", errorcode);
		return false;
	}

	// Create error handle
	OCIHandleAlloc( (dvoid *) conn->environment, (dvoid **) &conn->error, OCI_HTYPE_ERROR, (size_t) 0, (dvoid **) 0);

	// Check characterset
	languagecode = OCINlsCharSetNameToId( conn->environment, (text*)"AL32UTF8" );
	if (languagecode != UTF8_LANGUAGECODE)
	{
		// Add a warning to the log
		OCINlsGetInfo(conn->environment, conn->error, languagename, 64, OCI_NLS_CHARACTER_SET);
		eto_log_add_entry_s("ETO-10202 Open database: Warning: characterset could not be set to AL32UTF8, using:", languagename);
	}

	// Construct connectstring
	connectstring = eto_oracle_connectstring(params->dbs, params->prt);

	// Connect
	if (!eto_continue(
		conn->error,
		OCILogon2(
			conn->environment, 
			conn->error,
			&conn->servercontext,
			(text*)params->usr,
			(ub4)strlen(params->usr),
			(text*)params->pwd,
			(ub4)strlen(params->pwd),
			(text*)connectstring,
			(ub4)strlen(connectstring),
			OCI_DEFAULT
		),
		"ETO-10206 Open database: Connect failed:") )
	{
		eto_log_add_entry_s("ETO-10206 Open database: Tried to connect to:", connectstring);
		free(connectstring);
		return false;
	}

	free(connectstring);

	// Init statement handle
	if (!eto_continue(
		conn->error,
		OCIHandleAlloc( (dvoid *)conn->environment, (dvoid **)&conn->stmt, OCI_HTYPE_STMT, (size_t)0, (dvoid **)0),
		"ETO-10207 Open database: prepare statement handle failed:") )
			return false;

	// Set date format
	if (!eto_continue(
		conn->error,
		OCIStmtPrepare(conn->stmt, conn->error, setdateformatsql, (ub4)strlen(setdateformatsql), (ub4)OCI_NTV_SYNTAX, (ub4)OCI_DEFAULT),
		"ETO-10224 Execute query: prepare dateformat query failed:") )
			return false;

	if(!eto_continue(
		conn->error,
		OCIStmtExecute(conn->servercontext, conn->stmt, conn->error, 1, 0, (OCISnapshot *)0, (OCISnapshot *)0, OCI_DEFAULT),
		"ETO-10225 Execute query: query dateformat execution failed:") )
			return false;

	return true;
}

static void eto_write_output(QUERYDATA *qdata, ETO_ORA_CONNECTION* conn)
{
	unsigned int	 i, newdatalen, lseplen, curr_cache_len = 0;
	ub4				 r, fetched_rows;
	sword			 status;
	bool			 continue_fetch = true;
	sb4				 error_code = 0;
	text			 error_message[10];

	if (qdata->is_describe || !conn->numcolumns)
		return;

	lseplen = strlen(qdata->lsep);
	
	while(true)
	{
		if( (status = OCIStmtFetch2(conn->stmt, conn->error, qdata->cache_rows, OCI_DEFAULT, 0, OCI_DEFAULT) ) != OCI_SUCCESS)
		{
			OCIErrorGet( (dvoid *)conn->error, (ub4)1, (text *)NULL, &error_code, error_message, 10, OCI_HTYPE_ERROR);

			// Check if this is the last fetch (with less rows than requested)
			if (status == OCI_NO_DATA && error_code == 1403 /* No Data Found */)
				continue_fetch = false;
			else
			{
				// Log the error and quit
				eto_continue(conn->error, status, "ETO-10243 Queryresults: Fetch next record failed:");
				break;
			}
		}

		// Get actual number of rows fetched
		if (!eto_continue(
			conn->error,
			OCIAttrGet((void *)conn->stmt, OCI_HTYPE_STMT, (void *)&fetched_rows, (ub4 *)0, OCI_ATTR_ROWS_FETCHED, conn->error),
			"ETO-10245 Queryresults: get row count failed:") )
				return;

		// Process rows
		for(r = 0; r < fetched_rows; r++)
		{
			for(i = 0; i < conn->numcolumns; i++)
			{
				// Get value length
				newdatalen = conn->valuelength[i][r];
				if (!newdatalen)
					newdatalen = 1;	// Just a separator

				// Cut off at 4000 characters (shouldn't happen because maxlength was passed to oracle when binding)
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

				// Add value to cache if it isn't NULL
				// See: Indicator variables: Oracle Call Interface Programmer's Guide, page 2-24
				if (conn->indicator[i][r] != -1)
					strncat(qdata->cache, conn->value[i] + r * (conn->columninfo[i]->size + 1), newdatalen);

				curr_cache_len += newdatalen;

				if (i < conn->numcolumns - 1)
				{
					strcat(qdata->cache, qdata->fsep);
					curr_cache_len++;
				}
			}

			strcat(qdata->cache, qdata->lsep);
			curr_cache_len += lseplen;

		}

		// Print rows to output
		printf("%s", qdata->cache);
		qdata->cache[0] = 0;
		curr_cache_len = 0;

		// Quit when last iteration found
		if (!continue_fetch)
			break;
	}
}

static bool eto_oracle_is_datetime(ub2 oracle_type)
{
	if (oracle_type == OCI_TYPECODE_DATE ||
		oracle_type == OCI_TYPECODE_TIME ||
		oracle_type == OCI_TYPECODE_TIME_TZ ||
		oracle_type == OCI_TYPECODE_TIMESTAMP ||
		oracle_type == OCI_TYPECODE_TIMESTAMP_TZ ||
		oracle_type == OCI_TYPECODE_TIMESTAMP_LTZ ||
		oracle_type == OCI_TYPECODE_INTERVAL_YM ||
		oracle_type == OCI_TYPECODE_INTERVAL_DS )
			return true;

	return false;
}

static bool eto_describe_output(QUERYDATA *qdata, ETO_ORA_CONNECTION* conn)
{
	ub4				 i, fieldname_length;
	ub2				 type;
	OCIParam		*column = NULL;
	text			*pfieldname;
	ub2				 col_width;

	// Get the number of columns in the query
	if (!eto_continue(
		conn->error,
		OCIAttrGet((void *)conn->stmt, OCI_HTYPE_STMT, (void *)&conn->numcolumns, (ub4 *)0, OCI_ATTR_PARAM_COUNT, conn->error),
		"ETO-10240 Queryresults: get column count failed:") )
			return false;

	if (conn->numcolumns == 0)
		return false;


	// Allocate columninfo
	conn->columninfo = (ETO_ORA_COLUMNINFO**)calloc(conn->numcolumns, sizeof(ETO_ORA_COLUMNINFO*) );
	if (!conn->columninfo)
		return false;

	for (i = 0; i < conn->numcolumns; i++)
		conn->columninfo[i] = NULL;


	// Copy header record to output
	if (qdata->is_describe)
		printf("%s", "describe_id;name;datatype;datatype_descr;maxlength;scale;precision;notnull;statement\n");


	// Get describe info for each column
	for (i = 0; i < conn->numcolumns; i++)
	{
		// Allocate columninfo
		conn->columninfo[i] = eto_new_columninfo();


		// Get column handle. See: Oracle Call Interface Programmer's Guide, page 6-12
		if (!eto_continue(
			conn->error,
			OCIParamGet( (void *)conn->stmt, OCI_HTYPE_STMT, conn->error, (void **)&column, i+1 /* plus one ! */),
			"ETO-10240 Queryresults: Error getting columnhandle:"
			) )
				return false;

		// Get fieldtype
		type = 0;
		if (!eto_continue(
			conn->error,
			OCIAttrGet( (void *)column, OCI_DTYPE_PARAM, (void *)&conn->columninfo[i]->oracletype, (ub4 *)0, OCI_ATTR_DATA_TYPE, conn->error),
			"ETO-10240 Queryresults: Error getting column type:") )
				return false;

		// Get fieldname
		fieldname_length = 0;
		if (!eto_continue(
			conn->error,
			OCIAttrGet( (void *)column, OCI_DTYPE_PARAM, (void**)&pfieldname, (ub4 *)&fieldname_length, OCI_ATTR_NAME, conn->error),
			"ETO-10240 Queryresults: Error getting column name:") )
				return false;

		conn->columninfo[i]->fieldname = (char*)calloc(fieldname_length + 1, sizeof(char) );
		if (!conn->columninfo[i]->fieldname)
			exit(1);
		memset(conn->columninfo[i]->fieldname, 0, fieldname_length + 1);
		strncpy(conn->columninfo[i]->fieldname, pfieldname, fieldname_length);

		// Get fieldlength
		if (!eto_continue(
			conn->error,
			OCIAttrGet( (void *)column, OCI_DTYPE_PARAM, (void *)&col_width, (ub4 *)0, OCI_ATTR_CHAR_SIZE, conn->error),
			"ETO-10240 Queryresults: Error getting field length:") )
				return false;

		if (!eto_continue(
			conn->error,
			OCIAttrGet( (void *)column, OCI_DTYPE_PARAM, (void *)&conn->columninfo[i]->size, (ub4 *)0, OCI_ATTR_DATA_SIZE, conn->error),
			"ETO-10240 Queryresults: Error getting column length:") )
				return false;

		// Adjust length for date/time types
		if (eto_oracle_is_datetime(conn->columninfo[i]->oracletype) )
			conn->columninfo[i]->size = DATETIMEFIELD_LENGTH;

		// Get precision
		if (!eto_continue(
			conn->error,
			OCIAttrGet( (void *)column, OCI_DTYPE_PARAM, (void *)&conn->columninfo[i]->precision, (ub4 *)0, OCI_ATTR_PRECISION, conn->error),
			"ETO-10240 Queryresults: Error getting column precision:") )
				return false;

		// Get scale
		if (!eto_continue(
			conn->error,
			OCIAttrGet( (void *)column, OCI_DTYPE_PARAM, (void *)&conn->columninfo[i]->scale, (ub4 *)0, OCI_ATTR_SCALE, conn->error),
			"ETO-10240 Queryresults: Error getting column scale:") )
				return false;

		// Get not null
		if (!eto_continue(
			conn->error,
			OCIAttrGet( (void *)column, OCI_DTYPE_PARAM, (void *)&conn->columninfo[i]->notnull, (ub4 *)0, OCI_ATTR_IS_NULL, conn->error),
			"ETO-10240 Queryresults: Error getting column nullable:") )
				return false;

		// Allocate fieldtype
		conn->columninfo[i]->fieldtype = (char*)calloc(MAX_FIELDTYPE_LENGTH + 1, sizeof(char) );
		if (!conn->columninfo[i]->fieldtype)
			exit(1);
		memset(conn->columninfo[i]->fieldtype, 0, MAX_FIELDTYPE_LENGTH + 1);

		// Describe column
		switch (type)
		{
			case OCI_TYPECODE_REF:
			case OCI_TYPECODE_PTR:
			case OCI_TYPECODE_MLSLABEL:
			case OCI_TYPECODE_VARRAY:
			case OCI_TYPECODE_TABLE:
			case OCI_TYPECODE_OBJECT:
			case OCI_TYPECODE_OPAQUE:
			case OCI_TYPECODE_NAMEDCOLLECTION:
			case OCI_TYPECODE_BLOB:
			case OCI_TYPECODE_BFILE:
			case OCI_TYPECODE_CLOB:
			case OCI_TYPECODE_CFILE:
			case OCI_TYPECODE_OTMFIRST:
			case OCI_TYPECODE_OTMLAST:
		//	case OCI_TYPECODE_SYSFIRST:
			case OCI_TYPECODE_SYSLAST:
			case OCI_TYPECODE_NCLOB:
			case OCI_TYPECODE_RAW:
				strcpy(conn->columninfo[i]->fieldtype, "ORACLE_RAW");
				type = OCI_TYPECODE_RAW;
				break;

			case OCI_TYPECODE_SIGNED8:
			case OCI_TYPECODE_SIGNED16:
			case OCI_TYPECODE_SIGNED32:
			case OCI_TYPECODE_REAL:
			case OCI_TYPECODE_DOUBLE:
			case OCI_TYPECODE_BFLOAT:
			case OCI_TYPECODE_BDOUBLE:
			case OCI_TYPECODE_FLOAT:
			case OCI_TYPECODE_NUMBER:
			case OCI_TYPECODE_DECIMAL:
			case OCI_TYPECODE_UNSIGNED8:
			case OCI_TYPECODE_UNSIGNED16:
			case OCI_TYPECODE_UNSIGNED32:
			case OCI_TYPECODE_OCTET:
			case OCI_TYPECODE_SMALLINT:
			case OCI_TYPECODE_INTEGER:
			case OCI_TYPECODE_PLS_INTEGER:
				strcpy(conn->columninfo[i]->fieldtype, "ORACLE_NUMBER");
				type = OCI_TYPECODE_NUMBER;
				break;

			case OCI_TYPECODE_UROWID:
			case OCI_TYPECODE_VARCHAR2:
			case OCI_TYPECODE_CHAR:
			case OCI_TYPECODE_VARCHAR:
			case OCI_TYPECODE_NCHAR:
			case OCI_TYPECODE_NVARCHAR2:
			case OCI_TYPECODE_NONE:
			{
				strcpy(conn->columninfo[i]->fieldtype, "ORACLE_VARCHAR2");
				type = OCI_TYPECODE_VARCHAR2;

				// Take the largest of width/size. For strings this should give the size in bytes (not chars)
				if (col_width > conn->columninfo[i]->size)
					conn->columninfo[i]->size = col_width;

				break;
			}

			case OCI_TYPECODE_DATE:				strcpy(conn->columninfo[i]->fieldtype, "ORACLE_DATE");			break;
			case OCI_TYPECODE_TIME:				strcpy(conn->columninfo[i]->fieldtype, "ORACLE_TIME");			break;
			case OCI_TYPECODE_TIME_TZ:			strcpy(conn->columninfo[i]->fieldtype, "ORACLE_TIME_TZ");		break;
			case OCI_TYPECODE_TIMESTAMP:		strcpy(conn->columninfo[i]->fieldtype, "ORACLE_TIMESTAMP");		break;
			case OCI_TYPECODE_TIMESTAMP_TZ:		strcpy(conn->columninfo[i]->fieldtype, "ORACLE_TIMESTAMP_TZ");	break;
			case OCI_TYPECODE_TIMESTAMP_LTZ:	strcpy(conn->columninfo[i]->fieldtype, "ORACLE_TIMESTAMP_LTZ");	break;
			case OCI_TYPECODE_INTERVAL_YM:		strcpy(conn->columninfo[i]->fieldtype, "ORACLE_INTERVAL_YM");	break;
			case OCI_TYPECODE_INTERVAL_DS:		strcpy(conn->columninfo[i]->fieldtype, "ORACLE_INTERVAL_DS");	break;

			default:
				strcpy(conn->columninfo[i]->fieldtype, "ORACLE_VARCHAR2");
				type = OCI_TYPECODE_VARCHAR2;
				break;
		}

		// Adjust fieldsize to fit maximum allowed width
		if (conn->columninfo[i]->size > qdata->vml)
			conn->columninfo[i]->size = qdata->vml;

		if (qdata->is_describe)
		{
			// Copy description to output
			printf("%d%s%s%s%d%s%s%s%d%s%d%s%d%s%s%s%s%s"
					, qdata->describe_id
					, qdata->fsep
					, conn->columninfo[i]->fieldname
					, qdata->fsep
					, conn->columninfo[i]->oracletype + 70000
					, qdata->fsep
					, conn->columninfo[i]->fieldtype
					, qdata->fsep
					, conn->columninfo[i]->size
					, qdata->fsep
					, conn->columninfo[i]->scale
					, qdata->fsep
					, conn->columninfo[i]->precision
					, qdata->fsep
					, conn->columninfo[i]->notnull ? "Y" : "N"
					, qdata->fsep
					, qdata->sql
					, qdata->lsep
					);
		}
		else
		{
			// Copy fieldnames to output
			if (qdata->hdr && !qdata->hdr_printed)
				printf("%s%s", i > 0 ? qdata->fsep : "", conn->columninfo[i]->fieldname);
		}
	}

	if (!qdata->is_describe && qdata->hdr)
		printf("%s", qdata->lsep);

	qdata->hdr_printed = true;

	return true;
}

static bool eto_bind_output(QUERYDATA *qdata, ETO_ORA_CONNECTION *conn)
{
	ub4				 i;

	// Allocate define buffer
	conn->defs = (OCIDefine**)calloc(conn->numcolumns, sizeof(OCIDefine*) );
	if (!conn->defs)
		exit(1);

	// Allocate value buffer: per column there is one char buffer that holds all field data
	// for 'qdata->cache_rows' rows.
	// Skipsize is thus: conn->columninfo[i]->size+1
	// See: Binding and Defining in OCI page 5-17
	conn->value = (char**)calloc(conn->numcolumns, sizeof(char*) );
	if (!conn->value)
		exit(1);

	for (i = 0; i < conn->numcolumns; i++)
	{
		conn->columninfo[i]->value_buffer_size = qdata->cache_rows * (conn->columninfo[i]->size+1) * sizeof(char);

		conn->value[i] = (char*)malloc(conn->columninfo[i]->value_buffer_size);
		if (!conn->value[i])
			exit(1);

		memset(conn->value[i], 0, conn->columninfo[i]->value_buffer_size);
	}

	// Allocate indicator buffer: 2D array of sb2. Skipsize is: sizeof(sb2)
	conn->indicator = (sb2**)calloc(conn->numcolumns, sizeof(sb2*) );
	if (!conn->indicator)
		exit(1);

	for (i = 0; i < conn->numcolumns; i++)
	{
		conn->indicator[i] = (sb2*)calloc(qdata->cache_rows, sizeof(sb2) );
		if (!conn->indicator[i])
			exit(1);
	}

	// Allocate value-length buffer: 2D array of ub2. Skipsize is: sizeof(ub2)
	conn->valuelength = (ub2**)calloc(conn->numcolumns, sizeof(ub2*) );
	if (!conn->valuelength)
		exit(1);

	for (i = 0; i < conn->numcolumns; i++)
	{
		conn->valuelength[i] = (ub2*)calloc(qdata->cache_rows, sizeof(ub2) );
		if (!conn->valuelength[i])
			exit(1);
	}

	// Bind all columns
	for (i = 0; i < conn->numcolumns; i++)
	{
		conn->defs[i] = NULL;

		// Define (=bind) outputdata
		if (!eto_continue(
			conn->error,
			OCIDefineByPos(
				conn->stmt,
				&conn->defs[i],
				conn->error,
				(ub4)i+1 /* plus one ! */,
				(void *)conn->value[i],	// GCC Warning typeconverion
				(sb4)conn->columninfo[i]->size,
				SQLT_STR,
				(void *)conn->indicator[i],
				(ub2 *)conn->valuelength[i],
				(ub2 *)NULL,
				OCI_DEFAULT
			),
			"ETO-10242 Queryresults: Error binding column:"
			) )
				return false;

		// Define output arrays
		if (!eto_continue(
			conn->error,
			OCIDefineArrayOfStruct(
				conn->defs[i],
				conn->error,
				conn->columninfo[i]->size+1,	// Skipsize for value buffer
				sizeof(sb2),					// Skipsize for indicator buffer
				sizeof(ub2),					// Skipsize for value-length buffer
				0
			),
			"ETO-10242 Queryresults: Error binding column:"
			) )
				return false;

	}

	return true;
}

static bool eto_execute_query(QUERYDATA *qdata, ETO_ORA_CONNECTION *conn)
{
	int i = 0;
	ub2 type = 0;

	// Prepare statement
	if (!eto_continue(
		conn->error,
		OCIStmtPrepare(conn->stmt, conn->error, (OraText *)qdata->sql, (ub4)strlen(qdata->sql), (ub4)OCI_NTV_SYNTAX, (ub4)OCI_DEFAULT),
		"ETO-10221 Execute query: prepare query failed:") )
			return false;

	// set the execution mode to OCI_DESCRIBE_ONLY to describe the request
	if(!eto_continue(
		conn->error,
		OCIStmtExecute(conn->servercontext, conn->stmt, conn->error, 0, 0, (OCISnapshot *)0, (OCISnapshot *)0, OCI_DESCRIBE_ONLY),
		"ETO-10222 Execute query: descripe step failed:") )
			return false;

	// Print headers/describe
	if (!eto_describe_output(qdata, conn) )
		return false;

	// Execute statement again to get query results
	if (!qdata->is_describe)
	{
		if (!eto_bind_output(qdata, conn) )
			return false;

		if(!eto_continue(
			conn->error,
			OCIStmtExecute(conn->servercontext, conn->stmt, conn->error, 0 /* Don't fetch any records yet */, 0, (OCISnapshot *)0, (OCISnapshot *)0, OCI_DEFAULT),
			"ETO-10223 Execute query: query execution failed:") )
				return false;

		// Print rows
		eto_write_output(qdata, conn);
	}

	return true;
}

bool eto_open_database(ETO_PARAMS* params)
{
	QUERYDATA			*qdata;
	ETO_ORA_CONNECTION	*conn = eto_new_connection();

	if (!conn)
		return false;

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
	if (!eto_create_query(&qdata->sql, params->sql, params->tbl, false /* is_describe: always false for Oracle */, false /* No show statements allowed */) )
	{
		// Log messages added by eto_create_query
		return false;
	}

	// Connect to Oracle
	if (!eto_oracle_connect(conn, params) )
	{
		eto_log_add_entry("ETO-10202 Open database: Connect/login failed");
		return false;
	}

	// Execute query
	eto_execute_query(qdata, conn);

	// Cleanup
	eto_delete_connection(conn);
	eto_delete_querydata(qdata);

	return true;
}