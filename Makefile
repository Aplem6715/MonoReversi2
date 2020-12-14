
TARGET=MonoReversi
OUTDIR=.\build
AI_OUTDIR=.\build\ai
LINK=link.exe

SRC_DIR=.\src
AI_SRC_DIR=.\src\ai
INCLUDE_PATH=.\src

OBJS=\
	$(OUTDIR)\const.obj\
	$(OUTDIR)\bit_operation.obj\
	$(OUTDIR)\board.obj\
	$(OUTDIR)\game.obj\
	$(OUTDIR)\main.obj\
	$(AI_OUTDIR)\mcts.obj\
	$(AI_OUTDIR)\node.obj\


all: $(OUTDIR)\$(TARGET).exe

clean:
	-@erase /Q $(OUTDIR)\* $(AI_OUTDIR)

$(OUTDIR):
	@if not exist $(OUTDIR) mkdir $(OUTDIR)

$(AI_OUTDIR):
	@if not exist $(AI_OUTDIR) mkdir $(AI_OUTDIR)


CFLAGS=\
	/nologo\
	/W3\
	/c\
	/Zi\
	/D_WIN32_WINNT=0x0600\
	/DUNICODE\
	/D_UNICODE\
	/EHsc\
	/I$(INCLUDE_PATH)

LINK_FLAGS=\
	/nologo\
	/subsystem:console\
	/pdb:"$(OUTDIR)\$(TARGET).pdb"\
	/out:"$(OUTDIR)\$(TARGET).exe"\
	/DEBUG


$(OUTDIR)\$(TARGET).exe: $(OUTDIR) $(AI_OUTDIR) $(OBJS)
	$(LINK) $(LINK_FLAGS) $(OBJS)

{$(SRC_DIR)}.cpp{$(OUTDIR)}.obj:
	$(CPP) $(CFLAGS) /Fo"$(OUTDIR)\\" /Fd"$(OUTDIR)\\" $<

{$(AI_SRC_DIR)}.cpp{$(AI_OUTDIR)}.obj:
	$(CPP) $(CFLAGS) /Fo"$(AI_OUTDIR)\\" /Fd"$(AI_OUTDIR)\\" $<