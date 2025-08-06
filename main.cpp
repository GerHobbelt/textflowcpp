#define CATCH_CONFIG_MAIN
#include <catch2/catch.hpp>

#include <catch2/catch_all.hpp>

namespace Catch {
	CATCH_INTERNAL_START_WARNINGS_SUPPRESSION
		CATCH_INTERNAL_SUPPRESS_GLOBALS_WARNINGS
		static LeakDetector leakDetector;
	CATCH_INTERNAL_STOP_WARNINGS_SUPPRESSION
}


// Standard C/C++ main entry point
extern "C"
int main(int argc, const char** argv) {

	using namespace Catch;

	// We want to force the linker not to discard the global variable
	// and its constructor, as it (optionally) registers leak detector
	(void)&leakDetector;

	{
		Session session; // There must be exactly one instance
		auto& cfg = session.configData();
		cfg.warnings = WarnAbout::What(WarnAbout::UnmatchedTestSpec | WarnAbout::NoAssertions);
		cfg.verbosity = Verbosity::High;
		//cfg.libIdentify = true;

#if 0
		session.ignoreStartupExceptions();
#endif

		// Let Catch2 (using Clara) parse the command line
		int returnCode = session.applyCommandLine(argc, argv);
		if (returnCode != 0) // Indicates a command line error
			return returnCode;

		session.run();
	}
	return 0;
}

