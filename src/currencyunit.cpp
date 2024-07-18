// Copyright (c) 2021 The Bitcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <cashaddr.h>
#include <currencyunit.h>

#include <common/args.h>

void SetupCurrencyUnitOptions(ArgsManager &argsman) {
    // whether to use eCash default unit and address prefix
    argsman.AddArg("-ecash",
                   strprintf("Use the eCash address prefix (default: %s)",
                             cashaddr::DEFAULT_ECASH ? "true" : "false"),
                   ArgsManager::ALLOW_BOOL, OptionsCategory::OPTIONS);
    argsman.AddArg("-usedogeunit",
                   strprintf("Use the DOGE unit (default: %s)",
                             DEFAULT_USE_DOGE_UNIT ? "true" : "false"),
                   ArgsManager::ALLOW_BOOL, OptionsCategory::OPTIONS);
}
