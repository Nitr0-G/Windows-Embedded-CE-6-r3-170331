!if 0
Copyright (c) Microsoft Corporation.  All rights reserved.
!endif
!if 0
Use of this source code is subject to the terms of the Microsoft shared
source or premium shared source license agreement under which you licensed
this source code. If you did not accept the terms of the license agreement,
you are not authorized to use this source code. For the terms of the license,
please see the license agreement between you and Microsoft or, if applicable,
see the SOURCE.RTF on your install media or the root of your tools installation.
THE SOURCE CODE IS PROVIDED "AS IS", WITH NO WARRANTIES.
!endif
###############################################################################
#
#   This makefile copies files to the %_RELEASEDIR% directory, and
#   to the %_FLATRELEASEDIR% directory if WINCEREL is set to "1".
#
###############################################################################

copy_xml:
    xcopy CETK_playback_tests.xml $(_RELEASEDIR) /Y /R /K /F

!if "$(WINCEREL)"=="1"
    xcopy CETK_playback_tests.xml $(_FLATRELEASEDIR) /Y /R /K /F
!endif