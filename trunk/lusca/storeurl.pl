#!/usr/bin/perl
# $Rev$
# by chudy_fernandez@yahoo.com
# Youtube updates at http://wiki.squid-cache.org/ConfigExamples/DynamicContent/YouTube/Discussion
$|=1;
while (<>) {
    @X = split;
        $x = $X[0] . " ";
        $_ = $X[1];
        $u = $X[1];

                        #profile.ak.fbcdn.net/hprofile-ak-xxx/xxx.jpg
if (m/^http:\/\/profile.ak.fbcdn.net\/hprofile-ak-[a-z]{3}[0-9]\/(.*)/) {
        print $x . "http://profile.ak.fbcdn.net/hprofile-ak-cdn/" . $1  . "\n";
		
                        #(xx.sphotos|photos-x).ak.fbcdn.net/hphotos-ak-xxx/
} elsif (m/^http:\/\/.*?([a-z]{4,}).*?.ak.fbcdn.net\/([a-z]*)-ak-[a-z]{3}[0-9]\/([^&?]*)/) {
        print $x . "http://$1.ak.fbcdn.net/$2-ak-cdn/$3\n";
		
                        #maps.google.com
} elsif (m/^http:\/\/(khm|mt)[0-9]?(.google.com.*)/) {
        print $x . "http://" . $1  . $2 . "\n";
		
                        #youtube/google video with range
} elsif ($X[1] =~ /(youtube|google).*(videoplayback|liveplay)\?/){
	@itag = m/[&?](itag=[0-9]*)/;
	@id = m/[&?](id=[^\&]*)/;
	@range = m/[&?](range=[^\&]*)/;
        print $x . "http://video-srv.youtube.com.SQUIDINTERNAL/@id&@itag@range\n";
		
} elsif (m/^http:\/\/www\.google-analytics\.com\/__utm\.gif\?.*/) {
        print $x . "http://STOREURL.www.google-analytics.com/__utm.gif\n";

                        #Cache High Latency Ads
} elsif (m/^http:\/\/([a-z0-9.]*)(\.doubleclick\.net|\.quantserve\.com|\.googlesyndication\.com|yieldmanager|cpxinteractive)(.*)/) {
        $y = $3;$z = $2;
        for ($y) {
        s/pixel;.*/pixel/;
        s/activity;.*/activity/;
        s/(imgad[^&]*).*/\1/;
        s/;ord=[?0-9]*//;
        s/;&timestamp=[0-9]*//;
        s/[&?]correlator=[0-9]*//;
        s/&cookie=[^&]*//;
        s/&ga_hid=[^&]*//;
        s/&ga_vid=[^&]*//;
        s/&ga_sid=[^&]*//;
        # s/&prev_slotnames=[^&]*//
        # s/&u_his=[^&]*//;
        s/&dt=[^&]*//;
        s/&dtd=[^&]*//;
        s/&lmt=[^&]*//;
        s/(&alternate_ad_url=http%3A%2F%2F[^(%2F)]*)[^&]*/\1/;
        s/(&url=http%3A%2F%2F[^(%2F)]*)[^&]*/\1/;
        s/(&ref=http%3A%2F%2F[^(%2F)]*)[^&]*/\1/;
        s/(&cookie=http%3A%2F%2F[^(%2F)]*)[^&]*/\1/;
        s/[;&?]ord=[?0-9]*//;
        s/[;&]mpvid=[^&;]*//;
        s/&xpc=[^&]*//;
        # yieldmanager
        s/\?clickTag=[^&]*//;
        s/&u=[^&]*//;
        s/&slotname=[^&]*//;
        s/&page_slots=[^&]*//;
        }
        print $x . "http://STOREURL." . $1 . $2 . $y . "\n";

                        #cache high latency ads
} elsif (m/^http:\/\/(.*?)\/(ads)\?(.*?)/) {
        print $x . "http://STOREURL." . $1 . "/" . $2  . "\n";

} elsif (m/^http:\/\/(www\.ziddu\.com.*\.[^\/]{3,4})\/(.*?)/) {
        print $x . "http://STOREURL." . $1 . "\n";

                        #cdn, varialble 1st path
} elsif (($u =~ /filehippo/) && (m/^http:\/\/(.*?)\.(.*?)\/(.*?)\/(.*)\.([a-z0-9]{3,4})(\?.*)?/)) {
        @y = ($1,$2,$4,$5);
        $y[0] =~ s/[a-z0-9]{2,5}/cdn./;
        print $x . "http://STOREURL." . $y[0] . $y[1] . "/" . $y[2] . "." . $y[3] . "\n";

                        #rapidshare
} elsif (($u =~ /rapidshare/) && (m/^http:\/\/(([A-Za-z]+[0-9-.]+)*?)([a-z]*\.[^\/]{3}\/[a-z]*\/[0-9]*)\/(.*?)\/([^\/\?\&]{4,})$/)) {
        print $x . "http://cdn." . $3 . "/SQUIDINTERNAL/" . $5 . "\n";

} elsif (($u =~ /maxporn/) && (m/^http:\/\/([^\/]*?)\/(.*?)\/([^\/]*?)(\?.*)?$/)) {
        print $x . "http://" . $1 . "/SQUIDINTERNAL/" . $3 . "\n";

                        #domain/path/.*/path/filename
} elsif (($u =~ /fucktube/) && (m/^http:\/\/(.*?)(\.[^\.\-]*?[^\/]*\/[^\/]*)\/(.*)\/([^\/]*)\/([^\/\?\&]*)\.([^\/\?\&]{3,4})(\?.*?)$/)) {
        @y = ($1,$2,$4,$5,$6);
        $y[0] =~ s/(([a-zA-A]+[0-9]+(-[a-zA-Z])?$)|([^\.]*cdn[^\.]*)|([^\.]*cache[^\.]*))/cdn/;
        print $x . "http://STOREURL." . $y[0] . $y[1] . "/" . $y[2] . "/" . $y[3] . "." . $y[4] . "\n";

                        #like porn hub variables url and center part of the path, filename etention 3 or 4 with or without ? at the end
#} elsif (($u =~ /tube8|pornhub|xvideos|hardsextube/) && (#m/^http:\/\/(([A-Za-z]+[0-9-.]+)*?(\.[a-z]*)?)\.([a-z]*[0-9]?\.[^\/]{3}\/[a-z]*)(.*?)((\/[a-z]*)?(\/[^\/]*){4}\.[^\/\?]{3,4})(\?.*)?$/)) {
#        print $x . "http://cdn." . $4 . $6 . "\n";
		
} elsif (m/^http:\/\/[^\/]*(tube8|pornhub|xvideos|hardsextube)[^\/]*\/(.*)\/([^\/]*)\.([^\/.?&_;]{2,4}).*$/) {
        print $x . "http://cdn.$1/$3.$4\n";

                        #for yimg.com video
} elsif (m/^http:\/\/(.*yimg.com)\/\/(.*)\/([^\/\?\&]*\/[^\/\?\&]*\.[^\/\?\&]{3,4})(\?.*)?$/) {
        print $x . "http://cdn.yimg.com//" . $3 . "\n";

                        #for yimg.com doubled
} elsif (m/^http:\/\/(.*?)\.yimg\.com\/(.*?)\.yimg\.com\/(.*?)\?(.*)/) {
        print $x . "http://cdn.yimg.com/"  . $3 . "\n";

                        #for yimg.com with &sig=
} elsif (m/^http:\/\/([^\.]*)\.yimg\.com\/(.*)/) {
        @y = ($1,$2);
        $y[0] =~ s/[a-z]+([0-9]+)?/cdn/;
        $y[1] =~ s/&sig=.*//;
        print $x . "http://STOREURL." . $y[0] . ".yimg.com/"  . $y[1] . "\n";

                        #youjizz. We use only domain and filename
} elsif (($u =~ /media[0-9]{1,5}\.youjizz/) && (m/^http:\/\/(.*?)(\.[^\.\-]*?\.[^\/]*)\/(.*)\/([^\/\?\&]*)\.([^\/\?\&]{3,4})(\?.*?)$/)) {
        @y = ($1,$2,$4,$5);
        $y[0] =~ s/(([a-zA-A]+[0-9]+(-[a-zA-Z])?$)|([^\.]*cdn[^\.]*)|([^\.]*cache[^\.]*))/cdn/;
        print $x . "http://STOREURL." . $y[0] . $y[1] . "/" . $y[2] . "." . $y[3] . "\n";

                        #general purpose for cdn servers. add above your specific servers.
} elsif (m/^http:\/\/([0-9.]*?)\/\/(.*?)\.(.*)\?(.*?)/) {
        print $x . "http://squid-cdn-url//" . $2  . "." . $3 . "\n";

                        # spicific extention
# } elsif (m/^http:\/\/(.*?)\.(jp(e?g|e|2)|gif|png|tiff?|bmp|ico|flv|wmv|3gp|mp(4|3)|exe|msi|zip|on2|mar|swf).*?/) {
        # @y = ($1,$2);
        # $y[0] =~ s/((cache|cdn)[-\d]*)|([a-zA-A]+-?[0-9]+(-[a-zA-Z]*)?)/cdn/;
        # print $x . "http://" . $y[0] . "." . $y[1] . "\n";

                        #generic http://variable.domain.com/path/filename."ex", "ext" or "exte"
                        #http://cdn1-28.projectplaylist.com
                        #http://s1sdlod041.bcst.cdn.s1s.yimg.com
} elsif (m/^http:\/\/([^\/]*?)(\.[^\.\-]*?\.[^?&]*)\/([^.]*)\.([\w\d]{2,4})[?&]?.*/) {
        @y = ($1,$2,$3,$4);
        $y[0] =~ s/([a-z][0-9][a-z]dlod[\d]{3})|((cache|cdn)[-\d]*)|([a-zA-A]+-?[0-9]+(-[a-zA-Z]*)?)/cdn/;
	print $x . "http://STOREURL.$y[0]$y[1]/$y[2].$y[3]\n"; 

                        # all that ends with ;
} elsif (m/^http:\/\/(.*?)\/(.*?)\;(.*)/) {
        print $x . "http://STOREURL." . $1 . "/" . $2  . "\n";

} else {
        print $x . $_ . "\n";
}
}
