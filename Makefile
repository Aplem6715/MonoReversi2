
OUTDIR			=.\build
AI_OUTDIR		=.\build\ai
SEARCH_OUTDIR	=.\build\search
LEARN_OUTDIR	=.\build\learning

.PHONY: clean
clean:
	-@erase /Q $(OUTDIR)\* $(AI_OUTDIR)\* $(SEARCH_OUTDIR)\* $(LEARN_OUTDIR)\*

