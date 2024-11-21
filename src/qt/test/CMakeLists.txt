# Copyright (c) 2018 The Bitcoin developers

project(test_dogecash-qt)

include(TestSuite)
create_test_suite(dogecash-qt)
add_dependencies(check check-dogecash-qt)

add_test_to_suite(dogecash-qt test_dogecash-qt
	apptests.cpp
	bitcoinaddressvalidatortests.cpp
	compattests.cpp
	guiutiltests.cpp
	optiontests.cpp
	rpcnestedtests.cpp
	test_main.cpp
	uritests.cpp
	util.cpp
)

target_link_libraries(test_dogecash-qt testutil dogecash-qt-base Qt5::Test)

# Wallet
if(BUILD_BITCOIN_WALLET)
	# Add wallet functionality to test_dogecash-qt
	target_sources(test_dogecash-qt
		PRIVATE
			addressbooktests.cpp
			wallettests.cpp
			../../wallet/test/wallet_test_fixture.cpp
	)

	if(ENABLE_BIP70)
		target_sources(test_dogecash-qt
			PRIVATE
				paymentservertests.cpp
		)
	endif()

	target_link_libraries(test_dogecash-qt wallet)
endif()
