tags_ahre: tags ahre

ahre:
	$(MAKE) -C src ../build/ahre


run_tests:
	$(MAKE) -C utests run_tests


tags: $(wildcard $(AHRE_SRCDIR)/*.c) clean-tags
	ctags -R \
		--exclude=.git \
		--exclude=hotl/scripts \
		--exclude=html \
		--exclude=js \
		--exclude=tmp \
		--exclude=build \
		--exclude=quickjs/tests \
		--exclude=quickjs/test262 \
		--exclude=quickjs/examples  .

cscope.files: $(AHRE_HEADERS) $(AHRE_SRCS)
	if [ -f cscope.files ]; then rm cscope.files; fi
	find . \( -path "./.git"  -o -path "./che" \) ! -prune -o -name "*.[ch]" > $@

cscope: cscope.files 
	cscope -R -b -i $<

clean-tags:
	if [ -f tags ]; then rm tags; fi

clean-cscope:
	find . -type f -name "cscope.*" -delete

clean: clean-cscope clean-tags
	find build -type f -delete

