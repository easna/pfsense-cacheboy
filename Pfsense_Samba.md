# Introduction #

Share files between pfsense and windows.
WARNING: not recommended for firewall. Security issues.


#### Instruction ####
  * Run this in console.
```
fetch http://pfsense-cacheboy.googlecode.com/svn/trunk/script/samba_install.sh && chmod a+x samba_install.sh && ./samba_install.sh
```

#### Tips ####
as a default samba starts on boot, for security and memory issues you can turn it off by:
```
mv /usr/local/etc/rc.d/samba.sh /usr/local/etc/rc.d/samba
```
to start
```
/usr/local/etc/rc.d/samba onestart
```
to stop
```
/usr/local/etc/rc.d/samba onestop
```
configurations at /usr/local/etc/smb.conf