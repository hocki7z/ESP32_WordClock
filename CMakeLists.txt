cmake_minimum_required(VERSION 3.16.0)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(ESP32_WordClock)

# Arduino 2.x does not declare a dependency on our project-local LittleFS
# component. Its LittleFS.cpp needs that component's public include directory.
idf_build_get_property(wordclock_components BUILD_COMPONENTS)
foreach(wordclock_component IN LISTS wordclock_components)
	if(wordclock_component MATCHES "^framework-arduinoespressif32($|[-@])")
		idf_component_get_property(wordclock_arduino_lib "${wordclock_component}" COMPONENT_LIB)
		target_link_libraries(${wordclock_arduino_lib} PUBLIC idf::littlefs)
	endif()
endforeach()