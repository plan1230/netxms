# Copyright (C) 2004-2019 NetXMS Team <bugs@netxms.org>
#  
# This file is free software; as a special exception the author gives
# unlimited permission to copy and/or distribute it, with or without 
# modifications, as long as this notice is preserved.
# 
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY, to the extent permitted by law; without even the
# implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

bin_SCRIPTS = nx-collect-server-diag
EXTRA_DIST = nx-collect-server-diag.in nx-collect-server-diag.cmd zip.ps1

REPLACE_CMD = sed "s,~BINDIR~,@NETXMS_BINDIR@,g" < $@.in > $@

nx-collect-server-diag: nx-collect-server-diag.in
	$(REPLACE_CMD)

clean-local:
	rm -f nx-collect-server-diag
