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
THE SOURCE CODE IS PROVIDED "AS IS", WITH NO WARRANTIES OR INDEMNITIES.
!endif

TARGETTYPE=LIBRARY
PREPROCESSDEFFILE=1
SYNCHRONIZE_DRAIN=1

COMMON_CDEFINES=-DWINCEMACRO -DCOREDLL
COMMON_INCLUDES=..\..\inc;..\..\..\inc;..\..\..\nk\inc;..\..\fpcrt;..\..\..\gwe\inc

NOMIPS16CODE=1

CONDITIONAL_INCLUDES=shx\shx.h

!IF "$(_TGTCPU)" == "SH4"
ADEFINES=$(ADEFINES) -DSH_CPU=64
!ENDIF

CFPUDEFINES=-DMIPS_HAS_FPU

ASM_SAFESEH=1

SOURCELIBS= \
    $(_COMMONOAKROOT)\lib\$(_CPUINDPATH)\ccrtsecd.lib

COMMON_SOURCES= \
    ..\apis.c     \
    ..\UsrCoredllCallbacks.c \
    ..\UsrGweCallbacks.c \
    ..\coredll.cpp  \
    ..\time.c     \
    ..\random.c   \
    ..\strsafea.c \
    ..\strsafea_sprintf.c \
    ..\strsafew.c \
    ..\username.cpp \
    ..\CePtrCoredll.cpp \
    ..\cnnclpth.c \
    ..\exdsptch.c \
    ..\resource.cpp \
    ..\loader.cpp \
    ..\securewipe.c \
    ..\AppVerifier.cpp \
    ..\wsprintf.c \
    ..\imm.cpp \
    ..\gscore.cpp \
    ..\crtsupp.cpp \
    ..\AutoGestures.cpp \

ARM_SOURCES= \
    ..\arm\intrlock.s \
    ..\arm\unwind.c \
    ..\arm\rtlsup.s \
    ..\muldiv.c \

MIPS_SOURCES= \
    ..\mips\intrlock.s \
    ..\mips\mipsuwnd.c \
    ..\mips\rtlsup.s \
    ..\muldiv.c \

SH_SOURCES= \
    ..\sh\intrlock.src \
    ..\sh\shunwind.c \
    ..\sh\rtlsup.src \
    ..\muldiv.c \

x86_SOURCES= \
    ..\x86\intrlock.c \
    ..\x86\rtlsup.c \
    ..\x86\rtlsupsafeseh.asm \
    ..\x86\muldiv.asm \


#xref ModuleMembership coredll.dll
#xref VIGUID {c01aec23-e12b-45f2-94bb-865e927c536f}
#xref VSGUID {4a9f4e8a-90c9-4c7b-8be2-c3acfdf520f8}
