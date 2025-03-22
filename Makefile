ahre:
	$(MAKE) -C src ../build/ahre

test_all:
	$(MAKE) -C utests test_all

clean:
	$(MAKE) -C src clean

