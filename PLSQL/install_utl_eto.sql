Rem This file is part of Easy to Oracle - Free Open Source Data Integration

Rem Copyright (C) 2011-2014 Ivo Herweijer
Rem Easy to Oracle is free software: you can redistribute it and/or modify
Rem it under the terms of the GNU General Public License as published by
Rem the Free Software Foundation, either version 3 of the License, or
Rem (at your option) any later version.

Rem This program is distributed in the hope that it will be useful,
Rem but WITHOUT ANY WARRANTY; without even the implied warranty of
Rem MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
Rem GNU General Public License for more details.

Rem You should have received a copy of the GNU General Public License
Rem along with this program.  If not, see <http://www.gnu.org/licenses/>.
Rem You can contact me via email: info@easydatawarehousing.com

Rem Install using SQL*Plus. Login as sysdba.
Rem Optionally change schema from SYS to a user schema

CREATE TABLE SYS.utl_eto$
( ConnectionName  varchar2(4000)
, Parameter       integer
, ParameterValue  varchar2(4000)
);
/


CREATE OR REPLACE PACKAGE SYS.UTL_ETO
AS
/*****************************************************************************
 *
 * COPYRIGHT (C) 2011-2014 Ivo Herweijer
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
 *
 * VERSION DATE        AUTHOR(S)       DESCRIPTION
 * ------- ----------- --------------- ------------------------------------
 * 1.0.0   15-05-2011  Ivo Herweijer   1st release version
 * 1.1.0   15-06-2012  Ivo Herweijer   2nd release. Added MS-Access support
 * 2.0.0   15-02-2014  Ivo Herweijer   3rd release. Added Presto support
 ****************************************************************************/

-- Constants for Describe table --
Eto_descr_tablename          CONSTANT VARCHAR2(30)  := 'UTL_ETO_DESCRIBE$';
Eto_descr_ininame            CONSTANT VARCHAR2(30)  := 'utl_eto_describe.ini';
Eto_descr_logname            CONSTANT VARCHAR2(30)  := 'utl_eto_describe.log';

-- Constants for List_Tables table --
Eto_List_Tables_tablename    CONSTANT VARCHAR2(30)  := 'UTL_ETO_LIST_TABLES$';
Eto_List_Tables_ininame      CONSTANT VARCHAR2(30)  := 'utl_eto_list_tables.ini';
Eto_List_Tables_logname      CONSTANT VARCHAR2(30)  := 'utl_eto_list_tables.log';

-- Constants for Set/Get parameter --
Eto_par_connection           CONSTANT BINARY_INTEGER := 1000;   -- Name of this datasource
Eto_par_executable           CONSTANT BINARY_INTEGER := 1001;   -- Name of EasyXTO executable for this datasource
Eto_par_bin_dir              CONSTANT BINARY_INTEGER := 1002;   -- Name of DIRECTORY defined in Oracle, for location of EasyXTO executables
Eto_par_data_dir             CONSTANT BINARY_INTEGER := 1003;   -- Name of DIRECTORY defined in Oracle, for location of .ini files and OpenOffice.Org files
Eto_par_file_dir             CONSTANT BINARY_INTEGER := 1021;   -- Optional name of DIRECTORY defined in Oracle, for location of Excel/Access/SQLite files. If this parameter is null then Eto_par_data_dir is used
Eto_par_db_name              CONSTANT BINARY_INTEGER := 1004;   -- Databasename at remote database
Eto_par_db_user              CONSTANT BINARY_INTEGER := 1005;   -- Username for remote database
Eto_par_db_pwd               CONSTANT BINARY_INTEGER := 1006;   -- Password for remote database
Eto_par_def_cacherows        CONSTANT BINARY_INTEGER := 1007;   -- Number of records that are cached by the EasyXTO executable
Eto_par_def_conversion       CONSTANT BINARY_INTEGER := 1008;   -- Custom fieldformat definition
Eto_par_list_table_query     CONSTANT BINARY_INTEGER := 1009;   -- Query to pass to remote database that lists all tables and/or views. For OpenOffice.Org datasources use '*'
Eto_par_enable_logfiles      CONSTANT BINARY_INTEGER := 1010;   -- Enable logfile: Y = enable, all other values = disable
Eto_par_enable_badfiles      CONSTANT BINARY_INTEGER := 1011;   -- Enable badfile: Y = enable, all other values = disable
Eto_par_do_not_quote_names   CONSTANT BINARY_INTEGER := 1012;   -- Skip quoting of the source tablename if it containes spaces. Y = skip quotes, all other values = quote
Eto_par_default_charset      CONSTANT BINARY_INTEGER := 1013;   -- Name of default characterset. Do not set this parameter to use the database default. Can be overwritten when creating tables
                                                                -- For MS-SQL server and SQLite 'WE8ISO8859P15' works well, for spreadsheets and Presto use 'AL32UTF8',
                                                                -- for MS-Access use 'WE8ISO8859P1' or 'WE8MSWIN1252'
Eto_par_readsize             CONSTANT BINARY_INTEGER := 1014;   -- Optional: set the value of the external table READSIZE parameter. This is necessary on Windows platform

-- Constants for parameter: Eto_par_executable --
Eto_bin_type_position        CONSTANT PLS_INTEGER   := 5;       -- Determines in which position of the name of the excutable the first character is that can be used to distinquish between the various types 
Eto_bin_easytto              CONSTANT VARCHAR2(30)  := 'EasyTTOra';
Eto_bin_easymto              CONSTANT VARCHAR2(30)  := 'EasyMTOra';
Eto_bin_easyoto              CONSTANT VARCHAR2(30)  := 'EasyOTOra';
Eto_bin_easysto              CONSTANT VARCHAR2(30)  := 'EasySTOra';
Eto_bin_easyato              CONSTANT VARCHAR2(30)  := 'EasyATOra';
Eto_bin_easypto              CONSTANT VARCHAR2(30)  := 'EasyPTOra';

-- Constants for type conversions - Tabular Data Protocol --
Eto_TDP_ILLEGAL              CONSTANT BINARY_INTEGER :=  9999;
Eto_TDP_CHAR                 CONSTANT BINARY_INTEGER := 10000;
Eto_TDP_BINARY               CONSTANT BINARY_INTEGER := 10001;
Eto_TDP_LONGCHAR             CONSTANT BINARY_INTEGER := 10002;
Eto_TDP_LONGBINARY           CONSTANT BINARY_INTEGER := 10003;
Eto_TDP_TEXT                 CONSTANT BINARY_INTEGER := 10004;
Eto_TDP_IMAGE                CONSTANT BINARY_INTEGER := 10005;
Eto_TDP_TINYINT              CONSTANT BINARY_INTEGER := 10006;
Eto_TDP_SMALLINT             CONSTANT BINARY_INTEGER := 10007;
Eto_TDP_INT                  CONSTANT BINARY_INTEGER := 10008;
Eto_TDP_REAL                 CONSTANT BINARY_INTEGER := 10009;
Eto_TDP_FLOAT                CONSTANT BINARY_INTEGER := 10010;
Eto_TDP_BIT                  CONSTANT BINARY_INTEGER := 10011;
Eto_TDP_DATETIME             CONSTANT BINARY_INTEGER := 10012;
Eto_TDP_DATETIME4            CONSTANT BINARY_INTEGER := 10013;
Eto_TDP_MONEY                CONSTANT BINARY_INTEGER := 10014;
Eto_TDP_MONEY4               CONSTANT BINARY_INTEGER := 10015;
Eto_TDP_NUMERIC              CONSTANT BINARY_INTEGER := 10016;
Eto_TDP_DECIMAL              CONSTANT BINARY_INTEGER := 10017;
Eto_TDP_VARCHAR              CONSTANT BINARY_INTEGER := 10018;
Eto_TDP_VARBINARY            CONSTANT BINARY_INTEGER := 10019;
Eto_TDP_LONG                 CONSTANT BINARY_INTEGER := 10020;
Eto_TDP_SENSITIVITY          CONSTANT BINARY_INTEGER := 10021;
Eto_TDP_BOUNDARY             CONSTANT BINARY_INTEGER := 10022;
Eto_TDP_VOID                 CONSTANT BINARY_INTEGER := 10023;
Eto_TDP_USHORT               CONSTANT BINARY_INTEGER := 10024;
Eto_TDP_UNICHAR              CONSTANT BINARY_INTEGER := 10025;
Eto_TDP_BLOB                 CONSTANT BINARY_INTEGER := 10026;
Eto_TDP_DATE                 CONSTANT BINARY_INTEGER := 10027;
Eto_TDP_TIME                 CONSTANT BINARY_INTEGER := 10028;
Eto_TDP_UNITEXT              CONSTANT BINARY_INTEGER := 10029;
Eto_TDP_BIGINT               CONSTANT BINARY_INTEGER := 10030;
Eto_TDP_USMALLINT            CONSTANT BINARY_INTEGER := 10031;
Eto_TDP_UINT                 CONSTANT BINARY_INTEGER := 10032;
Eto_TDP_UBIGINT              CONSTANT BINARY_INTEGER := 10033;
Eto_TDP_XML                  CONSTANT BINARY_INTEGER := 10034;
Eto_TDP_UNIQUE               CONSTANT BINARY_INTEGER := 10040;
Eto_TDP_USER                 CONSTANT BINARY_INTEGER := 10100;

-- Constants for type conversions - MySQL --
Eto_MYSQL_DECIMAL            CONSTANT BINARY_INTEGER := 20000;
Eto_MYSQL_TINY               CONSTANT BINARY_INTEGER := 20001;
Eto_MYSQL_SHORT              CONSTANT BINARY_INTEGER := 20002;
Eto_MYSQL_LONG               CONSTANT BINARY_INTEGER := 20003;
Eto_MYSQL_FLOAT              CONSTANT BINARY_INTEGER := 20004;
Eto_MYSQL_DOUBLE             CONSTANT BINARY_INTEGER := 20005;
Eto_MYSQL_NULL               CONSTANT BINARY_INTEGER := 20006;
Eto_MYSQL_TIMESTAMP          CONSTANT BINARY_INTEGER := 20007;
Eto_MYSQL_LONGLONG           CONSTANT BINARY_INTEGER := 20008;
Eto_MYSQL_INT24              CONSTANT BINARY_INTEGER := 20009;
Eto_MYSQL_DATE               CONSTANT BINARY_INTEGER := 20010;
Eto_MYSQL_TIME               CONSTANT BINARY_INTEGER := 20011;
Eto_MYSQL_DATETIME           CONSTANT BINARY_INTEGER := 20012;
Eto_MYSQL_YEAR               CONSTANT BINARY_INTEGER := 20013;
Eto_MYSQL_NEWDATE            CONSTANT BINARY_INTEGER := 20014;
Eto_MYSQL_VARCHAR            CONSTANT BINARY_INTEGER := 20015;
Eto_MYSQL_BIT                CONSTANT BINARY_INTEGER := 20016;
Eto_MYSQL_NEWDECIMAL         CONSTANT BINARY_INTEGER := 20246;
Eto_MYSQL_ENUM               CONSTANT BINARY_INTEGER := 20247;
Eto_MYSQL_SET                CONSTANT BINARY_INTEGER := 20248;
Eto_MYSQL_TINY_BLOB          CONSTANT BINARY_INTEGER := 20249;
Eto_MYSQL_MEDIUM_BLOB        CONSTANT BINARY_INTEGER := 20250;
Eto_MYSQL_LONG_BLOB          CONSTANT BINARY_INTEGER := 20251;
Eto_MYSQL_BLOB               CONSTANT BINARY_INTEGER := 20252;
Eto_MYSQL_VAR_STRING         CONSTANT BINARY_INTEGER := 20253;
Eto_MYSQL_STRING             CONSTANT BINARY_INTEGER := 20254;
Eto_MYSQL_GEOMETRY           CONSTANT BINARY_INTEGER := 20255;

