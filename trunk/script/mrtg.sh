#!/bin/sh
pkg_add -r mrtg
fetch http://pfsense-cacheboy.googlecode.com/svn/trunk/conf/mrtg.cfg
fetch http://pfsense-cacheboy.googlecode.com/svn/trunk/script/mrtg_daemon.sh
mv mrtg_daemon.sh /usr/local/etc/rc.d
chmod a+x /usr/local/etc/rc.d/mrtg_daemon.sh
chmod a+rw /var
tae () {
sed 's+\(^Workdir: \).*+\1'$dir'+' mrtg.cfg > mrtg.cfg.tmp
break
}
while :
do
echo "Mrtg Working Directory path (default)"
echo "/usr/local/mrtg"
read dir
case $dir in
	[/a-zA-Z0-9-]*)
	tae
	;;
esac
done
ln -s "$dir" /usr/local/www/mrtg
mv mrtg.cfg.tmp /usr/local/etc/mrtg/mrtg.cfg
chown mrtg:mrtg /usr/local/etc/mrtg/mrtg.cfg
cat mrtg.cfg | grep Workdir
mkdir "$dir"
chmod a+rw "$dir"
indexmaker --output="$dir/index.html" /usr/local/etc/mrtg/mrtg.cfg
/usr/local/etc/rc.d/mrtg_daemon.sh stop
/usr/local/etc/rc.d/mrtg_daemon stop
/usr/local/etc/rc.d/mrtg_daemon.sh start
hostname=`hostname | sed 's/\...*//'`
echo "You may access your mrtg at http://$hostname/mrtg"