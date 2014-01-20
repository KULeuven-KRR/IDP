## File:      packages/pcre/cc/NMakefile.mak
## Author(s): Mandar Pathak
## Contact:   xsb-contact@cs.sunysb.edu
## 
## Copyright (C) The Research Foundation of SUNY, 2010
## 
## XSB is free software; you can redistribute it and/or modify it under the
## terms of the GNU Library General Public License as published by the Free
## Software Foundation; either version 2 of the License, or (at your option)
## any later version.
## 
## XSB is distributed in the hope that it will be useful, but WITHOUT ANY
## WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
## FOR A PARTICULAR PURPOSE.  See the GNU Library General Public License for
## more details.
## 
## You should have received a copy of the GNU Library General Public License
## along with XSB; if not, write to the Free Software Foundation,
## Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
##

# Make file for pcre4pl.dll

XSBDIR=..\..\..
MYPROGRAM=pcre4pl

CC=cl.exe
OUTDIR=$(XSBDIR)\config\x64-pc-windows\bin
INTDIR=.

ALL : "$(OUTDIR)\$(MYPROGRAM).dll"
	nmake /f NMakefile64.mak clean

CLEAN :
	-@if exist "$(INTDIR)\$(MYPROGRAM).obj" erase "$(INTDIR)\$(MYPROGRAM).obj"
	-@if exist "$(INTDIR)\$(MYPROGRAM).dll" erase "$(INTDIR)\$(MYPROGRAM).dll"
	-@if exist "$(INTDIR)\$(MYPROGRAM).exp" erase "$(INTDIR)\$(MYPROGRAM).exp"

##"$(OUTDIR)" :
##    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP_PROJ=/nologo /MT /W3 /EHsc /O2 /I "$(XSBDIR)\config\x64-pc-windows" \
		 /I "$(XSBDIR)\emu" /I "$(XSBDIR)\prolog_includes" /I "$(XSBDIR)\packages\pcre\cc\pcre" \
		/D "WIN_NT" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" \
		/Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /c 
	

SOURCE=$(MYPROGRAM).c

"$(INTDIR)\$(MYPROGRAM).obj" : $(SOURCE) "$(INTDIR)"
	$(CC) $(CPP_PROJ) $(SOURCE)

LINK32=link.exe
LINK32_FLAGS=kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib \
		advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib \
		odbc32.lib odbccp32.lib xsb.lib pcre.lib \
		/nologo /dll \
		/machine:x64 /out:"$(OUTDIR)\$(MYPROGRAM).dll" \
		/libpath:"$(XSBDIR)\config\x64-pc-windows\bin" \
		/libpath:"$(XSBDIR)\prolog_includes" \
		/libpath:.\bin64

LINK32_OBJS=  "$(INTDIR)\$(MYPROGRAM).obj"

## "$(OUTDIR)\$(MYPROGRAM).dll" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)

"$(OUTDIR)\$(MYPROGRAM).dll" : "$(OUTDIR)" $(LINK32_OBJS)
    $(LINK32) $(LINK32_FLAGS) $(LINK32_OBJS)
