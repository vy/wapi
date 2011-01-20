TEXNAME  = slide
TARGET   = $(TEXNAME).pdf
IMGFILES = dox1.jpg dox2.jpg dox3.jpg dox4.jpg feeling-lucky.jpg
TMPFILES = \
	$(TEXNAME).aux \
	$(TEXNAME).log \
	$(TEXNAME).nav \
	$(TEXNAME).out \
	$(TEXNAME).snm \
	$(TEXNAME).vrb \
	$(TEXNAME).toc

%.pdf: %.tex
	pdflatex -halt-on-error $< $@

all: $(IMGFILES) $(TARGET)

cleantemp:
	rm -f $(TMPFILES)

clean: cleantemp
	rm -f $(TARGET)

.PHONY: all clean cleantemp
