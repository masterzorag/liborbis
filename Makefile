.DEFAULT_GOAL := all

ifndef Ps4Sdk
ifdef ps4sdk
Ps4Sdk := $(ps4sdk)
endif
ifdef PS4SDK
Ps4Sdk := $(PS4SDK)
endif
ifndef Ps4Sdk
$(error Neither PS4SDK, Ps4Sdk nor ps4sdk set)
endif
endif
				
all:
	make all install -C portlibs/zlib
	make all install -C portlibs/libpng
	make all install -C orbislink/libdebugnet
	make all install -C orbislink/libelfloader
	make all install -C orbislink/libps4link
	make all install -C liborbis2d
	make all install -C liborbisAudio
	make all install -C liborbisFileBrowser
	make all install -C liborbisKeyboard
	make all install -C liborbisPad
	make all install -C liborbisXbmFont
	make all install -C libmod