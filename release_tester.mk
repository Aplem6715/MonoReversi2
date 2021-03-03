
TARGET			=tester
LINK			=link.exe

OUTDIR			=.\build\release\play

AI_OUTDIR		=$(OUTDIR)\ai
SEARCH_OUTDIR	=$(OUTDIR)\search

SRC_DIR		=.\src
AI_SRC_DIR	=.\src\ai
SEARCH_DIR	=.\src\search
INCLUDE_PATH=.\src

OBJS=\
	$(OUTDIR)\const.o\
	$(OUTDIR)\bit_operation.o\
	$(OUTDIR)\board.o\
	$(OUTDIR)\game.o\
	$(AI_OUTDIR)\eval.o\
	$(AI_OUTDIR)\ai_const.o\
	$(AI_OUTDIR)\nnet.o\
	$(AI_OUTDIR)\regression.o\
	$(SEARCH_OUTDIR)\random_util.o\
	$(SEARCH_OUTDIR)\hash.o\
	$(SEARCH_OUTDIR)\moves.o\
	$(SEARCH_OUTDIR)\mpc_info.o\
	$(SEARCH_OUTDIR)\mid.o\
	$(SEARCH_OUTDIR)\end.o\
	$(SEARCH_OUTDIR)\search.o\
	$(SEARCH_OUTDIR)\search_manager.o\
	$(OUTDIR)\tester.o

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
	/DUSE_INTRIN\
	/DUSE_REGRESSION
	#/DLEARN_MODE
	# /USE_SERIALIZER\ #

LINK_FLAGS=\
	/nologo\
	/subsystem:console\
	/out:"$(OUTDIR)\$(TARGET).exe"\


reversi: clean $(OUTDIR)\$(TARGET).exe

all: reversi

.PHONY: clean
clean:
	-@erase /Q $(OUTDIR)\*
	-@erase /Q $(AI_OUTDIR)\*
	-@erase /Q $(SEARCH_OUTDIR)\*



$(OUTDIR):
	@if not exist $(OUTDIR) mkdir $(OUTDIR)

$(AI_OUTDIR):
	@if not exist $(AI_OUTDIR) mkdir $(AI_OUTDIR)

$(SEARCH_OUTDIR):
	@if not exist $(SEARCH_OUTDIR) mkdir $(SEARCH_OUTDIR)


$(OUTDIR)\$(TARGET).exe: $(OUTDIR) $(AI_OUTDIR) $(SEARCH_OUTDIR) $(OBJS)
	$(LINK) $(LINK_FLAGS) $(OBJS)


{$(SRC_DIR)}.c{$(OUTDIR)}.o:
	$(CPP) $(CFLAGS) /Fo"$@" $<

{$(AI_SRC_DIR)}.c{$(AI_OUTDIR)}.o:
	$(CPP) $(CFLAGS) /Fo"$@" $<
	
{$(SEARCH_DIR)}.c{$(SEARCH_OUTDIR)}.o:
	$(CPP) $(CFLAGS) /Fo"$@" $<


{$(SRC_DIR)}.cpp{$(OUTDIR)}.obj:
	$(CPP) $(CFLAGS) /Fo"$@" /Fd"$(OUTDIR)\\" $<

{$(AI_SRC_DIR)}.cpp{$(AI_OUTDIR)}.obj:
	$(CPP) $(CFLAGS) /Fo"$@" /Fd"$(AI_OUTDIR)\\" $<
	
{$(SEARCH_DIR)}.cpp{$(SEARCH_OUTDIR)}.obj:
	$(CPP) $(CFLAGS) /Fo"$@" /Fd"$(SEARCH_OUTDIR)\\" $<