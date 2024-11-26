# Copyright (c) 2017-2020 The Bitcoin developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

project(doge-seeder)

add_library(seeder-base
	bitcoin.cpp
	db.cpp
	dns.cpp
	options.cpp
)
target_link_libraries(seeder-base server)

add_executable(doge-seeder
	main.cpp
)
target_link_libraries(doge-seeder seeder-base)

include(BinaryTest)
add_to_symbols_check(doge-seeder)
add_to_security_check(doge-seeder)

include(InstallationHelper)
install_target(doge-seeder)

add_subdirectory(test)
