
TARGET=MonoReversi
OUTDIR=.\build
LINK=link.exe

OBJS=\
	$(OUTDIR)\main.obj\
	$(OUTDIR)\util.obj

all: $(OUTDIR)\$(TARGET).exe

clean:
	-@erase /Q $(OUTDIR)\*

$(OUTDIR):
	@if not exist $(OUTDIR) mkdir $(OUTDIR)


CFLAGS=\
	/nologo\
	/W3\
	/Fo"$(OUTDIR)\\"\
	/Fd"$(OUTDIR)\\"\
	/c\
	/Zi\
	/D_WIN32_WINNT=0x0600\
	/DUNICODE\
	/D_UNICODE

LINK_FLAGS=\
	/nologo\
	/subsystem:console\
	/pdb:"$(OUTDIR)\$(TARGET).pdb"\
	/out:"$(OUTDIR)\$(TARGET).exe"\
	/DEBUG


$(OUTDIR)\$(TARGET).exe: $(OUTDIR) $(OBJS)
	$(LINK) $(LINK_FLAGS) $(OBJS)

.cpp{$(OUTDIR)}.obj:
	$(CPP) $(CFLAGS) $<