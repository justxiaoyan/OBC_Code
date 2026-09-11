.PHONY: tools tools_clean
tools:
	$(MAKE) -C "$(SIGN_TOOLS_DIR)" all
tools_clean:
	$(MAKE) -C "$(SIGN_TOOLS_DIR)" clean
