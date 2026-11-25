PYTHON ?= python3
JOBS ?= 8
.PHONY: all verify rebuild pack clean sync-sdk
all:
	$(PYTHON) tools/build_app.py --jobs $(JOBS)
verify:
	$(PYTHON) tools/build_app.py --verify-only
rebuild:
	$(PYTHON) tools/build_app.py --rebuild --jobs $(JOBS)
pack:
	$(PYTHON) tools/build_app.py --pack-only
clean:
	$(PYTHON) tools/build_app.py --clean
sync-sdk:
	$(PYTHON) tools/sync_sdk.py --jobs $(JOBS)
