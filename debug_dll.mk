
TARGET			=MonoReversi
LINK			=link.exe

OUTDIR			=.\build\debug\dll

AI_OUTDIR		=$(OUTDIR)\ai
SEARCH_OUTDIR	=$(OUTDIR)\search

SRC_DIR		=.\src
AI_SRC_DIR	=.\src\ai
SEARCH_DIR	=.\src\search

INCLUDE_PATH=.\src

OBJS=\
	$(OUTDIR)\const.obj\
	$(OUTDIR)\bit_operation.obj\
	$(OUTDIR)\board.obj\
	$(OUTDIR)\game.obj\
	$(OUTDIR)\dll.obj\
	$(AI_OUTDIR)\eval.obj\
	$(AI_OUTDIR)\ai_const.obj\
	$(AI_OUTDIR)\nnet.obj\
	$(AI_OUTDIR)\regression.obj\
	$(SEARCH_OUTDIR)\bench.obj\
	$(SEARCH_OUTDIR)\hash.obj\
	$(SEARCH_OUTDIR)\moves.obj\
	$(SEARCH_OUTDIR)\mpc_info.obj\
	$(SEARCH_OUTDIR)\mid.obj\
	$(SEARCH_OUTDIR)\end.obj\
	$(SEARCH_OUTDIR)\search.obj

	
CFLAGS=\
	/arch:AVX2\
	/nologo\
	/W3\
	/c\
	/DUNICODE\
	/D_UNICODE\
	/source-charset:utf-8\
	/bigobj\
	/EHsc\
	/I$(INCLUDE_PATH)\
	/DUSE_REGRESSION\
	/DUSE_INTRIN\
	/Zi\
	/DWIN32\
	/D_WINDOWS\
	/D_USRDLL\
	/D_WINDLL\
	#/DLEARN_MODE
	# /USE_SERIALIZER\ #

LINK_FLAGS=\
	/nologo\
	/subsystem:windows\
	/pdb:"$(OUTDIR)\$(TARGET).pdb"\
	/out:"$(OUTDIR)\$(TARGET).dll"\
	/DLL\
	/DEBUG



reversi: clean $(OUTDIR)\$(TARGET).dll

all: reversi learn

.PHONY: clean
clean:
	-erase /Q $(OUTDIR)\*
	-erase /Q $(AI_OUTDIR)\*
	-erase /Q $(SEARCH_OUTDIR)\*


$(OUTDIR):
	@if not exist $(OUTDIR) mkdir $(OUTDIR)

$(AI_OUTDIR):
	@if not exist $(AI_OUTDIR) mkdir $(AI_OUTDIR)

$(SEARCH_OUTDIR):
	@if not exist $(SEARCH_OUTDIR) mkdir $(SEARCH_OUTDIR)


$(OUTDIR)\$(TARGET).dll: $(OUTDIR) $(AI_OUTDIR) $(SEARCH_OUTDIR) $(OBJS)
	$(LINK) $(LINK_FLAGS) $(OBJS)


{$(SRC_DIR)}.cpp{$(OUTDIR)}.obj:
	$(CPP) $(CFLAGS) /Fo"$(OUTDIR)\\" /Fd"$(OUTDIR)\\" $<

{$(AI_SRC_DIR)}.cpp{$(AI_OUTDIR)}.obj:
	$(CPP) $(CFLAGS) /Fo"$(AI_OUTDIR)\\" /Fd"$(AI_OUTDIR)\\" $<
	
{$(SEARCH_DIR)}.cpp{$(SEARCH_OUTDIR)}.obj:
	$(CPP) $(CFLAGS) /Fo"$(SEARCH_OUTDIR)\\" /Fd"$(SEARCH_OUTDIR)\\" $<