-- Constants for type conversions - OpenOffice.Org --
Eto_XLS_TEXT                 CONSTANT BINARY_INTEGER := 30000;
Eto_XLS_NUMBER               CONSTANT BINARY_INTEGER := 30001;
Eto_XLS_DATETIME             CONSTANT BINARY_INTEGER := 30002;
Eto_XLS_DATE                 CONSTANT BINARY_INTEGER := 30003;
Eto_XLS_TIME                 CONSTANT BINARY_INTEGER := 30004;

-- Constants for type conversions - SQLite --
Eto_SQLITE_INTEGER           CONSTANT BINARY_INTEGER := 40001;
Eto_SQLITE_FLOAT             CONSTANT BINARY_INTEGER := 40002;
Eto_SQLITE_TEXT              CONSTANT BINARY_INTEGER := 40003;
Eto_SQLITE_BLOB              CONSTANT BINARY_INTEGER := 40004;

-- Constants for type conversions - JET4 --
Eto_MDB_BOOL                 CONSTANT BINARY_INTEGER := 50001;
Eto_MDB_BYTE                 CONSTANT BINARY_INTEGER := 50002;
Eto_MDB_INT                  CONSTANT BINARY_INTEGER := 50003;
Eto_MDB_LONGINT              CONSTANT BINARY_INTEGER := 50004;
Eto_MDB_MONEY                CONSTANT BINARY_INTEGER := 50005;
Eto_MDB_FLOAT                CONSTANT BINARY_INTEGER := 50006;
Eto_MDB_DOUBLE               CONSTANT BINARY_INTEGER := 50007;
Eto_MDB_DATETIME             CONSTANT BINARY_INTEGER := 50008;
Eto_MDB_BINARY               CONSTANT BINARY_INTEGER := 50009;
Eto_MDB_TEXT                 CONSTANT BINARY_INTEGER := 50010;
Eto_MDB_OLE                  CONSTANT BINARY_INTEGER := 50011;
Eto_MDB_MEMO                 CONSTANT BINARY_INTEGER := 50012;
Eto_MDB_REPID                CONSTANT BINARY_INTEGER := 50015;
Eto_MDB_NUMERIC              CONSTANT BINARY_INTEGER := 50016;

-- Constants for type conversions - Presto --
Eto_PRESTO_UNDEFINED         CONSTANT BINARY_INTEGER := 60000;
Eto_PRESTO_VARCHAR           CONSTANT BINARY_INTEGER := 60001;
Eto_PRESTO_BIGINT            CONSTANT BINARY_INTEGER := 60002;
Eto_PRESTO_BOOLEAN           CONSTANT BINARY_INTEGER := 60003;
Eto_PRESTO_DOUBLE            CONSTANT BINARY_INTEGER := 60004;

-- Functions --

/* Create a new connection. Parameters are stored under the name of this connection
 *
 * pConnectionName  Name of connection to be created. Nothing happens if name already exists.
 */
FUNCTION ETO_Create_Database_Connection(pConnectionName IN varchar2) RETURN NUMBER;

/* Drop a connection and all it's parameters
 *
 * pConnectionName  Name of connection to be deleted, incuding all parameters stored under this name
 */
FUNCTION ETO_Drop_Database_Connection(pConnectionName IN varchar2) RETURN NUMBER;

/* Add or modify a parameter
 *
 * pConnectionName  Name of connection created with ETO_Create_Database_Connection
 * pName            Use a constant for Set/Get parameter defined above
 * pValue1          New value for parameter
 */
FUNCTION ETO_Set_Parameter(pConnectionName IN varchar2, pName IN integer, pValue1 IN varchar2) RETURN NUMBER;

/* Retreive the current value of a parameter
 *
 * pConnectionName  Name of connection created with ETO_Create_Database_Connection
 * pName            Use a constant for Set/Get parameter defined above
 */
FUNCTION ETO_Get_Parameter(pConnectionName IN varchar2, pName IN integer) RETURN VARCHAR2;

/* Set a conversion to overrule the default conversion setting, used for the field definition clause of the external table
 *
 * pConnectionName  Name of connection created with ETO_Create_Database_Connection
 * pType            Use a constant for type conversion defined above
 * pValue1          New value for conversion
 */
FUNCTION ETO_Set_Conversion(pConnectionName IN varchar2, pType IN integer, pValue1 IN varchar2) RETURN NUMBER;

/* ETO_Create_Table
 * Create an external table that uses the Oracle 11.2 Preprocessor facility. The external table uses an .ini file that is passed to an executable file like: EasyTTO, EasyMTO or EasyOTO
 * All blob, image and binary fields are excluded from the external table. It is currenty not possible to import these fieldtypes.
 *
 * pOutputTableName Name of external table to generate. (May include scheme name: scheme.table)
 * pConnectionName  Name of connection created with ETO_Create_Database_Connection
 * pTableName       Name of table at remote database. If pSQL is filled: pTableName only used as name for .ini file. For OpenOffice.Org this the filename of the spreadsheet
 * pDropAllowed     If output table exists, is it allowed to drop the existing table
 * pSQL             Optional query for remote database. For OpenOffice.Org this the worksheetname
 * pCharacterset    Optional name of characterset returned by remote database. If null the value of Eto_par_default_charset is used
 * pOtherParameters Optional semicolon delimited list of parameters that are passed to EasyXTO executable via the .ini file. Currently only used for spreadsheets. Example: 'offset_row = 2;offset_col = 1;'
 *                  These values are valid: offset_row    Start reading from this row. Default = 0
                                            offset_col    Start reading from this column. Default = 0
                                            skip_rows     Skip this number of rows, counted from the row containing columnnames. Default = 1
                                            raw_data      A value not equal to zero causes EasyOTO to return data as text. Use in combination with max_columns. EasyOTO stops reading when 10 empty rows are found. Default = 0
                                            max_columns   Maximum number of columns to read. Default = 1000
                                            date_offset   Julian date representation of startingdate for OpenOffice.Org (default setting = 30-dec-1899). Default = 2415019
 */
FUNCTION ETO_Create_Table(pOutputTableName IN varchar2, pConnectionName IN varchar2, pTableName IN varchar2, pDropAllowed IN varchar2 := 'N', pSQL IN varchar2 := null, pCharacterset IN varchar2 := null, pOtherParameters IN varchar2 := null) RETURN NUMBER;

/* ETO_Create_Tables_From_DB
 * Create external tables for all tables at the remote database that are listed by the sql statement defined with parameter: Eto_par_list_table_query
 * For SQL-Server use something like: 'SELECT ''databasename.dbo.'' + name + '''' as name FROM databasename.sys.all_objects where type in (''U'', ''V'') and is_ms_shipped = 0'
 * For MySQL use something like: 'select concat(table_schema, ''.'', table_name) as name from information_schema.TABLES where table_schema=''databasename'''
 * For spreadsheets use: '*' (Note that worksheets are searched for data starting from cell A1. If the data has an offset use ETO_Create_Table on indiviual worksheets in the document)
 * For MS-Access use: '*'
 *
 * pOutputSchema    Name of Oracle schema for tables to be created in
 * pOutputPrefix    Optional prefix for tables to be created
 * pConnectionName  Name of connection created with ETO_Create_Database_Connection
 * pDropAllowed     If output table exists, is it allowed to drop the existing table
 * pCharacterset    Optional name of characterset returned by remote database. If null the value of Eto_par_default_charset is used
 *
 * Return value     Number of tables created
*/
FUNCTION ETO_Create_Tables_From_DB(pOutputSchema IN varchar2, pOutputPrefix IN varchar2 := null, pConnectionName IN varchar2, pDropAllowed IN varchar2 := 'N', pCharacterset IN varchar2 := null) RETURN NUMBER;

/* EXAMPLES

-- Create table sys.utl_eto$ (done by installer script) --
CREATE TABLE sys.utl_eto$
( ConnectionName  varchar2(4000)
, Parameter       integer
, ParameterValue  varchar2(4000)
);


-- Setup directories --
-- In order to maintain a SECURE system:
-- 1. Don't grant other access rights than these
-- 2. Do not use the same OS directory for these 3 dirs
-- 3. Use appropriate access rights at OS level (same as on database)
-- The names of directories can be chosen freely. Just use the same names for setting parameters Eto_par_bin_dir, Eto_par_data_dir and Eto_par_file_dir --

CREATE OR REPLACE DIRECTORY user_dir_eto_bin AS '/path_to_dir/eto_bin';
GRANT READ, EXECUTE ON DIRECTORY user_dir_eto_bin TO someuser;

CREATE OR REPLACE DIRECTORY user_dir_eto_data AS '/path_to_dir/eto_data';
GRANT READ, WRITE ON DIRECTORY user_dir_eto_data TO someuser;

CREATE OR REPLACE DIRECTORY user_dir_eto_files AS '/path_to_dir/eto_files';
GRANT READ, WRITE ON DIRECTORY user_dir_eto_files TO someuser;


-- Create connection and set parameters using a sample MS-SQL Server database --
DECLARE
  r   number;
BEGIN
  r := SYS.UTL_ETO.ETO_Create_Database_Connection('MSTEST');
  r := SYS.UTL_ETO.ETO_Set_Parameter('MSTEST', SYS.UTL_ETO.Eto_par_executable,      'EasyTTOra');
  r := SYS.UTL_ETO.ETO_Set_Parameter('MSTEST', SYS.UTL_ETO.Eto_par_bin_dir,         'user_dir_eto_bin');
  r := SYS.UTL_ETO.ETO_Set_Parameter('MSTEST', SYS.UTL_ETO.Eto_par_data_dir,        'user_dir_eto_data');
  r := SYS.UTL_ETO.ETO_Set_Parameter('MSTEST', SYS.UTL_ETO.Eto_par_db_name,         'ipadress_or_servername:port'); -- SQL Server uses 1433 as default tcp port, MySQL uses 3306, Presto uses 8080 --
  r := SYS.UTL_ETO.ETO_Set_Parameter('MSTEST', SYS.UTL_ETO.Eto_par_db_user,         'user');
  r := SYS.UTL_ETO.ETO_Set_Parameter('MSTEST', SYS.UTL_ETO.Eto_par_db_pwd,          'password');
  r := SYS.UTL_ETO.ETO_Set_Parameter('MSTEST', SYS.UTL_ETO.Eto_par_default_charset, 'WE8ISO8859P15');
  r := SYS.UTL_ETO.ETO_Set_Parameter('MSTEST', SYS.UTL_ETO.Eto_par_def_cacherows,   '1'); -- Size in rows for number of records to be cached by the executable. Generally lower values (i.e. 1) are faster --
END;


-- Create a single table --
DECLARE
  r   number;
BEGIN
  r := SYS.UTL_ETO.ETO_CREATE_TABLE(
      pOutputTableName => 'SCHEMA.TABLENAME'
    , pDropAllowed     => 'Y'
    , pConnectionname  => 'MSTEST'
    , pTablename       => 'Northwind.dbo.Employees'
  );

  DBMS_OUTPUT.PUT_LINE('Return value = ' || r);
END;


-- Create multiple tables --
DECLARE
  r   number;
BEGIN
  DBMS_OUTPUT.enable(20000);

  r := SYS.UTL_ETO.ETO_Set_Parameter('MSTEST', SYS.UTL_ETO.Eto_par_list_table_query, 'SELECT ''databasename.dbo.'' + name + '''' as name FROM databasename.sys.all_objects where type in (''U'', ''V'') and is_ms_shipped = 0');

  r := SYS.UTL_ETO.ETO_Create_Tables_From_DB
    ( pOutputSchema   => 'SCHEMA'
    , pOutputPrefix   => 'MS_'
    , pConnectionname => 'MSTEST'
    , pDropAllowed    => 'Y'
  );

  DBMS_OUTPUT.PUT_LINE('Return value = ' || r);
END;


-- Excel example --
DECLARE
  r   NUMBER;
BEGIN
  DBMS_OUTPUT.enable(20000);

  r := SYS.UTL_ETO.ETO_Create_Database_Connection('XLS');
  r := SYS.UTL_ETO.ETO_Set_Parameter('XLS', SYS.UTL_ETO.Eto_par_executable,         'EasyOTOra.sh');
  r := SYS.UTL_ETO.ETO_Set_Parameter('XLS', SYS.UTL_ETO.Eto_par_bin_dir,            'user_dir_eto_bin');
  r := SYS.UTL_ETO.ETO_Set_Parameter('XLS', SYS.UTL_ETO.Eto_par_data_dir,           'user_dir_eto_data');
  r := SYS.UTL_ETO.ETO_Set_Parameter('XLS', SYS.UTL_ETO.Eto_par_file_dir,           'user_dir_eto_files');
  r := SYS.UTL_ETO.ETO_Set_Parameter('XLS', SYS.UTL_ETO.Eto_par_db_user,            ' ');
  r := SYS.UTL_ETO.ETO_Set_Parameter('XLS', SYS.UTL_ETO.Eto_par_db_pwd,             ' ');
  r := SYS.UTL_ETO.ETO_Set_Parameter('XLS', SYS.UTL_ETO.Eto_par_def_cacherows,      '2');
  r := SYS.UTL_ETO.ETO_Set_Parameter('XLS', SYS.UTL_ETO.Eto_par_default_charset,    'AL32UTF8');
  r := SYS.UTL_ETO.ETO_Set_Parameter('XLS', SYS.UTL_ETO.Eto_par_list_table_query,   '*');

  -- The Eto_par_db_name is used to enter the 'database' name. This file should be present in directory 'Eto_par_file_dir' --
  r := SYS.UTL_ETO.ETO_Set_Parameter('XLS', SYS.UTL_ETO.Eto_par_db_name,            'Test.xls');

  r := SYS.UTL_ETO.ETO_Create_Tables_From_DB
    ( pOutputSchema   => 'SCHEMA'
    , pOutputPrefix   => 'XLS_'
    , pConnectionname => 'XLS'
    , pDropAllowed    => 'Y'
    , pCharacterset   => 'AL32UTF8'
  );

  DBMS_OUTPUT.PUT_LINE('v_Return = ' || r);
  
  -- Convert any date fields with: to_date(src.datefield + 2415019, 'J') as my_datefield --
  
END;
*/
END UTL_ETO;
/


