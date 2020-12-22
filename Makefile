
TARGET=MonoReversi
LINK=link.exe

OUTDIR=.\build
AI_OUTDIR=.\build\ai
SEARCH_OUTDIR=.\build\search

SRC_DIR=.\src
AI_SRC_DIR=.\src\ai
SEARCH_DIR=.\src\search

INCLUDE_PATH=.\src

OBJS=\
	$(OUTDIR)\const.obj\
	$(OUTDIR)\bit_operation.obj\
	$(OUTDIR)\board.obj\
	$(OUTDIR)\game.obj\
	$(OUTDIR)\main.obj\
	$(AI_OUTDIR)\mcts.obj\
	$(AI_OUTDIR)\node.obj\
	$(AI_OUTDIR)\model.obj\
	$(SEARCH_OUTDIR)\eval.obj\
	$(SEARCH_OUTDIR)\ab_node.obj\
	$(SEARCH_OUTDIR)\bench.obj\
	$(SEARCH_OUTDIR)\search.obj


all: $(OUTDIR)\$(TARGET).exe

clean:
	-@erase /Q $(OUTDIR)\* $(AI_OUTDIR)\* $(SEARCH_OUTDIR)\*

$(OUTDIR):
	@if not exist $(OUTDIR) mkdir $(OUTDIR)

$(AI_OUTDIR):
	@if not exist $(AI_OUTDIR) mkdir $(AI_OUTDIR)

$(SEARCH_OUTDIR):
	@if not exist $(SEARCH_OUTDIR) mkdir $(SEARCH_OUTDIR)


CFLAGS=\
	/nologo\
	/W3\
	/c\
	/Zi\
	/D_WIN32_WINNT=0x0600\
	/DUNICODE\
	/D_UNICODE\
	/source-charset:utf-8\
	/USE_SSE\
	/USE_AVX\
	/USE_AVX2\
	/USE_CUDA\
	/bigobj\
	/EHsc\
	/I$(INCLUDE_PATH)
	# /USE_SERIALIZER\ #

LINK_FLAGS=\
	/nologo\
	/subsystem:console\
	/pdb:"$(OUTDIR)\$(TARGET).pdb"\
	/out:"$(OUTDIR)\$(TARGET).exe"\
	/DEBUG


$(OUTDIR)\$(TARGET).exe: $(OUTDIR) $(AI_OUTDIR) $(SEARCH_OUTDIR) $(OBJS)
	$(LINK) $(LINK_FLAGS) $(OBJS)

{$(SRC_DIR)}.cpp{$(OUTDIR)}.obj:
	$(CPP) $(CFLAGS) /Fo"$(OUTDIR)\\" /Fd"$(OUTDIR)\\" $<

{$(AI_SRC_DIR)}.cpp{$(AI_OUTDIR)}.obj:
	$(CPP) $(CFLAGS) /Fo"$(AI_OUTDIR)\\" /Fd"$(AI_OUTDIR)\\" $<
	
{$(SEARCH_DIR)}.cpp{$(SEARCH_OUTDIR)}.obj:
	$(CPP) $(CFLAGS) /Fo"$(SEARCH_OUTDIR)\\" /Fd"$(SEARCH_OUTDIR)\\" $<