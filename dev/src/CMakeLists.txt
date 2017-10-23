cmake_minimum_required(VERSION 3.2)
project(PCDataGen)


set(SRC_FILES 
	
	main.cpp 
	audio_format.cpp
	brr.cpp
	cannonical_errors.cpp
	audiosystem.cpp
	monosampledata16.cpp
	status.cpp
	brr_file.cpp
	sdl_audio_utils.cpp
	make_cleanup.cpp
	stereosampler16.cpp
		
	PARENT_SCOPE)

source_group("Source Files" FILES ${SRC_FILES})