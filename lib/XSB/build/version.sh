#! /bin/sh

# Do not touch xsb_patch_date! It is updated by CVS.
xsb_patch_date='$Date: 2014-07-03 17:24:44 -0400 (Thu, 03 Jul 2014) $'

xsb_build_date=`date`
xsb_patch_date=`echo $xsb_patch_date | sed 's,.*Da,Patch da,' | sed 's, *\\$,,'`


xsb_major_version=3
xsb_minor_version=5
xsb_patch_version=0
#xsb_beta_version=pre"
#xsb_codename="Skol"        # for Version 2.1
#xsb_codename="Tsingtao"    # for Version 2.2
#xsb_codename="Zombie"      # Version 2.3
#xsb_codename="Bavaria"     # the beer from Holland
#xsb_codename="Okocim"      # from Poland! ;)
#xsb_codename="Duff"        # Simpsons
#xsb_codename="Kinryo"      # 2.7 Jonah's friend's uncle's sake
#xsb_codename="Sagres"      # version 3.0, Portuguese Beer, Rui's favorite
#xsb_codename="Incognito"   # version 3.1 Portuguese Syrah intro'd to Terry by Antonio Porto
#xsb_codename="Kopi Lewak"  # Version 3.2 Civit coffee
#xsb_codename="Pignoletto"  # Version 3.3 Italian Beer (Fabrizio)
#xsb_codename="Soy mILK"    # Version 3.4 (Benjamin)
xsb_codename="Maotai"       # Version 3.5 Chinese Beer (Neng-Fa)

# Format: YYYY-MM-DD or YYYY.MM.DD or YYYY/MM/DD
# With this, XSB should become Y2K compliant :-)
#xsb_release_date=2006-08-07
#xsb_release_date=2009-03-15
#xsb_release_date=2011-04-12
#xsb_release_date=2011-05-11
#xsb_release_date=2011-07-02
#xsb_release_date=2013-05-01
xsb_release_date=2013-07-06

