# SPDX-License-Identifier: MIT
.PHONY: pdf clean
pdf:
	latexmk -lualatex -interaction=nonstopmode -halt-on-error -file-line-error -outdir=build main.tex
clean:
	latexmk -C -outdir=build main.tex
