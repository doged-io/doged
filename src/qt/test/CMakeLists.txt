# Copyright (c) 2018 The Bitcoin developers

project(test_dogecoin-qt)

include(TestSuite)
create_test_suite(dogecoin-qt)
add_dependencies(check check-dogecoin-qt)

add_test_to_suite(dogecoin-qt test_dogecoin-qt
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

target_link_libraries(test_dogecoin-qt testutil bitcoin-qt-base Qt5::Test)

# Wallet
if(BUILD_BITCOIN_WALLET)
	# Add wallet functionality to test_dogecoin-qt
	target_sources(test_dogecoin-qt
		PRIVATE
			addressbooktests.cpp
			wallettests.cpp
			../../wallet/test/wallet_test_fixture.cpp
	)

	if(ENABLE_BIP70)
		target_sources(test_dogecoin-qt
			PRIVATE
				paymentservertests.cpp
		)
	endif()

	target_link_libraries(test_dogecoin-qt wallet)
endif()
