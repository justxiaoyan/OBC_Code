.PHONY: tools tools_clean sign_tools sign_tools_clean mkkimg mkkimg_clean
tools: sign_tools mkkimg

sign_tools:
	$(MAKE) -C "$(SIGN_TOOLS_DIR)" all

sign_tools_clean:
	$(MAKE) -C "$(SIGN_TOOLS_DIR)" clean

mkkimg:
	$(MAKE) -C "$(MKKIMG_DIR)" all

mkkimg_clean:
	$(MAKE) -C "$(MKKIMG_DIR)" clean

tools_clean: sign_tools_clean mkkimg_clean
