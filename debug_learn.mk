
TARGET			=MonoReversi
LEARN_TARGET	=learn
LINK			=link.exe

OUTDIR			=.\build\debug\learn

AI_OUTDIR		=$(OUTDIR)\ai
SEARCH_OUTDIR	=$(OUTDIR)\search
LEARN_OUTDIR	=$(OUTDIR)\learning

SRC_DIR		=.\src
AI_SRC_DIR	=.\src\ai
SEARCH_DIR	=.\src\search
LEARN_DIR	=.\src\learning

INCLUDE_PATH=.\src

LEARN_OBJS=\
	$(OUTDIR)\const.o\
	$(OUTDIR)\bit_operation.o\
	$(OUTDIR)\game.o\
	$(OUTDIR)\board.o\
	$(SEARCH_OUTDIR)\random_util.o\
	$(SEARCH_OUTDIR)\hash.o\
	$(SEARCH_OUTDIR)\moves.o\
	$(SEARCH_OUTDIR)\mpc_info.o\
	$(SEARCH_OUTDIR)\mid.o\
	$(SEARCH_OUTDIR)\end.o\
	$(SEARCH_OUTDIR)\search.o\
	$(SEARCH_OUTDIR)\search_manager.o\
	$(AI_OUTDIR)\eval.o\
	$(AI_OUTDIR)\ai_const.o\
	$(AI_OUTDIR)\nnet.o\
	$(AI_OUTDIR)\regression.o\
	$(LEARN_OUTDIR)\game_record.obj\
	$(LEARN_OUTDIR)\nnet_trainer.obj\
	$(LEARN_OUTDIR)\regr_trainer.obj\
	$(LEARN_OUTDIR)\ascii_loader.obj\
	$(LEARN_OUTDIR)\learn_const.obj\
	$(LEARN_OUTDIR)\selfplay.obj\
	$(LEARN_OUTDIR)\learner.obj

	
CFLAGS=\
	/arch:AVX2\
	/nologo\
	/W3\
	/c\
	/D_WIN32_WINNT=0x0600\
	/DUNICODE\
	/D_UNICODE\
	/source-charset:utf-8\
	/bigobj\
	/EHsc\
	/I$(INCLUDE_PATH)\
	/DUSE_REGRESSION\
	/DUSE_INTRIN\
	/Zi\
	/DLEARN_MODE
	# /USE_SERIALIZER\ #

LEARN_LINK_FLAGS=\
	/nologo\
	/subsystem:console\
	/pdb:"$(OUTDIR)\$(LEARN_TARGET).pdb"\
	/out:"$(OUTDIR)\$(LEARN_TARGET).exe"\
	/DEBUG


reversi: clean $(OUTDIR)\$(TARGET).exe

learn: clean $(OUTDIR)\$(LEARN_TARGET).exe

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


$(OUTDIR)\$(LEARN_TARGET).exe: $(OUTDIR) $(AI_OUTDIR) $(SEARCH_OUTDIR) $(LEARN_OUTDIR) $(LEARN_OBJS)
	$(LINK) $(LEARN_LINK_FLAGS) $(LEARN_OBJS)

{$(SRC_DIR)}.c{$(OUTDIR)}.o:
	$(CPP) $(CFLAGS) /Fo"$@" $<

{$(AI_SRC_DIR)}.c{$(AI_OUTDIR)}.o:
	$(CPP) $(CFLAGS) /Fo"$@" $<
	
{$(SEARCH_DIR)}.c{$(SEARCH_OUTDIR)}.o:
	$(CPP) $(CFLAGS) /Fo"$@" $<

{$(LEARN_DIR)}.c{$(LEARN_OUTDIR)}.o:
	$(CPP) $(CFLAGS) /Fo"$@" $<


{$(SRC_DIR)}.cpp{$(OUTDIR)}.obj:
	$(CPP) $(CFLAGS) /Fo"$@" $<

{$(AI_SRC_DIR)}.cpp{$(AI_OUTDIR)}.obj:
	$(CPP) $(CFLAGS) /Fo"$@" $<
	
{$(SEARCH_DIR)}.cpp{$(SEARCH_OUTDIR)}.obj:
	$(CPP) $(CFLAGS) /Fo"$@" $<
	
{$(LEARN_DIR)}.cpp{$(LEARN_OUTDIR)}.obj:
	$(CPP) $(CFLAGS) /Fo"$@" $<