# Copyright (c) 2025 Tobias Ruck and Alexandre Guillioud

include(TestSuite)
create_test_suite(stratum)
add_dependencies(check check-stratum)

add_boost_unit_tests_to_suite(stratum test_stratum
	TESTS
		stratumconfig_tests.cpp
		stratumprotocol_tests.cpp
		stratumworker_tests.cpp
		stratumjob_tests.cpp
		stratumsubmit_tests.cpp
		stratumaux_tests.cpp
		stratumstats_tests.cpp
)

target_link_libraries(test_stratum server testutil)
