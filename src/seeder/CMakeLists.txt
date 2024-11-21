# Copyright (c) 2017-2020 The Bitcoin developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

project(dogecoin-seeder)

add_library(seeder-base
	bitcoin.cpp
	db.cpp
	dns.cpp
	options.cpp
)
target_link_libraries(seeder-base server)

add_executable(dogecoin-seeder
	main.cpp
)
target_link_libraries(dogecoin-seeder seeder-base)

include(BinaryTest)
add_to_symbols_check(dogecoin-seeder)
add_to_security_check(dogecoin-seeder)

include(InstallationHelper)
install_target(dogecoin-seeder)

add_subdirectory(test)
