####################################################################
#Copyright 2010-2012 Katholieke Universiteit Leuven
# 
#Use of this software is governed by the GNU LGPLv3.0 license
#
#Written by Broes De Cat, Stef De Pooter, Johan Wittocx
#and Bart Bogaerts, K.U.Leuven, Departement Computerwetenschappen,
#Celestijnenlaan 200A, B-3001 Leuven, Belgium
####################################################################

SUBDIRS = lib src tests

dist_doc_DATA = README 

data_stddir = $(datadir)/std

data_std_DATA = data/std/idp_intern.idp data/std/idp_intern.lua data/std/mx.idp

data_localdir = $(localstatedir)/idp
data_local_DATA = data/std/.idprc

dist_doc_DATA = $(data_std_DATA) $(data_local_DATA)