# File: XSB/docs/userman/Makefile
#
# $Id: Makefile,v 1.7 2010-08-19 15:03:36 spyrosh Exp $

.PHONY: all manual1 manual2

all: manual1 manual2

manual1:
	latex manual1	
	bibtex manual1
	makeindex manual1
	makeindex manual1.prdx -o manual1.pnd
	latex manual1
	latex manual1
	dvips manual1.dvi -o manual1.ps
	dvipdf manual1.dvi

manual2:
	latex manual2	
	bibtex manual2
	makeindex manual1
	latex manual2
	latex manual2
	dvips manual2.dvi -o manual2.ps
	dvipdf manual2.dvi
