cmake_minimum_required(VERSION 3.2)
project(TLG)

set(HDR_FILES 
	
	external/flat_hash_map.h
	
	abort.h
	audio_defaults.h
	assert.h
	audio_format.h
	audiosystem.h
	deleterptr.h
	bit_utils.h
	brr.h
	brr_file.h
	sdl_audio_utils.h
	instrument.h
	instrument_characteristics.h
	cannonical_errors.h
	make_cleanup.h
	fixed_point.h
	sampledatam16.h
	platform.h
	pstruct.h
	reflect.h
	resourcemanager.h
	samplesupplier.h
	static_type_assert.h
	status.h
	status_macros.h
	statusor.h
	sequencers16.h
	samplers16.h
	strcat.h
	tostring.h
	zsequence.h
	
	PARENT_SCOPE)

source_group("Header Files" FILES ${HDR_FILES})