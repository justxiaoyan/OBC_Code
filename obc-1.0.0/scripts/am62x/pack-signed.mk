# Common AM62x signing entry point.
.PHONY: pack-signed
pack-signed: output
	@set -eu; \
	if [ -z "$(PACK_INPUT)" ] || [ -z "$(PACK_OUTPUT)" ]; then \
		echo "ERROR: pack-signed requires PACK_INPUT and PACK_OUTPUT"; exit 2; \
	fi; \
	if [ ! -f "$(PACK_INPUT)" ]; then \
		echo "ERROR: signing input not found: $(PACK_INPUT)"; exit 1; \
	fi; \
	if [ ! -x "$(SIGN_TOOL)" ]; then \
		echo "ERROR: signing tool not found: $(SIGN_TOOL)"; \
		echo "       run 'make tools' first"; exit 1; \
	fi; \
	mkdir -p "$(dir $(PACK_OUTPUT))"; \
	if [ "$(PACK_HEAD_WRITE)" = "1" ]; then \
		"$(SIGN_TOOL)" -h "$(PACK_INPUT)" "$(PACK_OUTPUT)"; \
	else \
		"$(SIGN_TOOL)" "$(PACK_INPUT)" "$(PACK_OUTPUT)"; \
	fi; \
	echo "Signed: $(PACK_OUTPUT)"
