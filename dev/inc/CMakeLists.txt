cmake_minimum_required(VERSION 3.2)
project(PCDataGen)


set(HDR_FILES 
	
	abort.h
	audio_defaults.h
	assert.h
	audio_format.h
	audiosystem.h
	bit_utils.h
	brr.h
	brr_file.h
	sdl_audio_utils.h
	cannonical_errors.h
	make_cleanup.h
	fixed_point.h
	sampledatam16.h
	platform.h
	pstruct.h
	reflect.h
	samplesupplier.h
	static_type_assert.h
	status.h
	status_macros.h
	statusor.h
	samplers16.h
	strcat.h
	tostring.h
	zsequence.h
	
	PARENT_SCOPE)

source_group("Header Files" FILES ${HDR_FILES})