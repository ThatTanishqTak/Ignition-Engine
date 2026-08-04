#pragma once

#ifdef _WIN32
	#ifdef IGNITION_BUILD_DLL
		#define IGNITION_API __declspec(dllexport)
	#else
		#define IGNITION_API __declspec(dllimport)
	#endif
		#define IGNITION_LOCAL
#else
	#define IGNITION_API __attribute__((visibility("default")))
	#define IGNITION_LOCAL __attribute__((visibility("hidden")))
#endif