CREATE OR REPLACE PACKAGE BODY SYS.UTL_ETO
AS
/*****************************************************************************
 * COPYRIGHT (C) 2011-2014 Ivo Herweijer
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
 ****************************************************************************/


-- Globals, cursors and types --
gDescribe_id          pls_integer;
gDataPath             varchar2(250);
gFilesPath            varchar2(250);

--
CURSOR cConnectionRecord(pConnectionName IN varchar2)
IS
SELECT max(decode(Parameter, Eto_par_executable,           ParameterValue, null)) as executable
,      max(decode(Parameter, Eto_par_bin_dir,              ParameterValue, null)) as bin_dir
,      max(decode(Parameter, Eto_par_data_dir,             ParameterValue, null)) as data_dir
,      max(decode(Parameter, Eto_par_db_name,              ParameterValue, null)) as db_name
,      max(decode(Parameter, Eto_par_db_user,              ParameterValue, null)) as db_user
,      max(decode(Parameter, Eto_par_db_pwd,               ParameterValue, null)) as db_pwd
,      max(decode(Parameter, Eto_par_def_cacherows,        ParameterValue, null)) as cache_rows
,      max(decode(Parameter, Eto_par_list_table_query,     ParameterValue, null)) as list_table_query
,      max(decode(Parameter, Eto_par_enable_logfiles,      ParameterValue, null)) as enable_logfiles
,      max(decode(Parameter, Eto_par_enable_badfiles,      ParameterValue, null)) as enable_badfiles
,      max(decode(Parameter, Eto_par_do_not_quote_names,   ParameterValue, null)) as do_not_quote_names
,      max(decode(Parameter, Eto_par_default_charset,      ParameterValue, null)) as default_charset
,      max(decode(Parameter, Eto_par_file_dir,             ParameterValue, null)) as file_dir
,      max(decode(Parameter, Eto_par_readsize,             ParameterValue, null)) as readsize
FROM   utl_eto$
WHERE  ConnectionName = pConnectionName
GROUP BY ConnectionName
;

TYPE type_ConnectionRecord IS TABLE OF cConnectionRecord%ROWTYPE;

--
CURSOR cParameterValue(pConnectionName IN varchar2, pParameter IN BINARY_INTEGER)
IS
SELECT ParameterValue
FROM   utl_eto$
WHERE  ConnectionName = pConnectionName
AND    Parameter      = pParameter;

--
CURSOR cConversions(pConnectionName IN varchar2)
IS
SELECT to_number(substr(ParameterValue, 1, instr(ParameterValue, ',') - 1)) as T_Datatype
,      substr(ParameterValue, instr(ParameterValue, ',') + 1)               as T_Conversion
FROM   utl_eto$
WHERE  ConnectionName = pConnectionName
and    Parameter      = Eto_par_def_conversion
;

TYPE type_ConversionRecord IS TABLE OF cConversions%ROWTYPE;
gConv         type_ConversionRecord;

-- Type and table to hold records produced by describe table query -- 
TYPE type_Describe IS RECORD
( T_Describe_Id     Number
, T_Name            Varchar2(128)
, T_Datatype        Number
, T_Datatype_Descr  Varchar2(20)
, T_Maxlength       Number
, T_Scale           Number
, T_Precision       Number
, T_Notnull         Varchar2(1)
, T_Statement       Varchar2(4000)
);

TYPE type_Describe_tab IS TABLE OF type_Describe;
gDesc         type_Describe_tab;

-- Type and table to hold records produced by list tables on remote database query --
TYPE type_TableList    IS TABLE OF varchar2(128);
gTableList    type_TableList;


/* ---------------------------------------------------------------------------------------------------------------- */
PROCEDURE Update_Parameter(pConnectionName IN varchar2, pParameter IN BINARY_INTEGER, pParameterValue IN varchar2)
AS
  PRAGMA AUTONOMOUS_TRANSACTION;
  vConn       varchar2(4000);
  eNoConn     exception;
BEGIN

  -- Check if connection exists --
  IF pParameter <> Eto_par_connection THEN
    OPEN  cParameterValue(pConnectionName, Eto_par_connection);
    FETCH cParameterValue INTO vConn;
    CLOSE cParameterValue;

    IF vConn is null THEN
      Raise eNoConn;
    END IF;
  END IF;

  -- Update parameter --
  MERGE INTO utl_eto$ target
  USING (SELECT * FROM dual) src
  ON (    target.ConnectionName = pConnectionName
      and target.Parameter      = pParameter )
  WHEN MATCHED THEN UPDATE SET target.ParameterValue = pParameterValue
                    DELETE WHERE (pParameterValue is null)
  WHEN NOT MATCHED THEN INSERT (ConnectionName, Parameter, ParameterValue)
                        VALUES (pConnectionName, pParameter, pParameterValue);

  COMMIT;

  EXCEPTION
    WHEN eNoConn THEN
      RAISE_APPLICATION_ERROR(-20000, 'ETO-20000 Connection does not exist: ' || pConnectionName);
    WHEN others THEN
      RAISE_APPLICATION_ERROR(-20000, 'ETO-20000 Error writing parameter' || chr(10) || SQLERRM);
END Update_Parameter;

/* ---------------------------------------------------------------------------------------------------------------- */
FUNCTION ETO_Create_Database_Connection(pConnectionName IN varchar2) RETURN NUMBER
AS
BEGIN
  IF pConnectionName is null THEN
    Return -1;
  END IF;

  Update_Parameter(upper(pConnectionName), Eto_par_connection, upper(pConnectionName));

  Return 0;

  EXCEPTION
    WHEN others THEN
      RAISE_APPLICATION_ERROR(-20001, 'ETO-20001 Error creating connection' || chr(10) || SQLERRM);
      Return -1;
END ETO_Create_Database_Connection;

/* ---------------------------------------------------------------------------------------------------------------- */
FUNCTION ETO_Drop_Database_Connection(pConnectionName IN varchar2) RETURN NUMBER
AS
  PRAGMA AUTONOMOUS_TRANSACTION;
BEGIN

  IF pConnectionName is null THEN
    Return -1;
  END IF;

  DELETE FROM utl_eto$ target
  WHERE  target.ConnectionName = upper(pConnectionName);

  COMMIT;

  Return 0;

  EXCEPTION
    WHEN others THEN
      RAISE_APPLICATION_ERROR(-20002, 'ETO-20002 Error deleting connection' || chr(10) || SQLERRM);
      Return -1;
END ETO_Drop_Database_Connection;

/* ---------------------------------------------------------------------------------------------------------------- */
FUNCTION ETO_Set_Parameter(pConnectionName IN varchar2, pName IN integer, pValue1 IN varchar2) RETURN NUMBER
AS
  vValue1        varchar2(4000);
  eParInvalid1   exception;
  eValInvalid1   exception;
