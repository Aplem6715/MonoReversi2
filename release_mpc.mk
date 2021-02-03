
TARGET			=mpc_playout
LEARN_TARGET	=learn
LINK			=link.exe

OUTDIR			=.\build\release\learn

AI_OUTDIR		=$(OUTDIR)\ai
SEARCH_OUTDIR	=$(OUTDIR)\search
LEARN_OUTDIR	=$(OUTDIR)\learning

SRC_DIR		=.\src
AI_SRC_DIR	=.\src\ai
SEARCH_DIR	=.\src\search
LEARN_DIR	=.\src\learning

INCLUDE_PATH=.\src

LEARN_OBJS=\
	$(OUTDIR)\const.obj\
	$(OUTDIR)\bit_operation.obj\
	$(OUTDIR)\game.obj\
	$(OUTDIR)\board.obj\
	$(SEARCH_OUTDIR)\hash.obj\
	$(SEARCH_OUTDIR)\moves.obj\
	$(SEARCH_OUTDIR)\mid.obj\
	$(SEARCH_OUTDIR)\end.obj\
	$(SEARCH_OUTDIR)\search.obj\
	$(SEARCH_OUTDIR)\mpc_playout.obj\
	$(AI_OUTDIR)\eval.obj\
	$(AI_OUTDIR)\ai_const.obj\
	$(AI_OUTDIR)\nnet.obj\
	$(AI_OUTDIR)\regression.obj\
	$(LEARN_OUTDIR)\game_record.obj\

	
CFLAGS=\
	/Ox\
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
	/DNDEBUG\
	/DLEARN_MODE\
	/DUSE_INTRIN\
	/DUSE_REGRESSION
	# /USE_SERIALIZER\ #

LEARN_LINK_FLAGS=\
	/nologo\
	/subsystem:console\
	/out:"$(OUTDIR)\$(LEARN_TARGET).exe"\


learn: clean $(OUTDIR)\$(LEARN_TARGET).exe

all: reversi learn

.PHONY: clean
clean:
	-@erase /Q $(OUTDIR)\*
	-@erase /Q (AI_OUTDIR)\*
	-@erase /Q $(SEARCH_OUTDIR)\*
	-@erase /Q $(LEARN_OUTDIR)\*



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

{$(SRC_DIR)}.cpp{$(OUTDIR)}.obj:
	$(CPP) $(CFLAGS) /Fo"$(OUTDIR)\\" /Fd"$(OUTDIR)\\" $<

{$(AI_SRC_DIR)}.cpp{$(AI_OUTDIR)}.obj:
	$(CPP) $(CFLAGS) /Fo"$(AI_OUTDIR)\\" /Fd"$(AI_OUTDIR)\\" $<
	
{$(SEARCH_DIR)}.cpp{$(SEARCH_OUTDIR)}.obj:
	$(CPP) $(CFLAGS) /Fo"$(SEARCH_OUTDIR)\\" /Fd"$(SEARCH_OUTDIR)\\" $<

{$(LEARN_DIR)}.cpp{$(LEARN_OUTDIR)}.obj:
	$(CPP) $(CFLAGS) /Fo"$(LEARN_OUTDIR)\\" /Fd"$(LEARN_OUTDIR)\\" $<
	