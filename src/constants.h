/*
 * passwords - A GnuPG based password manager
 *
 * Copyright (C) 2025  Adam.Lanzafame <sysal@tuta.io>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */


#pragma once

namespace Passwords {
inline constexpr const char* Name           = "Passwords";
inline constexpr const char* Organization   = "sysal1280";
inline constexpr const char* Version        = "1.3.1";
inline constexpr const char* Icon           = ":/password.png";

inline constexpr int SBTransientMessageTime = 5000;

inline constexpr const char* HelpBaseUrl    = "https://sysal1280.github.io/passwords/";
inline constexpr const char* GitUrl         = "https://github.com/sysal1280/passwords.git";
inline constexpr const char* BugUrl         = "https://github.com/sysal1280/passwords/security";

inline constexpr const char* License        = "Licensed under the <a href=\"https://www.gnu.org/licenses/gpl-3.0.html\">GNU GPLv3 License</a> or later.";
inline constexpr const char* WarrantyDisclaimer =
    "This program is distributed in the hope that it will be useful, but it is provided without any warranty; "
    "including without the implied warranties of merchantability or fitness for a particular purpose. "
    "For full details, see the GNU General Public License.";

inline constexpr const char* WordlistsCredit = "Wordlists by Sam Schlinkert (MIT License)";
inline constexpr const char* MaterialSymbolsCredit = "Material Symbols by Google (Apache 2.0 License)";
inline constexpr const char* IconCreditFormat = "%1 icon by Iconic Panda (Flaticon)";

inline constexpr const char* debugWarningSB = "Debug Version Only";
inline constexpr const char* debugWarningMB =
    "This is a Debug build.\n\n"
    "Debug builds include extra diagnostic information and reduced security "
    "protections. They are not safe for production use.\n\n"
    "Do not create or access real passwords or other sensitive data in this build.\n\n"
    "If this program has been installed for you, uninstall it immediately.";

}
