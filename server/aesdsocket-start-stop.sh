#! /bin/sh

case "$1" in
	start)
	/usr/bin/aesdsocket -d
	;;
	stop)
	killall aesdsocket
	;;
	*)
	exit 1
esac
exit 0
