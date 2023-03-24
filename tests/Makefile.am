clean:
	rm Apple*.fw

tests:
	for driver in AppleUSBVideoSupport*; do \
	echo $$driver;\
	$(top_builddir)/src/ift-extract -d . -f $${driver}.fw -a $$driver;\
	done
