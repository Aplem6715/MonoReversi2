
TARGET			=MonoReversi
LEARN_TARGET	=learn
LINK			=link.exe

OUTDIR			=.\build
AI_OUTDIR		=.\build\ai
SEARCH_OUTDIR	=.\build\search
LEARN_OUTDIR	=.\build\learning

SRC_DIR		=.\src
AI_SRC_DIR	=.\src\ai
SEARCH_DIR	=.\src\search
LEARN_DIR	=.\src\learning

INCLUDE_PATH=.\src

OBJS=\
	$(OUTDIR)\const.obj\
	$(OUTDIR)\bit_operation.obj\
	$(OUTDIR)\board.obj\
	$(OUTDIR)\game.obj\
	$(OUTDIR)\main.obj\
	$(AI_OUTDIR)\eval.obj\
	$(LEARN_OUTDIR)\nnet.obj\
	$(LEARN_OUTDIR)\regression.obj\
	$(SEARCH_OUTDIR)\bench.obj\
	$(SEARCH_OUTDIR)\hash.obj\
	$(SEARCH_OUTDIR)\moves.obj\
	$(SEARCH_OUTDIR)\search.obj


LEARN_OBJS=\
	$(OUTDIR)\const.obj\
	$(OUTDIR)\bit_operation.obj\
	$(OUTDIR)\board.obj\
	$(SEARCH_OUTDIR)\hash.obj\
	$(SEARCH_OUTDIR)\moves.obj\
	$(SEARCH_OUTDIR)\search.obj\
	$(AI_OUTDIR)\eval.obj\
	$(LEARN_OUTDIR)\game_record.obj\
	$(LEARN_OUTDIR)\nnet.obj\
	$(LEARN_OUTDIR)\regression.obj\
	$(LEARN_OUTDIR)\learn_eval.obj\
	$(LEARN_OUTDIR)\learner.obj

	
CFLAGS=\
	/nologo\
	/W3\
	/c\
	/D_WIN32_WINNT=0x0600\
	/DUNICODE\
	/D_UNICODE\
	/source-charset:utf-8\
	/USE_SSE\
	/USE_AVX\
	/USE_AVX2\
	/USE_CUDA\
	/USE_OPENCL\
	/bigobj\
	/EHsc\
	/DUSE_REGRESSION\
	/I$(INCLUDE_PATH)\
	/Zi\
	/DLEARN_MODE
	# /USE_SERIALIZER\ #

LINK_FLAGS=\
	/nologo\
	/subsystem:console\
	/pdb:"$(OUTDIR)\$(TARGET).pdb"\
	/out:"$(OUTDIR)\$(TARGET).exe"\
	/DEBUG

LEARN_LINK_FLAGS=\
	/nologo\
	/subsystem:console\
	/pdb:"$(OUTDIR)\$(LEARN_TARGET).pdb"\
	/out:"$(OUTDIR)\$(LEARN_TARGET).exe"\
	/DEBUG


reversi: $(OUTDIR)\$(TARGET).exe

learn: $(OUTDIR)\$(LEARN_TARGET).exe

all: reversi learn

.PHONY: clean
clean:
	-@erase /Q $(OUTDIR)\* $(AI_OUTDIR)\* $(SEARCH_OUTDIR)\* $(LEARN_OUTDIR)\*



$(OUTDIR):
	@if not exist $(OUTDIR) mkdir $(OUTDIR)

$(AI_OUTDIR):
	@if not exist $(AI_OUTDIR) mkdir $(AI_OUTDIR)

$(SEARCH_OUTDIR):
	@if not exist $(SEARCH_OUTDIR) mkdir $(SEARCH_OUTDIR)

$(LEARN_OUTDIR):
	@if not exist $(LEARN_OUTDIR) mkdir $(LEARN_OUTDIR)


$(OUTDIR)\$(TARGET).exe: $(OUTDIR) $(AI_OUTDIR) $(SEARCH_OUTDIR) $(OBJS)
	$(LINK) $(LINK_FLAGS) $(OBJS)

$(OUTDIR)\$(LEARN_TARGET).exe: $(OUTDIR) $(AI_OUTDIR) $(SEARCH_OUTDIR) $(LEARN_OUTDIR) $(LEARN_OBJS)
	$(LINK) $(LEARN_LINK_FLAGS) $(LEARN_OBJS)

{$(SRC_DIR)}.cpp{$(OUTDIR)}.obj:
	$(CPP) $(CFLAGS) /Fo"$(OUTDIR)\\" /Fd"$(OUTDIR)\\" $<

{$(AI_SRC_DIR)}.cpp{$(AI_OUTDIR)}.obj:
	$(CPP) $(CFLAGS) /Fo"$(AI_OUTDIR)\\" /Fd"$(AI_OUTDIR)\\" $<
	
{$(SEARCH_DIR)}.cpp{$(SEARCH_OUTDIR)}.obj:
	$(CPP) $(CFLAGS) /Fo"$(SEARCH_OUTDIR)\\" /Fd"$(SEARCH_OUTDIR)\\" $<

{$(LEARN_DIR)}.cpp{$(LEARN_OUTDIR)}.obj:
	$(CPP) $(CFLAGS) /Fo"$(LEARN_OUTDIR)\\" /Fd"$(LEARN_OUTDIR)\\" $<