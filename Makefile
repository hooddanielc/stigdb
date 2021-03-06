.PHONY: test apps clean

apps: tools/starsha
	starsha stig/stig stig/server/stig stig/spa/spa stig/client/stig stig/indy/disk/util/stig_dm starsha/starsha starsha/dummy

release: tools/starsha
	starsha stig/stig stig/server/stig stig/spa/spa stig/client/stig stig/indy/disk/util/stig_dm starsha/starsha starsha/dummy --config=release

tools/starsha:
	./bootstrap.sh

test: tools/starsha
	starsha --all="*.test" --test

test_lang: apps
	cd stig/lang_tests; ./run_tests.py

clean:
	rm -f ../.starsha/.notes
	rm -rf ../out/
	rm -f tools/starsha