BEGIN

  IF pConnectionName is null OR pName is null OR pValue1 is null THEN
    Return -1;
  END IF;

  -- Check if parameter is within valid range and not Eto_par_connection --
  IF pName < Eto_par_executable OR pName > Eto_par_file_dir THEN
    Raise eParInvalid1;
  END IF;

  -- Check value for Eto_par_executable. This enables us to use this name to determine which remote platform we are dealing with --
  IF pName = Eto_par_executable AND substr(pValue1, 1, 9) not in (Eto_bin_easytto, Eto_bin_easymto, Eto_bin_easyoto, Eto_bin_easysto, Eto_bin_easyato, Eto_bin_easypto) THEN
    Raise eValInvalid1;
  END IF;

  IF pName NOT IN (Eto_par_db_name, Eto_par_db_user, Eto_par_db_pwd, Eto_par_executable, Eto_par_list_table_query) THEN
    vValue1 := upper(pValue1);
  ELSE
    vValue1 := pValue1;
  END IF;

  Update_Parameter(upper(pConnectionName), pName, vValue1);

  Return 0;

  EXCEPTION
    WHEN eParInvalid1 THEN
      RAISE_APPLICATION_ERROR(-20003, 'ETO-20003 Parameter is invalid: ' || pName);
      Return -1;
    WHEN eValInvalid1 THEN
      RAISE_APPLICATION_ERROR(-20003, 'ETO-20003 Value ''Eto_par_executable=' || pValue1 || ''' is invalid. Please refer to the UTL_ETO package for valid values');
      Return -1;
END ETO_Set_Parameter;

/* ---------------------------------------------------------------------------------------------------------------- */
FUNCTION ETO_Get_Parameter(pConnectionName IN varchar2, pName IN integer) RETURN VARCHAR2
AS
  vPar        varchar2(4000);
BEGIN

  IF pName = Eto_par_db_pwd THEN
    Return '*';
  END IF;

  OPEN  cParameterValue(pConnectionName, pName);
  FETCH cParameterValue INTO vPar;
  CLOSE cParameterValue;

  Return vPar; -- todo catch multiple values --

  EXCEPTION
    WHEN others THEN
      RAISE_APPLICATION_ERROR(-20003, 'ETO-20003 Unknown error getting parameter' || chr(10) || SQLERRM);
      Return NULL;
END ETO_Get_Parameter;

/* ---------------------------------------------------------------------------------------------------------------- */
FUNCTION ETO_Set_Conversion(pConnectionName IN varchar2, pType IN integer, pValue1 IN varchar2) RETURN NUMBER
AS
  vValue1        varchar2(4000);
  eParInvalid2   exception;
BEGIN

  IF pConnectionName is null OR pType is null OR pValue1 is null THEN
    Return -1;
  END IF;

  -- Check if parameter is valid --
  IF pType < Eto_TDP_ILLEGAL OR pType > Eto_TDP_USER THEN
    Raise eParInvalid2;
  END IF;

  vValue1 := to_char(pType) || ',' || upper(pValue1);

  Update_Parameter(upper(pConnectionName), Eto_par_def_conversion, vValue1);

  Return 0;

  EXCEPTION
    WHEN eParInvalid2 THEN
      RAISE_APPLICATION_ERROR(-20003, 'ETO-20003 Parameter is invalid: ' || pType);
      Return -1;
END ETO_Set_Conversion;

/* ---------------------------------------------------------------------------------------------------------------- */
FUNCTION ETO_Read_Logfile_Contents(p_data_dir IN varchar2, p_logfilename IN varchar2) RETURN varchar2
AS
  f    utl_file.file_type;
  line varchar2(1024);
  msg  varchar2(1024);
BEGIN
  f := utl_file.fopen(p_data_dir, p_logfilename, 'r');

  begin
    msg := 'Logmessage: ';
    while true loop
      utl_file.get_line(f, line, 1024);

      if length(msg) + length(line) <= 1024 then
        msg := msg || line;
      end if;

    end loop;
  exception when NO_DATA_FOUND then null;
  end;

  utl_file.fclose(f);

  Return msg;

  EXCEPTION
    WHEN others THEN
      Return 'Check log file:' || p_logfilename;
END ETO_Read_Logfile_Contents;

/* ---------------------------------------------------------------------------------------------------------------- */
FUNCTION ETO_Get_Data_Path(pDir IN varchar2) RETURN varchar2
AS
  vPath        varchar2(250);
  eNoDir       exception;
BEGIN

  select directory_path into vPath
  from   all_directories
  where  directory_name = pDir;

  IF vPath is null THEN
    Raise eNoDir;
  END IF;

  vPath := rtrim(vPath, '/');
  vPath := rtrim(vPath, '\');

  -- Add Linux or Windows style separator --
  IF instr(vPath, '/') > 0 THEN
    vPath := vPath || '/';
  ELSE
    vPath := vPath || '\';
  END IF;

  Return vPath;

  EXCEPTION
    WHEN eNoDir THEN
      RAISE_APPLICATION_ERROR(-20010, 'ETO-20010 Directory ''' || pDir || ''' not defined');
      Return NULL;
    WHEN others THEN
      RAISE_APPLICATION_ERROR(-20010, 'ETO-20010 Error getting directory ''' || pDir || ''' path: ' || SQLERRM);
      Return NULL;
END ETO_Get_Data_Path;

/* ---------------------------------------------------------------------------------------------------------------- */
FUNCTION ETO_Write_ini( p_filename IN varchar2, p_data_dir   IN varchar2, p_db_name     IN varchar2
                      , p_db_user  IN varchar2, p_db_pwd     IN varchar2, pTableName    IN varchar2
                      , pSQL       IN varchar2, p_cache_rows IN varchar2, p_logfilename IN varchar2
                      , pOtherParameters IN varchar2 := null)
RETURN NUMBER
AS
  f   utl_file.file_type;
BEGIN
  f := utl_file.fopen(p_data_dir, p_filename, 'w');

  utl_file.put_line(f, '[EasyXTOra]');
  utl_file.put_line(f, 'database       = ' || p_db_name || ';');

  IF length(trim(p_db_user)) > 0 THEN  
  utl_file.put_line(f, 'user           = ' || p_db_user || ';');
  END IF;
  
  IF length(trim(p_db_pwd)) > 0 THEN  
  utl_file.put_line(f, 'password       = ' || p_db_pwd || ';');
  END IF;

  IF p_cache_rows is not null THEN
  utl_file.put_line(f, 'cache_rows     = ' || p_cache_rows || ';');
  END IF;

  utl_file.put_line(f, 'logfilename    = ' || gDataPath || p_logfilename || ';');

  IF gDescribe_id > 0 THEN
  utl_file.put_line(f, 'describe_id    = ' || gDescribe_id || ';');
  utl_file.put_line(f, 'fieldseparator = ";";');
  utl_file.put_line(f, 'lineseparator  = \n;'  );
  END IF;

  IF pTableName is not null THEN
  utl_file.put_line(f, 'table          = ' || pTableName || ';');
  END IF;

  IF pSQL is not null THEN
  utl_file.put_line(f, 'sql            = ' || rtrim(trim(replace(pSQL, chr(10), ' \' || chr(10) )), ';') || ';');
  END IF;

  IF pOtherParameters is not null THEN
  utl_file.put_line(f, replace(pOtherParameters, ';', chr(10)));
  END IF;

  utl_file.fclose(f);

  Return 0;

  EXCEPTION
    WHEN others THEN
      RAISE_APPLICATION_ERROR(-20005, 'ETO-20005 Error writing ini file' || chr(10) || SQLERRM);
      Return -1;
END ETO_Write_ini;

/* ---------------------------------------------------------------------------------------------------------------- */
FUNCTION ETO_Create_Describe_Table(p_executable IN varchar2, p_bin_dir IN varchar2, p_data_dir IN varchar2)
RETURN NUMBER
AS
  vDDL       varchar2(2000);
BEGIN
          vDDL := 'CREATE TABLE ' || Eto_descr_tablename || chr(10);
  vDDL := vDDL || '(  t_describe_id    number' || chr(10);
  vDDL := vDDL || ',  t_name           varchar2(128)' || chr(10);
  vDDL := vDDL || ',  t_datatype       number' || chr(10);
  vDDL := vDDL || ',  t_datatype_descr varchar2(20)' || chr(10);
  vDDL := vDDL || ',  t_maxlength      number' || chr(10);
  vDDL := vDDL || ',  t_scale          number' || chr(10);
  vDDL := vDDL || ',  t_precision      number' || chr(10);
  vDDL := vDDL || ',  t_notnull        varchar2(1)' || chr(10);
  vDDL := vDDL || ',  t_statement      varchar2(4000))' || chr(10);
  vDDL := vDDL || 'ORGANIZATION EXTERNAL' || chr(10);
  vDDL := vDDL || '    ( TYPE              ORACLE_LOADER' || chr(10);
  vDDL := vDDL || '      DEFAULT DIRECTORY ' || p_data_dir || chr(10);
  vDDL := vDDL || '      ACCESS PARAMETERS' || chr(10);
  vDDL := vDDL || '      (' || chr(10);
  vDDL := vDDL || '          RECORDS DELIMITED   BY NEWLINE' || chr(10);
  vDDL := vDDL || '          PREPROCESSOR ' || p_bin_dir || ': ''' || p_executable || '''' || chr(10);
  vDDL := vDDL || '          NOBADFILE' || chr(10);
  vDDL := vDDL || '          NOLOGFILE' || chr(10);
  vDDL := vDDL || '          SKIP                1' || chr(10);
  vDDL := vDDL || '          FIELDS TERMINATED   BY '';''' || chr(10);
  vDDL := vDDL || '          MISSING FIELD VALUES ARE NULL' || chr(10);
  vDDL := vDDL || '          (  t_describe_id                   unsigned integer external (12)' || chr(10);
  vDDL := vDDL || '          ,  t_name                          char(128)' || chr(10);
  vDDL := vDDL || '          ,  t_datatype                      unsigned integer external (8)' || chr(10);
  vDDL := vDDL || '          ,  t_datatype_descr                char(20)' || chr(10);
  vDDL := vDDL || '          ,  t_maxlength                     unsigned integer external (8)' || chr(10);
  vDDL := vDDL || '          ,  t_scale                         unsigned integer external (8)' || chr(10);
  vDDL := vDDL || '          ,  t_precision                     unsigned integer external (8)' || chr(10);
  vDDL := vDDL || '          ,  t_notnull                       char(1)' || chr(10);
  vDDL := vDDL || '          ,  t_statement                     char(4000)' || chr(10);
  vDDL := vDDL || '          )' || chr(10);
  vDDL := vDDL || '      )' || chr(10);
  vDDL := vDDL || '    LOCATION (''' || Eto_descr_ininame || ''')' || chr(10);
  vDDL := vDDL || '    )' || chr(10);
  vDDL := vDDL || 'NOPARALLEL' || chr(10);
  vDDL := vDDL || 'REJECT LIMIT UNLIMITED';

  Execute immediate vDDL;

  Execute immediate 'grant select on ' || Eto_descr_tablename || ' to public';

  Return 0;

  EXCEPTION
    WHEN others THEN
      Return -1;
END ETO_Create_Describe_Table;

/* ---------------------------------------------------------------------------------------------------------------- */
FUNCTION ETO_Create_List_Table(p_executable IN varchar2, p_bin_dir IN varchar2, p_data_dir IN varchar2)
RETURN NUMBER
AS
  vDDL       varchar2(2000);
BEGIN
          vDDL := 'CREATE TABLE ' || Eto_List_Tables_tablename || chr(10);
  vDDL := vDDL || '(  t_tablename      varchar2(128) )' || chr(10);
  vDDL := vDDL || 'ORGANIZATION EXTERNAL' || chr(10);
  vDDL := vDDL || '    ( TYPE              ORACLE_LOADER' || chr(10);
  vDDL := vDDL || '      DEFAULT DIRECTORY ' || p_data_dir || chr(10);
  vDDL := vDDL || '      ACCESS PARAMETERS' || chr(10);
  vDDL := vDDL || '      (' || chr(10);
  vDDL := vDDL || '          RECORDS DELIMITED   BY X''01''' || chr(10);
  vDDL := vDDL || '          PREPROCESSOR ' || p_bin_dir || ': ''' || p_executable || '''' || chr(10);
  vDDL := vDDL || '          NOBADFILE' || chr(10);
  vDDL := vDDL || '          NOLOGFILE' || chr(10);
  vDDL := vDDL || '          FIELDS TERMINATED   BY X''01''' || chr(10);
  vDDL := vDDL || '          MISSING FIELD VALUES ARE NULL' || chr(10);
  vDDL := vDDL || '          (  t_tablename char(128) )' || chr(10);
  vDDL := vDDL || '      )' || chr(10);
  vDDL := vDDL || '    LOCATION (''' || Eto_List_Tables_ininame || ''')' || chr(10);
  vDDL := vDDL || '    )' || chr(10);
  vDDL := vDDL || 'NOPARALLEL' || chr(10);
  vDDL := vDDL || 'REJECT LIMIT UNLIMITED';

  Execute immediate vDDL;

  Execute immediate 'grant select on ' || Eto_List_Tables_tablename || ' to public';

  Return 0;

  EXCEPTION
    WHEN others THEN
      Return -1;
END ETO_Create_List_Table;

/* ---------------------------------------------------------------------------------------------------------------- */
FUNCTION ETO_Describe( pConnectionName IN varchar2, p_executable IN varchar2
                     , p_bin_dir IN varchar2, p_data_dir IN varchar2, p_db_name IN varchar2, p_db_user IN varchar2
                     , p_db_pwd  IN varchar2, pTableName IN varchar2, pSQL      IN varchar2, p_cache_rows IN varchar2
                     , pFilename IN varchar2, pOtherParameters IN varchar2 := null)
RETURN NUMBER
AS
  vExists          pls_integer;
  eCreateDescr     exception;
  eQueryDescr      exception;
  eDropError       exception;
BEGIN

  -- Write ini file --
  gDescribe_id := dbms_random.value(1000000, 9999999);

  IF ETO_Write_ini(Eto_descr_ininame, p_data_dir, p_db_name, p_db_user, p_db_pwd, pFilename, pSQL, p_cache_rows, Eto_descr_logname, pOtherParameters) <> 0 THEN
    Return -1; -- Errors are handled by ini writer so this should never occur --
  END IF;

  -- Check if describe table exists --
  select count(*) into vExists from USER_ALL_TABLES where table_name = Eto_descr_tablename;

  IF vExists > 0 THEN
    BEGIN
      Execute immediate 'DROP TABLE ' || Eto_descr_tablename;
    EXCEPTION WHEN others THEN Raise eDropError;
    END;
  END IF;

  -- (Re)create table --
  IF ETO_Create_Describe_Table(p_executable, p_bin_dir, p_data_dir) <> 0 THEN
    Raise eCreateDescr;
  END IF;

  -- Run describe --
  -- Since describe table is created by this package, it is not possible to define a static cursor so we use dynamic sql --
  EXECUTE IMMEDIATE 'Select * from ' || Eto_descr_tablename || ' where t_describe_id=' || gDescribe_id
  BULK COLLECT INTO gDesc;

  IF gDesc is null THEN
    Raise eQueryDescr;
  END IF;

  IF gDesc.count = 0 THEN
    Raise eQueryDescr;
  END IF;

  -- Fetch conversions --
  OPEN  cConversions(pConnectionName);
  FETCH cConversions BULK COLLECT INTO gConv;
  CLOSE cConversions;

  Return 0;

  EXCEPTION
    WHEN eDropError THEN
      RAISE_APPLICATION_ERROR(-20006, 'ETO-20006 Error describing table. Drop ' || Eto_descr_tablename || ' failed: ' || SQLERRM);
      Return -1;
    WHEN eCreateDescr THEN
      RAISE_APPLICATION_ERROR(-20006, 'ETO-20006 Error describing table. Create ' || Eto_descr_tablename || ' failed: ' || SQLERRM);
      Return -1;
    WHEN eQueryDescr THEN
      RAISE_APPLICATION_ERROR(-20006, 'ETO-20006 Error describing table. ' || ETO_Read_Logfile_Contents(p_data_dir, Eto_descr_logname) || ' : ' || SQLERRM);
      Return -1;
    WHEN others THEN
      RAISE_APPLICATION_ERROR(-20006, 'ETO-20006 Error describing table. ' || ETO_Read_Logfile_Contents(p_data_dir, Eto_descr_logname) || ' : ' || SQLERRM);
      Return -1;
END ETO_Describe;

/* ---------------------------------------------------------------------------------------------------------------- */
FUNCTION ETO_Rewrite_SQL(pTableName IN varchar2)
RETURN varchar2
AS
  vRewrite    boolean;
  vSQL        varchar2(4000);
BEGIN

  vRewrite := false;
  vSQL     := null;

  -- Are there any conversions necessary ? -- XLS types can not be rewritten (since sql is the worksheetname --
  FOR i IN gDesc.FIRST .. gDesc.LAST LOOP
    IF gDesc(i).T_Datatype in (Eto_TDP_DATETIME, Eto_TDP_DATETIME4, Eto_TDP_BINARY, Eto_TDP_VARBINARY, Eto_TDP_LONGBINARY, Eto_TDP_IMAGE, Eto_TDP_BLOB, Eto_MYSQL_TINY_BLOB, Eto_MYSQL_MEDIUM_BLOB, Eto_MYSQL_LONG_BLOB, Eto_MYSQL_BLOB, Eto_SQLITE_BLOB) THEN
      vRewrite := true;
      exit;
    END IF;
  END LOOP;
  
  --  Construct statement --
  IF vRewrite THEN
    vSQL := 'Select ' || chr(10);

    FOR i IN gDesc.FIRST .. gDesc.LAST LOOP
      IF gDesc(i).T_Datatype in (Eto_TDP_DATETIME, Eto_TDP_DATETIME4) THEN
        vSQL := vSQL || 'Cast(' || gDesc(i).T_Name || ' as datetime2(0)) as ' || gDesc(i).T_Name || ',' || chr(10);
      ELSIF gDesc(i).T_Datatype in (Eto_TDP_BINARY, Eto_TDP_VARBINARY, Eto_TDP_LONGBINARY, Eto_TDP_IMAGE, Eto_TDP_BLOB, Eto_MYSQL_TINY_BLOB, Eto_MYSQL_MEDIUM_BLOB, Eto_MYSQL_LONG_BLOB, Eto_MYSQL_BLOB, Eto_SQLITE_BLOB) THEN
        gDesc.Delete(i); -- Remove binary fields from nested table --
      ELSE
        vSQL := vSQL || gDesc(i).T_Name || ',' || chr(10);
      END IF;
    END LOOP;

    vSQL := rtrim(rtrim(vSQL, chr(10)), ',') || chr(10);
    vSQL := vSQL || 'from ' || pTableName;
  END IF;

 Return vSQL;

  EXCEPTION
    WHEN others THEN
      Return null;
END ETO_Rewrite_SQL;

/* ---------------------------------------------------------------------------------------------------------------- */
FUNCTION ETO_Check_Fieldnames
RETURN number
AS
  vBaseName varchar2(128);
  cChar     varchar2(2);
  vCnt      pls_integer;
  vFound    boolean;
BEGIN
  IF gDesc is null THEN
    Return -1;
  END IF;

  -- Check content --
  FOR i IN gDesc.FIRST .. gDesc.LAST LOOP
    IF gDesc.Exists(i) THEN
      -- Cannot start with a digit --
      IF substr(gDesc(i).T_Name, 1, 1) between '0' and '9' THEN
        gDesc(i).T_Name := 'c_' || gDesc(i).T_Name;
      END IF;

      -- Replace illegal characters with underscores --
      FOR c IN 1..length(gDesc(i).T_Name) LOOP
        cChar := upper(substr(gDesc(i).T_Name, c, 1));
        IF NOT(cChar between 'A' and 'Z' or cChar between '0' and '9' or cChar = '_') THEN
          gDesc(i).T_Name := replace(gDesc(i).T_Name, cChar, '_');
        END IF;
      END LOOP;
      
      -- Maximum length is 30 --
      gDesc(i).T_Name := substr(gDesc(i).T_Name, 1, 30);
    END IF;
  END LOOP;

  -- Are there any duplicate or empty fieldnames -- this should only happen when reading from spreadsheets --
  IF gDesc.count < 2 THEN
    Return 0;
  END IF;

  FOR i IN 2 .. gDesc.count LOOP
    IF gDesc.Exists(i) THEN
      FOR j IN 1 .. (i-1) LOOP
        IF gDesc.Exists(j) THEN
          IF gDesc(i).T_Name = gDesc(j).T_Name THEN

            vBaseName := substr(gDesc(i).T_Name, 1, 27) || '_';
            vCnt := 1;
            vFound := true;

            WHILE vFound LOOP

              vFound := false;

              FOR t IN 1 .. gDesc.count LOOP
                IF gDesc.Exists(t) THEN
                  IF t <> i and gDesc(i).T_Name = vBaseName || to_char(vCnt) THEN
                    vFound := true;
                    vCnt := vCnt + 1;
                  END IF;
                END IF;
              END LOOP;
          
              gDesc(i).T_Name := vBaseName || to_char(vCnt);

            END LOOP;
          END IF;
        END IF;
      END LOOP;
    END IF;
  END LOOP;

  Return 0;

  EXCEPTION
    WHEN others THEN
      Return -1;
END ETO_Check_Fieldnames;

/* ---------------------------------------------------------------------------------------------------------------- */
FUNCTION ETO_Translate_Type( p_Type IN pls_integer, p_Length IN pls_integer, p_Scale IN pls_integer, p_Precision IN pls_integer)
RETURN varchar2
AS
  vOutType    varchar2(50);
  vCharLength pls_integer;
BEGIN

  IF p_Length is NULL OR p_Length < 1 OR p_Length > 4000 THEN
    vCharLength := 4000;
  ELSE
    vCharLength := p_Length;
  END IF;

  CASE p_Type
    WHEN Eto_TDP_CHAR           THEN vOutType := 'varchar2(' || vCharLength || ')';
    WHEN Eto_TDP_LONGCHAR       THEN vOutType := 'varchar2(' || vCharLength || ')';
    WHEN Eto_TDP_TEXT           THEN vOutType := 'varchar2(' || vCharLength || ')';
    WHEN Eto_TDP_VARCHAR        THEN vOutType := 'varchar2(' || vCharLength || ')';
    WHEN Eto_TDP_LONG           THEN vOutType := 'varchar2(' || vCharLength || ')';

    WHEN Eto_TDP_UNICHAR        THEN vOutType := 'nvarchar2(' || vCharLength || ')';
    WHEN Eto_TDP_UNITEXT        THEN vOutType := 'nvarchar2(' || vCharLength || ')';

  --WHEN Eto_TDP_BINARY         THEN vOutType := 'raw(2000)'; -- max length = 2000 for raw type --
  --WHEN Eto_TDP_VARBINARY      THEN vOutType := 'raw(2000)'; -- max length = 2000 for raw type --
  --WHEN Eto_TDP_LONGBINARY     THEN vOutType := 'raw(2000)'; -- blob conversion isn't going to work --
  --WHEN Eto_TDP_IMAGE          THEN vOutType := 'raw(2000)'; -- blob conversion isn't going to work --
  --WHEN Eto_TDP_BLOB           THEN vOutType := 'raw(4000)'; -- blob conversion isn't going to work --

    WHEN Eto_TDP_REAL           THEN vOutType := 'number';
    WHEN Eto_TDP_FLOAT          THEN vOutType := 'number';
    WHEN Eto_TDP_MONEY          THEN vOutType := 'number';
    WHEN Eto_TDP_MONEY4         THEN vOutType := 'number';
    WHEN Eto_TDP_DECIMAL        THEN vOutType := 'number'; -- '(' || p_Scale || ',' || p_Precision || ')';

    WHEN Eto_TDP_TINYINT        THEN vOutType := 'number';
    WHEN Eto_TDP_SMALLINT       THEN vOutType := 'number';
    WHEN Eto_TDP_INT            THEN vOutType := 'number';
    WHEN Eto_TDP_BIT            THEN vOutType := 'number';
    WHEN Eto_TDP_NUMERIC        THEN vOutType := 'number';
    WHEN Eto_TDP_BIGINT         THEN vOutType := 'number';
    WHEN Eto_TDP_USMALLINT      THEN vOutType := 'number';
    WHEN Eto_TDP_UINT           THEN vOutType := 'number';
    WHEN Eto_TDP_UBIGINT        THEN vOutType := 'number';
    WHEN Eto_TDP_USHORT         THEN vOutType := 'number';

    WHEN Eto_TDP_DATETIME       THEN vOutType := 'date';
    WHEN Eto_TDP_DATETIME4      THEN vOutType := 'date';
    WHEN Eto_TDP_DATE           THEN vOutType := 'date';
    WHEN Eto_TDP_TIME           THEN vOutType := 'date';

    WHEN Eto_TDP_SENSITIVITY    THEN vOutType := 'varchar2(4000)';
    WHEN Eto_TDP_BOUNDARY       THEN vOutType := 'varchar2(4000)';
    WHEN Eto_TDP_VOID           THEN vOutType := 'varchar2(4000)';
    WHEN Eto_TDP_ILLEGAL        THEN vOutType := 'varchar2(4000)';
    WHEN Eto_TDP_XML            THEN vOutType := 'varchar2(4000)';
    WHEN Eto_TDP_UNIQUE         THEN vOutType := 'varchar2(4000)';
    WHEN Eto_TDP_USER           THEN vOutType := 'varchar2(4000)';

    WHEN Eto_MYSQL_DECIMAL      THEN vOutType := 'number';
    WHEN Eto_MYSQL_TINY         THEN vOutType := 'number';
    WHEN Eto_MYSQL_SHORT        THEN vOutType := 'number';
    WHEN Eto_MYSQL_LONG         THEN vOutType := 'number';
    WHEN Eto_MYSQL_FLOAT        THEN vOutType := 'number';
    WHEN Eto_MYSQL_DOUBLE       THEN vOutType := 'number';
    WHEN Eto_MYSQL_NULL         THEN vOutType := 'varchar2(4000)';
    WHEN Eto_MYSQL_TIMESTAMP    THEN vOutType := 'date';
    WHEN Eto_MYSQL_LONGLONG     THEN vOutType := 'number';
    WHEN Eto_MYSQL_INT24        THEN vOutType := 'number';
    WHEN Eto_MYSQL_DATE         THEN vOutType := 'date';
    WHEN Eto_MYSQL_TIME         THEN vOutType := 'date';
    WHEN Eto_MYSQL_DATETIME     THEN vOutType := 'date';
    WHEN Eto_MYSQL_YEAR         THEN vOutType := 'number';
    WHEN Eto_MYSQL_NEWDATE      THEN vOutType := 'date';
    WHEN Eto_MYSQL_VARCHAR      THEN vOutType := 'varchar2(' || vCharLength || ')';
    WHEN Eto_MYSQL_BIT          THEN vOutType := 'number';
    WHEN Eto_MYSQL_NEWDECIMAL   THEN vOutType := 'number';
    WHEN Eto_MYSQL_ENUM         THEN vOutType := 'varchar2(4000)';
    WHEN Eto_MYSQL_SET          THEN vOutType := 'varchar2(4000)';
    WHEN Eto_MYSQL_TINY_BLOB    THEN vOutType := 'varchar2(4000)';
    WHEN Eto_MYSQL_MEDIUM_BLOB  THEN vOutType := 'varchar2(4000)';
    WHEN Eto_MYSQL_LONG_BLOB    THEN vOutType := 'varchar2(4000)';
    WHEN Eto_MYSQL_BLOB         THEN vOutType := 'varchar2(4000)';
    WHEN Eto_MYSQL_VAR_STRING   THEN vOutType := 'varchar2(' || vCharLength || ')';
    WHEN Eto_MYSQL_STRING       THEN vOutType := 'varchar2(' || vCharLength || ')';
    WHEN Eto_MYSQL_GEOMETRY     THEN vOutType := 'varchar2(4000)';

    WHEN Eto_XLS_TEXT           THEN vOutType := 'varchar2(4000)';
    WHEN Eto_XLS_NUMBER         THEN vOutType := 'number';
    WHEN Eto_XLS_DATETIME       THEN vOutType := 'date';
    WHEN Eto_XLS_DATE           THEN vOutType := 'date';
    WHEN Eto_XLS_TIME           THEN vOutType := 'varchar2(10)';

    WHEN Eto_SQLITE_INTEGER     THEN vOutType := 'number';
    WHEN Eto_SQLITE_FLOAT       THEN vOutType := 'number';
    WHEN Eto_SQLITE_TEXT        THEN vOutType := 'varchar2(4000)';
  --WHEN Eto_SQLITE_BLOB 

    WHEN Eto_MDB_BOOL           THEN vOutType := 'number';
    WHEN Eto_MDB_BYTE           THEN vOutType := 'number';
    WHEN Eto_MDB_INT            THEN vOutType := 'number';
    WHEN Eto_MDB_LONGINT        THEN vOutType := 'number';
    WHEN Eto_MDB_MONEY          THEN vOutType := 'number';
    WHEN Eto_MDB_FLOAT          THEN vOutType := 'number';
    WHEN Eto_MDB_DOUBLE         THEN vOutType := 'number';
    WHEN Eto_MDB_DATETIME       THEN vOutType := 'date';
    WHEN Eto_MDB_BINARY         THEN vOutType := 'varchar2(1)'; -- always returned as null --
    WHEN Eto_MDB_TEXT           THEN vOutType := 'varchar2(' || vCharLength || ')';
    WHEN Eto_MDB_OLE            THEN vOutType := 'varchar2(1)'; -- always returned as null --
    WHEN Eto_MDB_MEMO           THEN vOutType := 'varchar2(4000)';
    WHEN Eto_MDB_REPID          THEN vOutType := 'number';
    WHEN Eto_MDB_NUMERIC        THEN vOutType := 'number';

    WHEN Eto_PRESTO_VARCHAR     THEN vOutType := 'varchar2(' || vCharLength || ')';
    WHEN Eto_PRESTO_BIGINT      THEN vOutType := 'number';
    WHEN Eto_PRESTO_BOOLEAN     THEN vOutType := 'number';
    WHEN Eto_PRESTO_DOUBLE      THEN vOutType := 'number';

                                ELSE vOutType := 'varchar2(4000)';
  END CASE;

  Return vOutType;

  EXCEPTION
    WHEN others THEN
      Return 'varchar2(4000)';
END ETO_Translate_Type;

/* ---------------------------------------------------------------------------------------------------------------- */
FUNCTION ETO_Source_Type( p_Type IN pls_integer, p_Length IN pls_integer, p_Scale IN pls_integer, p_Precision IN pls_integer)
RETURN varchar2
AS
  vRewrite    boolean;
  vOutType    varchar2(60);
  vCharLength pls_integer;
BEGIN

  -- Is the default conversion overruled ? --
  vRewrite := false;

  IF gConv.count > 0 THEN
    FOR c IN gConv.FIRST .. gConv.LAST LOOP
      IF p_Type = gConv(c).T_Datatype THEN
        vOutType := gConv(c).T_Conversion;
        vRewrite := true;
        exit;
      END IF;
    END LOOP;
  END IF;

  IF not(vRewrite) THEN
    IF p_Length is NULL OR p_Length < 1 OR p_Length > 4000 THEN
      vCharLength := 4000;
    ELSE
      vCharLength := p_Length;
    END IF;

    CASE p_Type
      WHEN Eto_TDP_CHAR           THEN vOutType := 'char(' || vCharLength || ')';
      WHEN Eto_TDP_LONGCHAR       THEN vOutType := 'char(' || vCharLength || ')';
      WHEN Eto_TDP_TEXT           THEN vOutType := 'char(' || vCharLength || ')';
      WHEN Eto_TDP_VARCHAR        THEN vOutType := 'char(' || vCharLength || ')';
      WHEN Eto_TDP_LONG           THEN vOutType := 'char(' || vCharLength || ')';

      WHEN Eto_TDP_UNICHAR        THEN vOutType := 'char(' || vCharLength || ')';
      WHEN Eto_TDP_UNITEXT        THEN vOutType := 'char(' || vCharLength || ')';

    --WHEN Eto_TDP_BINARY         THEN vOutType := 'char(2000)'; -- max length = 2000 for raw type --
    --WHEN Eto_TDP_VARBINARY      THEN vOutType := 'char(2000)'; -- max length = 2000 for raw type --
    --WHEN Eto_TDP_LONGBINARY     THEN vOutType := 'char(2000)'; -- blob conversion isn't going to work --
    --WHEN Eto_TDP_IMAGE          THEN vOutType := 'char(2000)'; -- blob conversion isn't going to work --
    --WHEN Eto_TDP_BLOB           THEN vOutType := 'char(4000)'; -- blob conversion isn't going to work --

      WHEN Eto_TDP_REAL           THEN vOutType := 'float external(25)';
      WHEN Eto_TDP_FLOAT          THEN vOutType := 'float external(25)';
      WHEN Eto_TDP_MONEY          THEN vOutType := 'float external(25)';
      WHEN Eto_TDP_MONEY4         THEN vOutType := 'float external(25)';
      WHEN Eto_TDP_DECIMAL        THEN vOutType := 'float external(25)';

      WHEN Eto_TDP_TINYINT        THEN vOutType := 'unsigned integer external(10)';
      WHEN Eto_TDP_SMALLINT       THEN vOutType := 'unsigned integer external(10)';
      WHEN Eto_TDP_INT            THEN vOutType := 'unsigned integer external(10)';
      WHEN Eto_TDP_BIT            THEN vOutType := 'unsigned integer external(10)';
      WHEN Eto_TDP_NUMERIC        THEN vOutType := 'unsigned integer external(10)';
      WHEN Eto_TDP_BIGINT         THEN vOutType := 'unsigned integer external(10)';
      WHEN Eto_TDP_USMALLINT      THEN vOutType := 'unsigned integer external(10)';
      WHEN Eto_TDP_UINT           THEN vOutType := 'unsigned integer external(10)';
      WHEN Eto_TDP_UBIGINT        THEN vOutType := 'unsigned integer external(10)';
      WHEN Eto_TDP_USHORT         THEN vOutType := 'unsigned integer external(10)';

      WHEN Eto_TDP_DATETIME       THEN vOutType := 'char(20) date_format DATE mask "yyyy-mm-dd hh24:mi:ss"';
      WHEN Eto_TDP_DATETIME4      THEN vOutType := 'char(20) date_format DATE mask "yyyy-mm-dd hh24:mi:ss"';
      WHEN Eto_TDP_DATE           THEN vOutType := 'char(10) date_format DATE mask "yyyy-mm-dd"';
      WHEN Eto_TDP_TIME           THEN vOutType := 'char(10) date_format DATE mask "hh24:mi:ss"';

      WHEN Eto_TDP_SENSITIVITY    THEN vOutType := 'char(4000)';
      WHEN Eto_TDP_BOUNDARY       THEN vOutType := 'char(4000)';
      WHEN Eto_TDP_VOID           THEN vOutType := 'char(4000)';
      WHEN Eto_TDP_ILLEGAL        THEN vOutType := 'char(4000)';
      WHEN Eto_TDP_XML            THEN vOutType := 'char(4000)';
      WHEN Eto_TDP_UNIQUE         THEN vOutType := 'char(4000)';
      WHEN Eto_TDP_USER           THEN vOutType := 'char(4000)';

      WHEN Eto_MYSQL_DECIMAL      THEN vOutType := 'float external(25)';
      WHEN Eto_MYSQL_TINY         THEN vOutType := 'unsigned integer external(10)';
      WHEN Eto_MYSQL_SHORT        THEN vOutType := 'unsigned integer external(10)';
      WHEN Eto_MYSQL_LONG         THEN vOutType := 'unsigned integer external(10)';
      WHEN Eto_MYSQL_FLOAT        THEN vOutType := 'float external(25)';
      WHEN Eto_MYSQL_DOUBLE       THEN vOutType := 'float external(25)';
      WHEN Eto_MYSQL_NULL         THEN vOutType := 'char(4000)';
      WHEN Eto_MYSQL_TIMESTAMP    THEN vOutType := 'char(20) date_format DATE mask "yyyy-mm-dd hh24:mi:ss"';
      WHEN Eto_MYSQL_LONGLONG     THEN vOutType := 'unsigned integer external(10)';
      WHEN Eto_MYSQL_INT24        THEN vOutType := 'unsigned integer external(10)';
      WHEN Eto_MYSQL_DATE         THEN vOutType := 'char(20) date_format DATE mask "yyyy-mm-dd"';
      WHEN Eto_MYSQL_TIME         THEN vOutType := 'char(20) date_format DATE mask "hh24:mi:ss"';
      WHEN Eto_MYSQL_DATETIME     THEN vOutType := 'char(20) date_format DATE mask "yyyy-mm-dd hh24:mi:ss"';
      WHEN Eto_MYSQL_YEAR         THEN vOutType := 'unsigned integer external(10)';
      WHEN Eto_MYSQL_NEWDATE      THEN vOutType := 'char(20) date_format DATE mask "yyyy-mm-dd hh24:mi:ss"';
      WHEN Eto_MYSQL_VARCHAR      THEN vOutType := 'char(' || vCharLength || ')';
      WHEN Eto_MYSQL_BIT          THEN vOutType := 'unsigned integer external(10)';
      WHEN Eto_MYSQL_NEWDECIMAL   THEN vOutType := 'float external(25)';
      WHEN Eto_MYSQL_ENUM         THEN vOutType := 'char(4000)';
      WHEN Eto_MYSQL_SET          THEN vOutType := 'char(4000)';
      WHEN Eto_MYSQL_TINY_BLOB    THEN vOutType := 'char(4000)';
      WHEN Eto_MYSQL_MEDIUM_BLOB  THEN vOutType := 'char(4000)';
      WHEN Eto_MYSQL_LONG_BLOB    THEN vOutType := 'char(4000)';
      WHEN Eto_MYSQL_BLOB         THEN vOutType := 'char(4000)';
      WHEN Eto_MYSQL_VAR_STRING   THEN vOutType := 'char(' || vCharLength || ')';
      WHEN Eto_MYSQL_STRING       THEN vOutType := 'char(' || vCharLength || ')';
      WHEN Eto_MYSQL_GEOMETRY     THEN vOutType := 'char(4000)';

      WHEN Eto_XLS_TEXT           THEN vOutType := 'char(4000)';
      WHEN Eto_XLS_NUMBER         THEN vOutType := 'float external(25)';
      WHEN Eto_XLS_DATETIME       THEN vOutType := 'char(20) date_format date mask "J.SSSSS"';
      WHEN Eto_XLS_DATE           THEN vOutType := 'char(20) date_format date mask "J"';
      WHEN Eto_XLS_TIME           THEN vOutType := 'char(10)';

      WHEN Eto_SQLITE_INTEGER     THEN vOutType := 'float external(25)'; --'unsigned integer external(10)';
      WHEN Eto_SQLITE_FLOAT       THEN vOutType := 'float external(25)';
      WHEN Eto_SQLITE_TEXT        THEN vOutType := 'char(4000)';
    --WHEN Eto_SQLITE_BLOB

      WHEN Eto_MDB_BOOL           THEN vOutType := 'unsigned integer external(10)';
      WHEN Eto_MDB_BYTE           THEN vOutType := 'unsigned integer external(10)';
      WHEN Eto_MDB_INT            THEN vOutType := 'unsigned integer external(20)';
      WHEN Eto_MDB_LONGINT        THEN vOutType := 'unsigned integer external(20)';
      WHEN Eto_MDB_MONEY          THEN vOutType := 'float external(25)';
      WHEN Eto_MDB_FLOAT          THEN vOutType := 'float external(25)';
      WHEN Eto_MDB_DOUBLE         THEN vOutType := 'float external(25)';
      WHEN Eto_MDB_DATETIME       THEN vOutType := 'char(20) date_format DATE mask "yyyy-mm-dd hh24:mi:ss"';
      WHEN Eto_MDB_BINARY         THEN vOutType := 'char(1)'; -- always returned as null --
      WHEN Eto_MDB_TEXT           THEN vOutType := 'char(' || vCharLength || ')';
      WHEN Eto_MDB_OLE            THEN vOutType := 'char(1)'; -- always returned as null --
      WHEN Eto_MDB_MEMO           THEN vOutType := 'char(4000)';
      WHEN Eto_MDB_REPID          THEN vOutType := 'unsigned integer external(20)';
      WHEN Eto_MDB_NUMERIC        THEN vOutType := 'float external(25)';

      WHEN Eto_PRESTO_VARCHAR     THEN vOutType := 'char(' || vCharLength || ')';
      WHEN Eto_PRESTO_BIGINT      THEN vOutType := 'unsigned integer external(20)';
      WHEN Eto_PRESTO_BOOLEAN     THEN vOutType := 'unsigned integer external(10)';
      WHEN Eto_PRESTO_DOUBLE      THEN vOutType := 'float external(25)';

                                  ELSE vOutType := 'char(4000)';
    END CASE;
  END IF;

  Return vOutType;

  EXCEPTION
    WHEN others THEN
      Return 'char(4000)';
END ETO_Source_Type;

/* ---------------------------------------------------------------------------------------------------------------- */
FUNCTION ETO_Reserved_Word(pWord IN varchar2)
RETURN varchar2
AS
  CURSOR c_RWord(pW IN varchar2)
  IS
  SELECT count(*)
  FROM   V$RESERVED_WORDS
  WHERE  KEYWORD = pW; -- Note: not filtering on reserved=Y. This flag is not always filled correctly. Example: scale --

  vWord       varchar2(30);
  vCount      pls_integer;
BEGIN

  -- Check if name can be used as a column name --
  vWord := pWord;
  
  OPEN  c_RWord(upper(pWord));
  FETCH c_RWord INTO vCount;
  CLOSE c_RWord;

  IF vCount > 0 THEN
    vWord := substr('c_' || vWord, 1, 30);
  END IF;

  Return vWord;

  EXCEPTION
    WHEN others THEN
      Return pWord;
END ETO_Reserved_Word;

/* ---------------------------------------------------------------------------------------------------------------- */
FUNCTION ETO_Create_Table_Statement( p_IniName IN varchar2, pOutputTableName IN varchar2, pDropAllowed IN varchar2, p_executable IN varchar2
                                   , p_bin_dir IN varchar2, p_data_dir IN varchar2, pTableName IN varchar2, p_enable_logfiles IN varchar2
                                   , p_enable_badfiles IN varchar2, pCharacterset IN varchar2 := null, pReadsize IN varchar2 := null)
RETURN NUMBER
AS
  vUser      varchar2(30);
  vTablename varchar2(61);
  vDDL       varchar2(32000);
  vExists    pls_integer;
BEGIN

  vTablename := upper(pOutputTableName);
  
  -- Tablename has scheme ? --
  IF instr(vTablename, '.') <= 0 THEN
    SELECT SYS_CONTEXT('USERENV', 'SESSION_USER') INTO vUser FROM DUAL;
    vTablename := vUser || '.' || vTablename;
  END IF;

          vDDL := 'CREATE TABLE ' || vTablename || ' (' || chr(10);

  FOR i IN gDesc.FIRST .. gDesc.LAST LOOP
    IF gDesc.Exists(i) THEN
      gDesc(i).T_Name := ETO_Reserved_Word(gDesc(i).T_Name);
      vDDL := vDDL || '  ' || gDesc(i).T_Name || '  ' || ETO_Translate_Type(gDesc(i).T_Datatype, gDesc(i).T_Maxlength, gDesc(i).T_Scale, gDesc(i).T_Precision) || ',' || chr(10);
    END IF;
  END LOOP;
  vDDL := rtrim(rtrim(vDDL, chr(10)), ',');

  vDDL := vDDL || ')' || chr(10);
  vDDL := vDDL || 'ORGANIZATION EXTERNAL' || chr(10);
  vDDL := vDDL || '    ( TYPE              ORACLE_LOADER' || chr(10);
  vDDL := vDDL || '      DEFAULT DIRECTORY ' || p_data_dir || chr(10);
  vDDL := vDDL || '      ACCESS PARAMETERS' || chr(10);
  vDDL := vDDL || '      (' || chr(10);
  vDDL := vDDL || '          RECORDS DELIMITED   BY X''01''' || chr(10);

  IF pCharacterset is not null THEN
  vDDL := vDDL || '          CHARACTERSET ''' || pCharacterset || '''' || chr(10);
  END IF;

  vDDL := vDDL || '          PREPROCESSOR ' || p_bin_dir || ': ''' || p_executable || '''' || chr(10);

  IF nvl(p_enable_badfiles, 'N') = 'Y' THEN
  vDDL := vDDL || '          BADFILE ' || p_data_dir || ': ''' || pOutputTableName || '%a_%p.bad''' || chr(10);
  ELSE
  vDDL := vDDL || '          NOBADFILE' || chr(10);
  END IF;

  IF nvl(p_enable_logfiles, 'N') = 'Y' THEN
  vDDL := vDDL || '          LOGFILE ' || p_data_dir || ': ''' || pOutputTableName || '%a_%p.log''' || chr(10);
  ELSE
  vDDL := vDDL || '          NOLOGFILE' || chr(10);
  END IF;

  IF pReadsize is not null THEN
  vDDL := vDDL || '          READSIZE ' || pReadsize || chr(10);
  END IF;

  vDDL := vDDL || '          FIELDS TERMINATED   BY X''02''' || chr(10);
--vDDL := vDDL || '          OPTIONALLY ENCLOSED BY ''"''' || chr(10); -- removed in V1.1
  vDDL := vDDL || '          MISSING FIELD VALUES ARE NULL' || chr(10);
  vDDL := vDDL || '          ( ' || chr(10);

  FOR i IN gDesc.FIRST .. gDesc.LAST LOOP
    IF gDesc.Exists(i) THEN
      vDDL := vDDL || '            ' || gDesc(i).T_Name || '  ' || ETO_Source_Type(gDesc(i).T_Datatype, gDesc(i).T_Maxlength, gDesc(i).T_Scale, gDesc(i).T_Precision) || ',' || chr(10);
    END IF;
  END LOOP;
  vDDL := rtrim(rtrim(vDDL, chr(10)), ',');

  vDDL := vDDL || chr(10) || '          )' || chr(10);
  vDDL := vDDL || '      )' || chr(10);
  vDDL := vDDL || '    LOCATION (''' || p_IniName || ''')' || chr(10);
  vDDL := vDDL || '    )' || chr(10);
  vDDL := vDDL || 'NOPARALLEL' || chr(10);
  vDDL := vDDL || 'REJECT LIMIT UNLIMITED';

  -- Drop table --
  -- Check if describe table exists --
  select count(*) into vExists from ALL_TABLES where owner || '.' || table_name = vTablename;

  IF pDropAllowed = 'N' and vExists > 0 THEN
    Return -1;
  END IF;

  IF pDropAllowed = 'Y' and vExists > 0 THEN
    Execute immediate 'drop table ' || vTablename;
  END IF;

  -- Execute ddl --
  Execute immediate vDDL;

  Return 0;

  EXCEPTION
    WHEN others THEN
      Return -1;
END ETO_Create_Table_Statement;

/* ---------------------------------------------------------------------------------------------------------------- */
FUNCTION ETO_Create_Table(pOutputTableName IN varchar2, pConnectionName IN varchar2, pTableName IN varchar2, pDropAllowed IN varchar2 := 'N', pSQL IN varchar2 := null, pCharacterset IN varchar2 := null, pOtherParameters IN varchar2 := null)
RETURN NUMBER
AS
  vConn       type_ConnectionRecord;
  vSQL        varchar2(4000);
  vIniName    varchar2(200);
  vDBName     varchar2(4000);
  vCharset    varchar2(64);
  eNoConn     exception;
  eNoParams   exception;
  eFieldnames exception;
  eNoTable    exception;
BEGIN
  -- Init --
  gDesc := Null;
  gConv := Null;

  -- Get connection data --
  OPEN  cConnectionRecord(upper(pConnectionName));
  FETCH cConnectionRecord BULK COLLECT INTO vConn;
  CLOSE cConnectionRecord;

  IF vConn.COUNT = 0 THEN
    Raise eNoConn;
  END IF;

  -- check setup --
  IF vConn(1).executable is null OR
     vConn(1).bin_dir    is null OR
     vConn(1).data_dir   is null OR
     vConn(1).db_name    is null OR
     --vConn(1).db_user    is null OR
     --vConn(1).db_pwd     is null OR
     pTableName          is null
  THEN
    Raise eNoParams;
  END IF;

  -- Set characterset --
  IF pCharacterset is not null THEN
    vCharset := pCharacterset;
  ELSE
    vCharset := vConn(1).default_charset;   -- May be null --
  END IF;

  -- describe source --
  gDataPath := ETO_Get_Data_Path(vConn(1).data_dir);

  IF vConn(1).file_dir is not null THEN
    gFilesPath := ETO_Get_Data_Path(vConn(1).file_dir);
  ELSE
    gFilesPath := gDataPath;
  END IF;

  IF substr(vConn(1).executable, Eto_bin_type_position, 1) in ('O', 'S', 'A') THEN
    vDBName := gFilesPath || vConn(1).db_name;
  ELSE
    vDBName := vConn(1).db_name;
  END IF;

  IF ETO_Describe(upper(pConnectionName), vConn(1).executable, vConn(1).bin_dir, vConn(1).data_dir, vDBName, vConn(1).db_user, vConn(1).db_pwd, pTableName, pSQL, vConn(1).cache_rows, pTableName, pOtherParameters) <> 0 THEN
    Return -1; -- Errors are handled by describe so this should never occur --
  END IF;

  -- write ini file --
  -- first rewrite sql statement if necessary --
  IF pSQL is null THEN
    vSQL := ETO_Rewrite_SQL(pTableName);
  ELSE
    vSQL := pSQL;
  END IF;

  -- check for duplicate field names --
  IF ETO_Check_Fieldnames <> 0 THEN
    Raise eFieldnames;
  END IF;

  gDescribe_id := 0;
  
  -- Create a unique name for ini file --
  vIniName := replace(replace(pTableName, '"', ''), ' ','') || '.' || dbms_crypto.HASH(to_clob(vDBName || nvl(vSQL, pTableName)), dbms_crypto.HASH_MD5); -- Added databasename in V1.1 for better hashing of excel files --

  IF ETO_Write_ini(vIniName || '.ini', vConn(1).data_dir, vDBName, vConn(1).db_user, vConn(1).db_pwd, pTableName, vSQL, vConn(1).cache_rows, vIniName || '.log', pOtherParameters) <> 0 THEN
    Return -1; -- Errors are handled by ini writer so this should never occur --
  END IF;

  -- create table --
  IF ETO_Create_Table_Statement(vIniName || '.ini', pOutputTableName, pDropAllowed, vConn(1).executable, vConn(1).bin_dir, vConn(1).data_dir, pTableName, vConn(1).enable_logfiles, vConn(1).enable_badfiles, vCharset, vConn(1).readsize) <> 0 THEN
    Raise eNoTable;
  END IF;

  Return 0;

  EXCEPTION
    WHEN eNoConn THEN
      RAISE_APPLICATION_ERROR(-20004, 'ETO-20004 Unknown connection: ''' || upper(pConnectionName) || '''');
      Return -1;
    WHEN eNoParams THEN
      RAISE_APPLICATION_ERROR(-20004, 'ETO-20004 Not all necessary parameters have been set');
      Return -1;
    WHEN eFieldnames THEN
      RAISE_APPLICATION_ERROR(-20004, 'ETO-20004 Error checking fieldnames');
      Return -1;
    WHEN eNoTable THEN
      RAISE_APPLICATION_ERROR(-20004, 'ETO-20004 Error creating table');
      Return -1;
END ETO_Create_Table;

/* ---------------------------------------------------------------------------------------------------------------- */
FUNCTION ETO_List_Tables( pConnectionName IN varchar2, p_executable IN varchar2
                        , p_bin_dir IN varchar2, p_data_dir IN varchar2, p_db_name    IN varchar2
                        , p_db_user IN varchar2, p_db_pwd   IN varchar2, p_cache_rows IN varchar2
                        , p_list_table_query IN varchar2)
RETURN NUMBER
AS
  vExists          pls_integer;
  eCreateDescr     exception;
  eQueryDescr      exception;
  eDropError       exception;
BEGIN

  gDescribe_id := 0;

  -- create database 'list tables' table --
  IF ETO_Write_ini(Eto_List_Tables_ininame, p_data_dir, p_db_name, p_db_user, p_db_pwd, NULL, p_list_table_query, 1, Eto_List_Tables_logname) <> 0 THEN
    Return -1; -- Errors are handled by ini writer so this should never occur --
  END IF;

  -- Check if list_tables table exists --
  select count(*) into vExists from USER_ALL_TABLES where table_name = Eto_List_Tables_tablename;

  IF vExists > 0 THEN
    BEGIN
      Execute immediate 'DROP TABLE ' || Eto_List_Tables_tablename;
    EXCEPTION WHEN others THEN Raise eDropError;
    END;
  END IF;

  -- (Re)create table --
  IF ETO_Create_List_Table(p_executable, p_bin_dir, p_data_dir) <> 0 THEN
    Raise eCreateDescr;
  END IF;

  -- Run describe --
  -- Since describe table is created by this package, it is not possible to define a static cursor so we use dynamic sql --
  EXECUTE IMMEDIATE 'Select * from ' || Eto_List_Tables_tablename
  BULK COLLECT INTO gTableList;

  IF gTableList is null THEN
    Raise eQueryDescr;
  END IF;

  IF gTableList.count = 0 THEN
    Raise eQueryDescr;
  END IF;

  return 0;

  EXCEPTION
    WHEN eDropError THEN
      RAISE_APPLICATION_ERROR(-20008, 'ETO-20008 Error creating LIST_TABLES table. Drop ' || Eto_List_Tables_tablename || ' failed: ' || SQLERRM);
      Return -1;
    WHEN eCreateDescr THEN
      RAISE_APPLICATION_ERROR(-20008, 'ETO-20008 Error creating LIST_TABLES table. Create ' || Eto_List_Tables_tablename || ' failed: ' || SQLERRM);
      Return -1;
    WHEN eQueryDescr THEN
      RAISE_APPLICATION_ERROR(-20008, 'ETO-20008 Error creating LIST_TABLES table. ' || ETO_Read_Logfile_Contents(p_data_dir, Eto_List_Tables_logname) || ' : ' || SQLERRM);
      Return -1;
    WHEN others THEN
      RAISE_APPLICATION_ERROR(-20008, 'ETO-20008 Error creating LIST_TABLES table. ' || ETO_Read_Logfile_Contents(p_data_dir, Eto_List_Tables_logname) || ' : ' || SQLERRM);
      Return -1;
END ETO_List_Tables;

/* ---------------------------------------------------------------------------------------------------------------- */
FUNCTION FormatTableName(pName IN varchar2)
RETURN VARCHAR2
AS
  vName            varchar2(250);
  vParts           type_TableList := type_TableList(0);
  vCount           pls_integer;
  vBeforePos       pls_integer;
  i                pls_integer;
BEGIN
  vName := pName;

  -- Check if any part of the name contains spaces, if so enclose between " quotes --
  -- database.dbo.table name     ->      database.dbo."table name" --

  IF instr(vName, ' ') > 0 THEN
  
    -- Break name apart by . --
    vCount := 0;
    vBeforePos := 1;
    
    <<iteration>>
    FOR i IN 1..length(vName) LOOP
      IF substr(vName, i, 1) = '.' THEN

        vCount := vCount + 1;
        vParts.extend(1);
        vParts(vCount) := substr(vName, vBeforePos, i - vBeforePos);

        vBeforePos := i + 1;
      END IF;

      -- last . reached ? --
      IF instr(vName, '.', i + 1, 1) = 0 THEN
        vCount := vCount + 1;
        vParts.extend(1);
        vParts(vCount) := substr(vName, vBeforePos);
        exit iteration;
      END IF;
    END LOOP;

    vName := null;

    FOR i IN 1..vCount LOOP
      IF instr(vParts(i), ' ') > 0 THEN
        vParts(i) := '"' || vParts(i) || '"';
      END IF;
      
      vName := vName || vParts(i) || '.';
      
    END LOOP;

    vName := rtrim(vName, '.');

  END IF;

  Return vName;

  EXCEPTION
    WHEN others THEN
      Return pName;
END FormatTableName;

/* ---------------------------------------------------------------------------------------------------------------- */
FUNCTION ETO_Create_Tables_From_DB(pOutputSchema IN varchar2, pOutputPrefix IN varchar2 := null, pConnectionName IN varchar2, pDropAllowed IN varchar2 := 'N', pCharacterset IN varchar2 := null)
RETURN NUMBER
AS
  vConn            type_ConnectionRecord;
  vExists          pls_integer;
  vNrCreated       pls_integer;
  vNewOutputName   varchar2(250);
  vTable           varchar2(128);
  vSQL             varchar2(128);
  vDBName          varchar2(4000);
  eNoConn          exception;
  eNoSchema        exception;
  eNoParams        exception;
BEGIN

  -- Init --
  gDesc := Null;

  -- Check input --
  -- Does output schema exist ? --
  select count(*) INTO vExists from ALL_USERS where username = upper(pOutputSchema);

  IF vExists = 0 THEN
    Raise eNoSchema;
  END IF;

  -- Get connection data --
  OPEN  cConnectionRecord(upper(pConnectionName));
  FETCH cConnectionRecord BULK COLLECT INTO vConn; 
  CLOSE cConnectionRecord; 

  IF vConn.COUNT = 0 THEN
    Raise eNoConn;
  END IF;

  -- check setup --
  IF vConn(1).executable       is null OR
     vConn(1).bin_dir          is null OR
     vConn(1).data_dir         is null OR
     vConn(1).db_name          is null OR
     --vConn(1).db_user          is null OR
     --vConn(1).db_pwd           is null OR
     vConn(1).list_table_query is null
  THEN
    Raise eNoParams;
  END IF;

  -- Set up variables for list tables --
  gDataPath  := ETO_Get_Data_Path(vConn(1).data_dir);

  IF vConn(1).file_dir is not null THEN
    gFilesPath := ETO_Get_Data_Path(vConn(1).file_dir);
  ELSE
    gFilesPath := gDataPath;
  END IF;

  IF substr(vConn(1).executable, Eto_bin_type_position, 1) in ('O', 'S', 'A') THEN
    vDBName := gFilesPath || vConn(1).db_name;
  ELSE
    vDBName := vConn(1).db_name;
  END IF;

  -- list tables at remote database --
  IF ETO_List_Tables(pConnectionName, vConn(1).executable, vConn(1).bin_dir, vConn(1).data_dir, vDBName, vConn(1).db_user, vConn(1).db_pwd, vConn(1).cache_rows, vConn(1).list_table_query) <> 0 THEN
    Return -1; -- Errors are handled by function so this should never occur --
  END IF;

  -- Loop through tables and create them -- gDesc and vConn are invalid after calling ETO_Create_Table --
  vNrCreated := 0;
  FOR i IN gTableList.FIRST .. gTableList.LAST LOOP

    vNewOutputName := replace(gTableList(i), ' ', '_');                            -- no spaces
    vNewOutputName := substr(vNewOutputName, instr(vNewOutputName, '.', -1) + 1);  -- from last .
    vNewOutputName := substr(pOutputPrefix || vNewOutputName, 1, 30);              -- max 30 chars (This is not necessarily a unique name, existing external table will be overwritten !)
    vNewOutputName := upper(pOutputSchema) || '.' || vNewOutputName;               -- add schema

    IF nvl(vConn(1).do_not_quote_names, 'N') = 'Y' THEN
      vTable := FormatTableName(gTableList(i));
    ELSE
      vTable := gTableList(i);
    END IF;
    vSQL := null;

    BEGIN
      vExists    := ETO_Create_Table(vNewOutputName, pConnectionName, vTable, pDropAllowed, vSQL, pCharacterset);
      vNrCreated := vNrCreated + 1;
    EXCEPTION WHEN others THEN NULL;  -- Skip table with an error and try next table --
    END;
  END LOOP;

  gDataPath  := null;
  gFilesPath := null;

  Return vNrCreated;

  EXCEPTION
    WHEN eNoConn THEN
      RAISE_APPLICATION_ERROR(-20009, 'ETO-20009 Unknown connection: ''' || upper(pConnectionName) || '''');
      Return -1;
    WHEN eNoSchema THEN
      RAISE_APPLICATION_ERROR(-20009, 'ETO-20009 Schema ' || upper(pOutputSchema) || ' does not exist');
      Return -1;
     WHEN eNoParams THEN
      RAISE_APPLICATION_ERROR(-20009, 'ETO-20009 Not all necessary parameters have been set');
      Return -1;
   WHEN others THEN
      RAISE_APPLICATION_ERROR(-20009, 'ETO-20009 Unknown error: ' || SQLERRM);
      Return -1;
END ETO_Create_Tables_From_DB;

END UTL_ETO;
/

grant execute on SYS.UTL_ETO to public;
/

CREATE OR REPLACE PUBLIC SYNONYM UTL_ETO FOR SYS.UTL_ETO;
/