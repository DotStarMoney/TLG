cmake_minimum_required(VERSION 3.2)
project(TLG)
set(THIS ${CMAKE_CURRENT_LIST_DIR})

# ----------------------------- ADD SUBDIRECTORIES -----------------------------

# -------------------------- ADD SOURCES IN DIRECTORY --------------------------
set(SRC_FILES 
	"${SRC_FILES}"
	
	${THIS}/audiosystem.cc
	${THIS}/brr.cc
	${THIS}/brr_file.cc
	${THIS}/delayline.cc
	${THIS}/format.cc
	${THIS}/instrument.cc
	${THIS}/karlsenfl3filter.cc
	${THIS}/mixer.cc
	${THIS}/sampledatam16.cc
	${THIS}/samplers16.cc
	${THIS}/utils.cc
	${THIS}/zsequence.cc
		
	PARENT_SCOPE)

# -------------------------- ADD HEADERS IN DIRECTORY --------------------------
set(HDR_FILES 
	"${HDR_FILES}"
	
	${THIS}/audiocontext.h
	${THIS}/audiosystem.h
	${THIS}/bimodefilter.h
	${THIS}/brr.h
	${THIS}/brr_file.h
	${THIS}/consts.h
	${THIS}/delayline.h
	${THIS}/format.h
	${THIS}/instrument.h
	${THIS}/instrument_characteristics.h
	${THIS}/karlsenfl3filter.h
	${THIS}/mixer.h
	${THIS}/sampledatam16.h
	${THIS}/samplers16.h
	${THIS}/samplesupplier.h
	${THIS}/sandcompressor.h
	${THIS}/sequencers16.h
	${THIS}/sharedsamplers16.h
	${THIS}/utils.h
	${THIS}/zsequence.h
	
	PARENT_SCOPE)