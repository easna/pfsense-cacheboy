#!/bin/sh
pkg_add -r mrtg
fetch http://pfsense-cacheboy.googlecode.com/svn/trunk/conf/mrtg.cfg
mv mrtg.cfg /usr/local/etc/mrtg/mrtg.cfg
chown mrtg:mrtg /usr/local/etc/mrtg/mrtg.cfg
tae () {
sed 's+\(^Workdir: \).*+\1'$dir'+' mrtg.cfg > mrtg.cfg.tmp
break
}
while :
do
echo "Mrtg Working Directory path"
echo "/usr/local/www/mrtg"
read dir
case $dir in
	[/a-zA-Z0-9-]*)
	tae
	;;
esac
done
cat mrtg.cfg.tmp

