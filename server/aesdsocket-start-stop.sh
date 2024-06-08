#! /bin/sh

case "$1" in
	start)
	aesdsocket -d
	;;
	stop)
	killall aesdsocket
	;;
	*)
	exit 1
esac
exit 0
