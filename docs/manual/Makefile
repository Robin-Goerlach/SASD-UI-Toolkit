# SPDX-License-Identifier: MIT
.PHONY: pdf pdf-print clean
pdf:
	latexmk -lualatex -interaction=nonstopmode -halt-on-error -file-line-error -outdir=build main.tex
pdf-print:
	latexmk -lualatex -interaction=nonstopmode -halt-on-error -file-line-error -outdir=build print.tex
clean:
	latexmk -C -outdir=build main.tex
	latexmk -C -outdir=build print.tex
