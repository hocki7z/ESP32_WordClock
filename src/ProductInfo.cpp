#include "ProductInfo.h"

#include "AutoGitInfo.h"

namespace
{
// Build-time metadata macros are emitted by tools/generate_git_info.py into
// AutoGitInfo.h; assemble the application-facing record from those values.
constexpr ProductInfoNS::Record gProductInfoRecord{
    {
        AUTO_GIT_INFO_SW_VERSION_MAJOR,
        AUTO_GIT_INFO_SW_VERSION_MINOR,
        AUTO_GIT_INFO_SW_VERSION_BUILD,
    },
    AUTO_GIT_INFO_GEN_DATE_ISO_8601,
    AUTO_GIT_INFO_GIT_BRANCH_SHORT,
    AUTO_GIT_INFO_COMMIT_HASH_SHORT,
};
}

/** @brief Return the build metadata populated from the generated AutoGitInfo.h. */
const ProductInfoNS::Record& ProductInfoNS::GetRecord(void)
{
    return gProductInfoRecord;
}