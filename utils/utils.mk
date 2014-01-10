LIBUTILS_SOURCES := \
        environment.cc \
        file_functions.cc \
        filter_streams.cc \
        string_functions.cc \
        parse_context.cc \
	configuration.cc \
	csv.cc \
	exc_check.cc \
	exc_assert.cc \
	hex_dump.cc \
	lzma.cc \
	floating_point.cc \
	json_parsing.cc \
	rng.cc \
	hash.cc \
	abort.cc \
	lz4_filters.cc

LIBUTILS_LINK :=	ACE arch boost_iostreams lzma lz4 boost_thread cryptopp

$(eval $(call library,utils,$(LIBUTILS_SOURCES),$(LIBUTILS_LINK)))

LIBWORKER_TASK_SOURCES := worker_task.cc
LIBWORKER_TASK_LINK    := ACE arch pthread

$(eval $(call library,worker_task,$(LIBWORKER_TASK_SOURCES),$(LIBWORKER_TASK_LINK)))

$(eval $(call include_sub_make,utils_testing,testing))
