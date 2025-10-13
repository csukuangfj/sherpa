

.PHONY: all
all:
	latexmk -xelatex pic.tex

.PHONY: clean
clean:
	@#latexmk -C # it cleans all, i.e., also deletes pdf files
	# latexmk -c
	$(RM) -f *.aux *.log *.out *.bbl *.blg *.toc *.vrb *.snm *.nav
	$(RM) -f *.dvi *.fls *.fdb_latexmk *.xdv

