Easy ... To Oracle - Presto
===========================

#### Executable name:             EasySTOra
#### Used library:                prestoclient
#### Minimum library version:     0.3.0
#### Recommended library version: 0.3.0+

Preparations
------------
*Red-Hat/Centos/Oracle 5 & 6:*  
Install CURL
	sudo yum install libcurl libcurl-devel

To compile EasyPTOra you will need gcc version 4.6.0 or higher. Otherwise you will get
typedef redefinition errors. RHEL 5 and 6 use older gcc versions. To install a newer
version, RedHat provides the devtoolset. In this example code the CentOS version is shown.

	wget http://people.centos.org/tru/devtools/devtools.repo
	sudo mv devtools.repo /etc/yum.repos.d
	sudo chown root:root /etc/yum.repos.d/devtools.repo
	sudo chmod 644 /etc/yum.repos.d/devtools.repo
	sudo yum clean all
	sudo yum install devtoolset-1.0
	
	# Start the devtoolset
	scl enable devtoolset-1.0 'bash'
	gcc -v
	
	# Now you can build with
	# make

On RHEL 7.0 (Beta) there are no such problems. EasyPTOra builds out-of-the-box.

*Ubuntu:*  

	sudo apt-get install curl libcurl4-openssl-dev

*Windows:*  
Download an appropriate version of cURL: http://curl.haxx.se/download.html  
Copy the debug version of libcurl.dll to: msvc/EasyPTOra/Debug  
header files to: EasyPTOra/prestoclient/curl/include/curl  
and libcurl.lib files to: EasyPTOra/prestoclient/curl/lib/Debug  
Do the same for the release versions, but to the Release folders.  

Building EasyPTO
----------------
See the general installation instructions.

Specific usage information
--------------------------
None

Test scripts
------------
	DECLARE
	  r   pls_integer;
	BEGIN
	  r := SYS.UTL_ETO.ETO_Create_Database_Connection('PRESTO');
	  r := SYS.UTL_ETO.ETO_Set_Parameter('PRESTO', SYS.UTL_ETO.Eto_par_executable,       'EasyPTOra');
	  r := SYS.UTL_ETO.ETO_Set_Parameter('PRESTO', SYS.UTL_ETO.Eto_par_db_name,          'servername:port');  -- Presto uses 8080 as default tcp port
	  r := SYS.UTL_ETO.ETO_Set_Parameter('PRESTO', SYS.UTL_ETO.Eto_par_bin_dir,          'user_dir_eto_bin');
	  r := SYS.UTL_ETO.ETO_Set_Parameter('PRESTO', SYS.UTL_ETO.Eto_par_data_dir,         'user_dir_eto_data');
	  r := SYS.UTL_ETO.ETO_Set_Parameter('PRESTO', SYS.UTL_ETO.Eto_par_def_cacherows,    '1');
	  r := SYS.UTL_ETO.ETO_Set_Parameter('PRESTO', SYS.UTL_ETO.Eto_par_enable_logfiles,  'Y');
	  r := SYS.UTL_ETO.ETO_Set_Parameter('PRESTO', SYS.UTL_ETO.Eto_par_enable_badfiles,  'Y');
	  r := SYS.UTL_ETO.ETO_Set_Parameter('PRESTO', SYS.UTL_ETO.Eto_par_default_charset,  'AL32UTF8');
	  r := SYS.UTL_ETO.ETO_Set_Parameter('PRESTO', SYS.UTL_ETO.Eto_par_list_table_query, 'SHOW TABLES');

	  r := SYS.UTL_ETO.ETO_Create_Tables_From_DB
		( pOutputSchema   => 'SCHEMA' -- replace
		, pOutputPrefix   => 'PRESTO_'
		, pConnectionname => 'PRESTO'
		, pDropAllowed    => 'Y'
	  );
	END;


-------------------------------------
Copyright (C) 2011-2014 Ivo Herweijer
