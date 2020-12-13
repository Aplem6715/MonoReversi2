
TARGET=MonoReversi
OUTDIR=.\build
SRC_DIR=.\src
LINK=link.exe

OBJS=\
	$(OUTDIR)\const.obj\
	$(OUTDIR)\bit_operation.obj\
	$(OUTDIR)\board.obj\
	$(OUTDIR)\mcts.obj\
	$(OUTDIR)\game.obj\
	$(OUTDIR)\util.obj\
	$(OUTDIR)\main.obj

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

{$(SRC_DIR)}.cpp{$(OUTDIR)}.obj:
	$(CPP) $(CFLAGS) $<
