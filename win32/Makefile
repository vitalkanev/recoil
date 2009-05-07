FAIL_CC = cl /nologo /O2 /GL /GR- /GS- /W3 /wd4996 /DNDEBUG /Fe$@ /I ..
LINKOPT = /link /release
PERL = perl

all: fail2png.exe

fail2png.exe: ..\fail2png.c ..\pngsave.c ..\fail.c ..\pngsave.h ..\fail.h ..\palette.h
	$(FAIL_CC) ..\fail2png.c ..\pngsave.c ..\fail.c libpng.lib zlib.lib $(LINKOPT)

..\palette.h: ..\raw2c.pl ..\jakub.act
	$(PERL) ..\raw2c.pl ..\jakub.act >$@

clean:
	del fail2png.exe
	del *.obj
	del ..\palette.h

