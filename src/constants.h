#pragma once

namespace Passwords {
inline constexpr const char* Name           = "Passwords";
inline constexpr const char* Organization   = "sysal1280";
inline constexpr const char* Version        = "1.2.1";
inline constexpr const char* Icon           = ":/password.png";

inline constexpr int SBTransientMessageTime = 5000;

inline constexpr const char* HelpBaseUrl    = "https://sysal1280.github.io/passwords/";
inline constexpr const char* GitUrl         = "https://github.com/sysal1280/passwords.git";
inline constexpr const char* BugUrl         = "https://github.com/sysal1280/passwords/security/advisories/new";

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
    "This is a DEBUG build.\n\n"
    "It is not safe for production use. It is intended for testing purposes only. "
    "Do not store real passwords or other proper data in this database.\n\n"
    "If this program has been installed for you, uninstall it immediately.";

}
