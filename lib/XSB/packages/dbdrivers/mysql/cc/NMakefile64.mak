# Make file for mysql_driver.dll

# !!!!
# !!!! Replace with a correct path to MySQL!!!
#MySQLLib="<Insert Proper Path>\mysqlclient.lib"
#MySQLIncludeDir="<Insert Proper Path>"
# !!!! You may need to copy the mysql library files and header files to
# !!!! directories with no space in paths
# !!!! The following commented path doesn't work!
#MySQLLib="C:\Program Files\MySQL\MySQL Server 6.0\lib\opt\mysqlclient.lib"
#MySQLIncludeDir="C:\Program Files\MySQL\MySQL Server 6.0\include"

XSBDIR=..\..\..\..
MYPROGRAM=mysql_driver

CPP=cl.exe
OUTDIR=$(XSBDIR)\config\x64-pc-windows
OUTBINDIR=$(OUTDIR)\bin
OUTOBJDIR=$(OUTDIR)\saved.o
INTDIR=.

ALL : "$(OUTBINDIR)\$(MYPROGRAM).dll"

CLEAN :
	-@if exist "$(INTDIR)\*.obj" erase "$(INTDIR)\*.obj"
	-@if exist "$(INTDIR)\*.dll" erase "$(INTDIR)\*.dll"
	-@if exist "$(INTDIR)\*.exp" erase "$(INTDIR)\*.exp"


CPP_PROJ=/nologo /MT /W3 /EHsc /O2 /I "$(OUTDIR)" \
		 /I "$(XSBDIR)\emu" /I "$(XSBDIR)\prolog_includes" \
		 /I "$(XSBDIR)\packages\dbdrivers\cc" \
		 /I "$(MySQLIncludeDir)" \
		 /D "WIN64" /D "WIN_NT" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" \
		 /Fo"$(OUTOBJDIR)\\" /Fd"$(OUTOBJDIR)\\" /c 
	
SOURCE=$(MYPROGRAM).c
"$(OUTOBJDIR)\$(MYPROGRAM).obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)

LINK32=link.exe
LINK32_FLAGS=kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib \
		advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib \
		odbc32.lib odbccp32.lib \
		WS2_32.lib \
		driver_manager.lib $(MySQLLib) \
		/nologo /dll \
		/machine:x64 /out:"$(OUTBINDIR)\$(MYPROGRAM).dll" \
		/libpath:"$(OUTBINDIR)"
LINK32_OBJS=  "$(OUTOBJDIR)\$(MYPROGRAM).obj"

"$(OUTBINDIR)\$(MYPROGRAM).dll" : "$(OUTBINDIR)" $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<
