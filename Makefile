# GNU Make solution makefile autogenerated by Premake
# Type "make help" for usage help

ifndef config
  config=debug
endif
export config

PROJECTS := OpenClam

.PHONY: all clean help $(PROJECTS)

all: $(PROJECTS)

OpenClam: 
	@echo "==== Building OpenClam ($(config)) ===="
	@${MAKE} --no-print-directory -C premake_files -f Makefile

clean:
	@${MAKE} --no-print-directory -C premake_files -f Makefile clean

help:
	@echo "Usage: make [config=name] [target]"
	@echo ""
	@echo "CONFIGURATIONS:"
	@echo "   debug"
	@echo "   release"
	@echo ""
	@echo "TARGETS:"
	@echo "   all (default)"
	@echo "   clean"
	@echo "   OpenClam"
	@echo ""
	@echo "For more information, see http://industriousone.com/premake/quick-start"
