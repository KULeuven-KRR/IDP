
#Directory where your IDP3 git is located
IDPGITDIR=~/software/idp
#Directory where you build IDP3
IDPBUILDDIR=/export/home1/NoCsBack/dtai/bartb/builds/idp/debug
#Command that runs idp (or a make check) and return 0 iff sucessful
TESTCOMMAND=idp /home/bartb/software/idp/tests/definitiontests/function.idp -e "print(S); return 0;"
#Git will execute the above commands on all commits between $START and $END
#Commit to start searching from (typically master)
START=fffb88f
#Commit to end searching (typically HEAD)
END=HEAD


cd $IDPGITDIR
for i in `git rev-list $START..$END`; do
	cd $IDPGITDIR
	git checkout $i  > /dev/null 2>&1
	cd $IDPBUILDDIR
	make -j 4 install > /dev/null 2>&1
	echo $i 
	$TESTCOMMAND || exit ;
done
